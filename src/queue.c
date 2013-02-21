/**
 * @file    rtcan.c
 * @brief   RTCAN message queue code.
 *
 * @addtogroup RTCAN
 * @{
 */
#include "ch.h"
#include "rtcan.h"
#include "queue.h"

void rtcan_msgqueue_init(rtcan_msgqueue_t* queue_p) {
	queue_p->next = queue_p->prev = (rtcan_msg_t *)(void *)queue_p;
}


void rtcan_msgqueue_insert(rtcan_msgqueue_t* queue_p, rtcan_msg_t* new_msg_p) {
	rtcan_msg_t *msg_p;

#ifdef _CHIBIOS_RT_
  chDbgCheckClassI();
  chDbgCheck((queue_p != NULL) && (new_msg_p != NULL), "rtcan_msgqueue_insert");
#endif /* _CHIBIOS_RT_ */

	/* Deadline based list insert. */
	msg_p = (rtcan_msg_t *)queue_p;

	while (msg_p->next != (rtcan_msg_t *)queue_p) {
		if (new_msg_p->deadline < (msg_p->next)->deadline) {
			break;
		}

		msg_p = msg_p->next;
	}

	new_msg_p->next = msg_p->next;
	new_msg_p->prev = msg_p;
	msg_p->next = new_msg_p;
	(new_msg_p->next)->prev = new_msg_p;
}


void rtcan_msgqueue_remove(rtcan_msg_t* msg_p) {
	rtcan_msg_t *prev_msg_p;

#ifdef _CHIBIOS_RT_
  chDbgCheckClassI();
  chDbgCheck(msg_p != NULL, "rtcan_msgqueue_remove");
  chDbgAssert(!rtcan_msgqueue_isempty((rtcan_msgqueue_t *) msg_p),
                "rtcan_msgqueue_remove(), #1", "queue is empty");
#endif /* _CHIBIOS_RT_ */

	prev_msg_p = msg_p->prev;
	prev_msg_p->next = msg_p->next;
	(prev_msg_p->next)->prev = prev_msg_p;
}

rtcan_msg_t* rtcan_msgqueue_get(rtcan_msgqueue_t* queue_p) {
	rtcan_msg_t* msg_p = queue_p->next;
	uint32_t i = RTCAN_MBOX_NUM;

	while ((msg_p != (rtcan_msg_t *)queue_p) && (i > 0)) {
		if (msg_p->status == RTCAN_MSG_QUEUED) {
			return msg_p;
		} else {
			msg_p = msg_p->next;
			i--;
		}
	}

	return NULL;
}
