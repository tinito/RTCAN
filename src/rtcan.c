/**
 * @file    rtcan.c
 * @brief   RTcan code.
 *
 * @addtogroup RTCAN
 * @{
 */
#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "rtcan.h"
#include "queue.h"

#include "rtcan_lld.h"

#include "uid.h"

#define MASTER_UID8 40

static msg_t rtcanRxThread(void *arg);
void rtcan_tim_cb(GPTDriver* gptp);
void rtcan_rx_isr_code(int32_t id);

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/
//#define RTCAN_HRT
#define RTCAN_MAX_QUEUE_LENGTH 32

#define RTCAN_RX_WA_SIZE      THD_WA_SIZE(1024)
#define RTCAN_TX_WA_SIZE      THD_WA_SIZE(1024)

#define RTCAN_DEBUG_LOG(s)
//#define RTCAN_DEBUG_LOG(string, args...) chprintf((BaseSequentialStream *)&SERIAL_DRIVER, string"\r\n", ##args)
//#define RTCAN_DEBUG_LOG(string, args...) chprintf((BaseSequentialStream *)&SERIAL_DRIVER, "%s:%d - "string"\r\n", __FILE__, __LINE__, ##args)

//#define RTCAN_DEBUG_ERROR_HOOK()
#define RTCAN_DEBUG_ERROR_HOOK() while (1)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

RTCANDriver RTCAND;

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

rtcan_msgqueue_t rtcan_srt_queue;

rtcan_msg_t *rtcan_onair[RTCAN_MBOX_NUM] = { NULL };

uint32_t reservation_mask[2];

uint32_t srt_queued = 0;

rtcan_msg_t sync_msg = {
	NULL,
	NULL,
	RTCAN_MSG_READY,
	NULL,
	NULL,
	0,
	0,
	0,
	0,
	reservation_mask,
	reservation_mask,
	8,
	0
};

/*
 * RTCAN default configuration.
 */
static const RTCANConfig rtcan_default_config = { 1000000, 100, 60 };

/*
 * CAN configuration.
 */
static const CANConfig can_cfg = { rtcan_lld_tx_cb, rtcan_rx_isr_code, NULL, CAN_MCR_NART | CAN_MCR_TTCM,
		CAN_BTR_SJW(0) | CAN_BTR_TS2(2) | CAN_BTR_TS1(4) | CAN_BTR_BRP(3) };

//static const CANConfig can_cfg = { rtcan_lld_tx_cb, rtcan_rx_isr_code, NULL, CAN_MCR_NART | CAN_MCR_TTCM,
//		CAN_BTR_SJW(0) | CAN_BTR_TS2(2) | CAN_BTR_TS1(4) | CAN_BTR_BRP(3) | CAN_BTR_LBKM};

/*
 * TIM configuration.
 */
static const GPTConfig tim_cfg = { 1000000, rtcan_tim_cb };

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

bool_t rtcan_ismaster(void) {

#ifdef RTCAN_HRT
	if (uid8() == 40) {
		return TRUE;
	}
	if (uid8() == 70) {
		return TRUE;
	}
#endif

	return FALSE;
}

/**
 * @brief   XXX.
 *
 * @api
 */
void rtcan_txok_isr_code(int32_t id) {
	rtcan_msg_t *msg_p;

	chSysLockFromIsr();
	palTogglePad(LED_GPIO, LED2);

	msg_p = rtcan_onair[id];
	rtcan_onair[id] = NULL;

	// XXX FIXME ERRORE da qui? pare di no...
	if (msg_p == NULL) {
		chSysUnlockFromIsr();
		return;
	}

	if (msg_p->fragment > 0) {
		msg_p->fragment--;
		msg_p->ptr += RTCAN_FRAME_SIZE;
		msg_p->status = RTCAN_MSG_QUEUED;
	} else {
		rtcan_msgqueue_remove(msg_p);
		srt_queued--;
		msg_p->status = RTCAN_MSG_READY;
		if (msg_p->callback) {
			msg_p->callback(msg_p);
		}
	}

#ifndef RTCAN_HRT
	srt_tx();
#endif

	chSysUnlockFromIsr();
}

