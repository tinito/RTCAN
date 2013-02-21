/**
 * @file    rtcan_lld.c
 * @brief   RTCAN platform dependent functions.
 *
 * @addtogroup XXX
 * @{
 */

#include "ch.h"
#include "hal.h"

#include "rtcan.h"
#include "rtcan_lld.h"



/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

rtcan_msg_t *rtcan_registered[RTCAN_FILTERS_NUM];
uint32_t next_fid;
uint32_t alst = 0;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   XXX.
 *
 * @api
 */
void rtcan_lld_tx_cb(
		CANDriver *canp,
		flagsmask_t flags)
{

	if (flags & CAN_TSR_TXOK0) {
		rtcan_txok_isr_code(0);
	}

	if (flags & CAN_TSR_TXOK1) {
		rtcan_txok_isr_code(1);
	}

	if (flags & CAN_TSR_TXOK2) {
		rtcan_txok_isr_code(2);
	}

	if (flags & CAN_TSR_ALST0) {
		rtcan_alst_isr_code(0);
		alst++;
	}

	if (flags & CAN_TSR_ALST1) {
		rtcan_alst_isr_code(1);
		alst++;
	}

	if (flags & CAN_TSR_ALST2) {
		rtcan_alst_isr_code(2);
		alst++;
	}

	if ((flags & CAN_TSR_TERR0) || (flags & CAN_TSR_TERR1) || (flags & CAN_TSR_TERR2)) {
		static int cnt = 0;
		cnt++;
		if (cnt > 1)
			palTogglePad(LED_GPIO, LED4);
	}
}


/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

extern uint32_t reservation_mask[2];

void rtcan_lld_init(void) {
	int i;

	for (i = 0; i < RTCAN_FILTERS_NUM; i++) {
		rtcan_registered[i] = NULL;
	}


}
void rtcan_lld_transmit_sync(RTCANDriver * const rtcanp) {
	CANTxFrame ctf;

	ctf.RTR = CAN_RTR_DATA;
	ctf.IDE = CAN_IDE_EXT;
	ctf.EID = 0;

	ctf.DLC = 8;

	ctf.data32[0] = reservation_mask[0];
	ctf.data32[1] = reservation_mask[1];

	if (!canTryTransmitI(rtcanp->can, &ctf)) {
		rtcanp->state = RTCAN_ERROR;
	}
}

/**
 * @brief   XXX.
 *
 */
bool_t rtcan_lld_transmit(
		const RTCANDriver * const rtcanp,
		const rtcan_msg_t * const msg_p,
		uint8_t *mbox)
{
	CANTxFrame ctf;
	uint8_t *ptr;
	uint8_t i;

	ctf.RTR = CAN_RTR_DATA;
	ctf.IDE = CAN_IDE_EXT;
	ctf.EID = ((msg_p->type & 0x01) << 28) | (rtcan_format_laxity(msg_p->deadline - chTimeNow()) << 22) | ((msg_p->id & 0x7FFF) << 7) | ((msg_p->fragment) & 0x7F);

	if (msg_p->fragment > 0) {
		ctf.DLC = RTCAN_FRAME_SIZE;
	} else {
		ctf.DLC = msg_p->data + msg_p->size - msg_p->ptr;
	}

	ptr = msg_p->ptr;
	for (i = 0; i < ctf.DLC; i++) {
		ctf.data8[i] = *(ptr++);
	}

	if (canTryTransmitI(rtcanp->can, &ctf)) {
		*mbox = ctf.mbox;
		return TRUE;
	} else {
		return FALSE;
	}
}


/**
 * @brief   XXX.
 *
 */
void rtcan_lld_receive(
		RTCANDriver * rtcanp,
		rtcan_msg_t ** msg_p)
{
	CANRxFrame crf;

	can_lld_receive(rtcanp->can, &crf);

	*msg_p = rtcan_registered[crf.FMI];

	if (msg_p == NULL ) {
		rtcanp->state = RTCAN_ERROR;
		return;
	}

	if ((*msg_p)->status == RTCAN_MSG_READY) {
		(*msg_p)->status = RTCAN_MSG_ONAIR;
	}

	if ((*msg_p)->status == RTCAN_MSG_ONAIR) {
		uint32_t i;

		for (i = 0; i < crf.DLC; i++) {
			*((*msg_p)->ptr++) = crf.data8[i];
		}

		(*msg_p)->fragment = crf.EID & 0x7F;
	}
}


/**
 * @brief   XXX.
 *
 */
bool_t rtcan_lld_addfilter(RTCANDriver* rtcan_p, rtcan_msg_t* msg_p) {
	CANDriver *canp = rtcan_p->can;
	CAN_FilterRegister_TypeDef * cfp;
	uint32_t id;
	uint32_t mask;
	uint32_t fmask;

	id = (((msg_p->type & 0x01) << 28) | (rtcan_format_laxity(msg_p->deadline - chTimeNow()) << 22) | ((msg_p->id & 0x7FFF) << 7) | ((msg_p->fragment) & 0x7F)) << 3;
	mask = 0x7FFF << 10;

	if (next_fid < STM32_CAN_FILTER_SLOTS) {
		cfp = &(canp->can->sFilterRegister[next_fid
				/ STM32_CAN_FILTER_SLOTS_PER_BANK]);
		fmask = (1 << (next_fid / STM32_CAN_FILTER_SLOTS_PER_BANK));
		canp->can->FMR |= CAN_FMR_FINIT;
		canp->can->FA1R &= ~fmask;

		// XXX tutto da sistemare!!!
		if (STM32_CAN_FILTER_MODE == CAN_FILTER_MODE_ID_MASK) {
			canp->can->FM1R &= ~fmask;
			if (STM32_CAN_FILTER_SCALE == CAN_FILTER_SCALE_16) {
				canp->can->FS1R &= ~fmask;
				id = ((id & 0x1FFC0000) >> 13) | 0x08 | (id & 0x00038000) >> 15;
				mask = (mask & 0x0000FFFF) << 16;
				if (next_fid % 2)
					cfp->FR2 = id | mask;
				else
					cfp->FR1 = id | mask;
			} else {
				canp->can->FS1R |= fmask;
				cfp->FR1 = id;
				cfp->FR2 = mask;
			}
		} else {
			canp->can->FM1R |= fmask;
			if (STM32_CAN_FILTER_SCALE == CAN_FILTER_SCALE_16) {
				canp->can->FS1R &= ~fmask;
				uint16_t *tmp16 = (uint16_t *) cfp;
				tmp16 = tmp16 + (next_fid % 4);
				*tmp16 = (uint16_t)(id & 0x0000FFFF);
			} else {
				canp->can->FS1R |= fmask;
				if (next_fid % 2)
					cfp->FR2 = id;
				else
					cfp->FR1 = id;
			}
		}

		canp->can->FA1R |= fmask;
		canp->can->FMR &= ~CAN_FMR_FINIT;

		rtcan_registered[next_fid] = msg_p;

		next_fid++;
		return TRUE;
	} else {
		return FALSE;
	}
}

/** @} */
