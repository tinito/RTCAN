/**
 * @file    rtcan.h
 * @brief   RTCAN.
 *
 * @addtogroup XXX
 * @{
 */

#ifndef _RTCAN_H_
#define _RTCAN_H_

#include "ch.h"
#include "hal.h"

/*===========================================================================*/
/* Constants.                                                                */
/*===========================================================================*/

#define RTCAN_FRAME_SIZE 8

/*===========================================================================*/
/* Pre-compile time settings.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Data structures and types.                                         			 */
/*===========================================================================*/

/**
 * @brief   RTCAN state machine possible states.
 */
typedef enum
{
	RTCAN_UNINIT = 0,
	RTCAN_STOP = 1,
	RTCAN_STARTING = 2,
	RTCAN_SLAVE = 3,
	RTCAN_MASTER = 4,
	RTCAN_ERROR = 5,
} rtcanstate_t;


typedef struct rtcan_msg_t rtcan_msg_t;

/**
 * @brief   RTCAN receive callback type.
 */
typedef void (*rtcanrxcb_t)(
		CANRxFrame* crfp);


/**
 * @brief   RTCAN message callback type.
 */
typedef void (*rtcan_msgcallback_t)(
		rtcan_msg_t* msg_p);

/**
 * @brief   RTCAN configuration structure.
 */
typedef struct
{
	/**
	 * @brief RTCAN baudrate.
	 */
	uint32_t baudrate;
	/**
	 * @brief RTCAN sync frequency.
	 */
	uint32_t clock;
	/**
	 * @brief Number of time-slots in each cycle.
	 */
	uint32_t slots;
} RTCANConfig;

/**
 * @brief   Structure representing a RTCAN driver.
 */
typedef struct
{
	/**
	 * @brief RTCAN state.
	 */
	rtcanstate_t state;
	/**
	 * @brief Time-slot counter.
	 */
	uint32_t slot;
	/**
	 * @brief Cycle counter.
	 */
	uint32_t cnt;
	/**
	 * @brief Current configuration data.
	 */
	const RTCANConfig *config;
	/**
	 * @brief Pointer to CAN driver.
	 */
	CANDriver *can;
	/**
	 * @brief Pointer to GPT driver.
	 */
	GPTDriver *tim;
} RTCANDriver;

typedef enum
{
	RTCAN_HRT = 0,
	RTCAN_SRT = 1,
} rtcan_msgpriority_t;

typedef enum
{
	RTCAN_MSG_UNINIT = 0,
	RTCAN_MSG_BUSY = 1,
	RTCAN_MSG_READY = 2,
	RTCAN_MSG_QUEUED = 3,
	RTCAN_MSG_ONAIR = 4,
	RTCAN_MSG_TIMEOUT = 5,
	RTCAN_MSG_ERROR = 6,
} rtcan_msgstatus_t;

struct __attribute__ ((__packed__)) rtcan_msg_t
{
	rtcan_msg_t *next;
	rtcan_msg_t *prev;
	rtcan_msgstatus_t status;
	rtcan_msgcallback_t callback; // TODO move to a shared location
	void * params; // TODO move to a shared location
	uint8_t type :1; // TODO move to a shared location
	uint16_t id :15; // TODO move to a shared location
	union {
		uint32_t deadline;
		uint32_t slot;
	};
	uint16_t period;
	uint8_t *data;
	uint8_t *ptr;
	uint16_t size; // TODO move to a shared location
	uint8_t fragment :7;
};

typedef struct
{
	rtcan_msg_t *next;
	rtcan_msg_t *prev;
} rtcan_msgqueue_t;

typedef struct {
		RTCANDriver *rtcan_p;
		rtcan_msg_t *msg_p;
		uint32_t period;

} rtcan_test_msg_t;

#include "rtcan_lld.h"

/*===========================================================================*/
/* Macros.                                                            			 */
/*===========================================================================*/

// XXX non usata per ora
#define _can_txok_isr_code(msg_p) {                                            \
	if (msg_p->fragment > 0) { \
		msg_p->fragment--; \
		msg_p->ptr += RTCAN_FRAGMENT_SIZE; \
	} else { \
		chSysLockFromIsr(); \
		rtcan_msgqueue_remove(msg_p); \
		srt_queued--; \
		msg_p->status = RTCAN_MSG_READY; \
		chSysUnlockFromIsr(); \
	} \
}

#define rtcan_now() chTimeNow()
/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
void rtcanInit(
		void);
void rtcanStart(
		const RTCANConfig *config);
void rtcanStop(
		void);
void rtcanRegister(
		RTCANDriver *rtcan_p,
		rtcan_msg_t *msg_p);
void rtcanSendSrt(
		rtcan_msg_t *new_msg_p,
		uint32_t laxity);
void rtcanSendSrtI(
		rtcan_msg_t *new_msg_p,
		uint32_t laxity);
uint8_t rtcan_format_laxity(
		int32_t laxity);

msg_t rtcanTxTestThread(
		void *arg);

void rtcan_tx(void);

uint32_t rtcan_time(void);

bool_t rtcan_reserved_slot(void);

#ifdef __cplusplus
}
#endif

#endif /* _RTCAN_H_ */

/** @} */