void rtcan_alst_isr_code(int32_t id) {
	rtcan_msg_t* msg_p;

	chSysLockFromIsr();

	msg_p = rtcan_onair[id];
	msg_p->status = RTCAN_MSG_QUEUED;
	rtcan_onair[id] = NULL;

#ifndef RTCAN_HRT
	srt_tx();
#endif
	chSysUnlockFromIsr();
}

/**
 * @brief   XXX.
 *
 * @api
 */
void rtcan_rx_isr_code(int32_t id) {
	rtcan_msg_t * msg_p;

	chSysLockFromIsr();
	palTogglePad(LED_GPIO, LED3);

	rtcan_lld_receive(&RTCAND, &msg_p);

	if (msg_p->id == 0) {
		gptStopTimerI(RTCAND.tim);
		gptStartContinuousI(RTCAND.tim, 166); // FIXME
		RTCAND.tim->tim->CNT = 158;
		RTCAND.cnt++;
		RTCAND.slot = 0;
#ifdef RTCAN_TEST
		palClearPad(TEST_GPIO, TEST2);
#endif
	}

	if (msg_p->fragment > 0) {
		msg_p->fragment--;
	} else {
		msg_p->ptr = msg_p->data;
		msg_p->status = RTCAN_MSG_READY;
	}


	if (msg_p->callback) {
		msg_p->callback(msg_p);
	}

	chSysUnlockFromIsr();
}

uint8_t rtcan_format_laxity(int32_t laxity) {

	if ((laxity < 0) || (laxity > 64)) {
		return 0x3F;
	}

	return laxity & 0x3F;
}

void srt_tx(void) {
	rtcan_msg_t* msg_p;
	uint8_t mbox;

	// FIXME prova imbecille
	if (rtcan_onair[0] != NULL)
		return;

	msg_p = rtcan_msgqueue_get(&rtcan_srt_queue);

	/* Any message? */
	if (msg_p == NULL ) {
		return;
	}

	/* Check for deadline */
	if (msg_p->deadline <= chTimeNow()) {
		msg_p->status = RTCAN_MSG_TIMEOUT;
		rtcan_msgqueue_remove(msg_p);
		srt_queued--;
		return;
	}

	if (rtcan_lld_transmit(&RTCAND, msg_p, &mbox)) {
		rtcan_onair[mbox] = msg_p;
		msg_p->status = RTCAN_MSG_ONAIR;
	}
}

/**
 * @brief   XXX.
 *
 * @api
 */
void rtcan_tim_cb(GPTDriver* gptp) {

	chSysLockFromIsr();

	if (RTCAND.slot == (RTCAND.config->slots - 1)) {
		if (RTCAND.state == RTCAN_MASTER) {
#ifdef RTCAN_TEST
			palSetPad(TEST_GPIO, TEST1);
#endif
			rtcan_lld_transmit_sync(&RTCAND);
			RTCAND.cnt++;
			RTCAND.slot = 0;
#ifdef RTCAN_TEST
			palClearPad(TEST_GPIO, TEST1);
			palClearPad(TEST_GPIO, TEST2);
#endif
		}
		chSysUnlockFromIsr();
		return;
	}

	RTCAND.slot++;

#ifdef RTCAN_TEST
	palTogglePad(TEST_GPIO, TEST2);
#endif

#ifdef RTCAN_HRT
	if (rtcan_reserved_slot()) {
		hrt_tx();
		chSysUnlockFromIsr();
		return;
	}
#endif

	chSysUnlockFromIsr();
}

uint32_t rtcan_time(void) {
	return (RTCAND.cnt * RTCAND.config->slots) + RTCAND.slot;
}

