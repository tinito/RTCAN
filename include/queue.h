/**
 * @file    rtcan_msgqueue.h
 * @brief   RTCAN message queue functions.
 *
 * @addtogroup XXX
 * @{
 */

#ifndef _RTCAN_MSGQUEUE_H_
#define _RTCAN_MSGQUEUE_H_

#include "ch.h"
#include "rtcan.h"

/*===========================================================================*/
/* Constants.                                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Pre-compile time settings.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Data structures and types.                                         			 */
/*===========================================================================*/

/*===========================================================================*/
/* Macros.                                                            			 */
/*===========================================================================*/

#define rtcan_msgqueue_isempty(queue_p) \
  ((void *)(queue_p) == (void *)(queue_p)->next)
//
//#define rtcan_msgqueue_get(queue_p) \
//  ((queue_p)->msg_next)
//
//#define rtcan_msgqueue_getnext(queue_p, msg_p) \
//  (msg_p->next_msg == queue_p) ? NULL : msg_p->next_msg;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
	void rtcan_msgqueue_init(rtcan_msgqueue_t* queue_p);
	void rtcan_msgqueue_insert(rtcan_msgqueue_t* queue_p, rtcan_msg_t* msg_p);
	void rtcan_msgqueue_remove(rtcan_msg_t* msg_p);
	rtcan_msg_t* rtcan_msgqueue_get(rtcan_msgqueue_t* queue_p);
#ifdef __cplusplus
}
#endif

#endif /* _RTCAN_MSGQUEUE_H_ */

/** @} */