bool_t rtcan_reserved_slot(void) {
	if (RTCAND.slot < 32) {
		return ((0x01 << RTCAND.slot) & reservation_mask[0]);
	}

	if (RTCAND.slot < 60) {
		return ((0x01 << (RTCAND.slot - 32)) & reservation_mask[1]);
	}

	/* Never reach */
	return FALSE;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   RTcan initialization.
 *
 * @init
 */
void rtcanInit(void) {

	chSysLock();

	RTCAND.state = RTCAN_STOP;
	RTCAND.can = &CAND1;
	RTCAND.tim = &GPTD3;
	RTCAND.cnt = -1;
	RTCAND.slot = 0;

	reservation_mask[0] = 0;
	reservation_mask[1] = 0;

	rtcan_lld_init();

#ifdef RTCAN_HRT
	hrt_init();
#endif
	rtcan_msgqueue_init(&rtcan_srt_queue);

	chSysUnlock();
}

/**
 * @brief   Configures and activates the RTCAN peripheral.
 *
 * @param[in] config    pointer to the @p RTCANConfig object
 *
 * @api
 */
void rtcanStart(const RTCANConfig *config) {
	Thread *tp;

	if (config != NULL ) {
		RTCAND.config = config;
	} else {
		RTCAND.config = &rtcan_default_config;
	}
	canStart(RTCAND.can, &can_cfg);
	canSetFilters(RTCAND.can, NULL, 0);

	chSysLock();
	rtcan_lld_addfilter(&RTCAND, &sync_msg);
	chSysUnlock();

	if (rtcan_ismaster()) {
		RTCAND.state = RTCAN_MASTER;
	} else {
		RTCAND.state = RTCAN_SLAVE;
	}
/*
	tp = chThdCreateFromHeap(NULL, RTCAN_RX_WA_SIZE, chThdGetPriority(),
			rtcanRxThread, &RTCAND);
	if (tp == NULL ) {
		RTCAN_DEBUG_LOG("out of memory\r\n");
	}
*/
#ifdef RTCAN_HRT
	gptStart(RTCAND.tim, &tim_cfg);
	if (rtcan_ismaster()) {
		gptStartContinuous(RTCAND.tim, 166); // FIXME
	}
#endif
}

/**
 * @brief   XXX.
 *
 * @api
 */
void rtcanStop(void) {

	gptStopTimer(RTCAND.tim);
	canStop(RTCAND.can);

	RTCAND.state = RTCAN_STOP;
}

/**
 * @brief   XXX.
 *
 * @api
 */
void rtcanSendSrt(rtcan_msg_t *msg_p, uint32_t laxity) {

	/* Lock message */
	msg_p->status = RTCAN_MSG_BUSY;

	/* Compute absolute deadline */
	msg_p->deadline = rtcan_now() + laxity;

	if (msg_p->size > RTCAN_FRAME_SIZE) {
		/* Reset fragment counter. */
		msg_p->fragment = msg_p->size / RTCAN_FRAME_SIZE;
	} else {
		msg_p->fragment = 0;
	}
	/* Reset data pointer. */
	msg_p->ptr = msg_p->data;

	/* XXX brutto in lock, l'inserimento puo' essere lungo */

	chSysLock();
	rtcan_msgqueue_insert(&rtcan_srt_queue, msg_p);
	msg_p->status = RTCAN_MSG_QUEUED;
	srt_queued++;
#ifndef RTCAN_HRT
	srt_tx();
#endif
	chSysUnlock();
}

void rtcanSendSrtI(rtcan_msg_t *msg_p, uint32_t laxity) {

	/* Lock message */
	msg_p->status = RTCAN_MSG_BUSY;

	/* Compute absolute deadline */
	msg_p->deadline = rtcan_now() + laxity;

	if (msg_p->size > RTCAN_FRAME_SIZE) {
		/* Reset fragment counter. */
		msg_p->fragment = msg_p->size / RTCAN_FRAME_SIZE;
	} else {
		msg_p->fragment = 0;
	}
	/* Reset data pointer. */
	msg_p->ptr = msg_p->data;

	rtcan_msgqueue_insert(&rtcan_srt_queue, msg_p);
	msg_p->status = RTCAN_MSG_QUEUED;
	srt_queued++;
#ifndef RTCAN_HRT
	srt_tx();
#endif

}

/**
 * @brief   XXX.
 *
 * @api
 */
void rtcanRegister(RTCANDriver *rtcan_p, rtcan_msg_t *msg_p) {
	rtcan_lld_addfilter(rtcan_p, msg_p);

	msg_p->ptr = msg_p->data;
	msg_p->status = RTCAN_MSG_READY;
}

/**
 * @brief   RTCAN receiver working thread.
 *
 * @RTCAN
 */
static msg_t rtcanRxThread(void *arg) {
	RTCANDriver *rtcan_p;
	rtcan_msg_t *msg_p;
	CANRxFrame crf;

	rtcan_p = (RTCANDriver *) arg;
	(void) rtcan_p;

	while (!chThdShouldTerminate()) {
		canReceive(&CAND1, &crf);
//		msg_p = rtcan_registered[crf.FMI];

		if (msg_p == NULL ) {
			RTCAND.state = RTCAN_ERROR;
			RTCAN_DEBUG_ERROR_HOOK()
				;
			continue;
		}

		if (msg_p->status == RTCAN_MSG_READY) {
			msg_p->status = RTCAN_MSG_ONAIR;
		}

		if (msg_p->status == RTCAN_MSG_ONAIR) {
			uint32_t i;

			for (i = 0; i < crf.DLC; i++) {
				*(msg_p->ptr++) = crf.data8[i];
			}

			msg_p->fragment = crf.EID & 0x7F;

			if (msg_p->fragment > 0) {
				msg_p->fragment--;
			} else {
				msg_p->ptr = msg_p->data;
				msg_p->status = RTCAN_MSG_READY;
				if (msg_p->callback)
					msg_p->callback(msg_p);

//				rtcan_print_msg(msg_p);
			}
		}
	}

	return 0;
}

void rtcan_print_msg(rtcan_msg_t* msg_p) {
	uint32_t i;

	chprintf((BaseSequentialStream*) &SERIAL_DRIVER, "RX %5d %2d", msg_p->id,
			msg_p->fragment);
	for (i = 0; i < msg_p->size; i++) {
		chprintf((BaseSequentialStream*) &SERIAL_DRIVER, " %x", msg_p->data[i]);
	}
	chprintf((BaseSequentialStream*) &SERIAL_DRIVER, "\r\n");
}

/**
 * @brief   RTCAN TX test thread.
 *
 * @RTCAN
 */
msg_t rtcanTxTestThread(void *arg) {
	rtcan_test_msg_t *test_msg_p;
	RTCANDriver *rtcan_p;
	rtcan_msg_t *msg_p;
	uint32_t period;
	systime_t time;
	systime_t test_time;
	rtcan_msgstatus_t status;

	(void) rtcan_p;

	test_msg_p = (rtcan_test_msg_t *) arg;

	rtcan_p = test_msg_p->rtcan_p;
	msg_p = test_msg_p->msg_p;
	period = test_msg_p->period;

	test_msg_p->msg_p = NULL;

	time = chTimeNow();

	while (!chThdShouldTerminate()) {
		time += MS2ST(period);
		test_time = chTimeNow();
		status = msg_p->status;
		if (status != RTCAN_MSG_READY) {
			if (status == RTCAN_MSG_TIMEOUT) {
				chprintf(&SERIAL_DRIVER, "T\r\n");
				msg_p->status = RTCAN_MSG_READY;
			}
			if (status == RTCAN_MSG_ONAIR) {
				chprintf(&SERIAL_DRIVER, "O\r\n");
				msg_p->status = RTCAN_MSG_READY;
			}
		} else {
			rtcanSendSrt(msg_p, period - 1);
		}
		chThdSleepUntil(time);
	}

	return 0;
}

