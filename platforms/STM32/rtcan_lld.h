/**
 * @file    rtcan_lld.h
 * @brief   RTCAN platform dependent functions.
 *
 * @addtogroup XXX
 * @{
 */

#ifndef _RTCAN_LLD_H
#define _RTCAN_LLD_H

#define RTCAN_FILTERS_NUM 28
#define RTCAN_MBOX_NUM 3

#define CAN_FILTER_MODE_ID_MASK     0     /**< @brief Identifier mask mode. */
#define CAN_FILTER_MODE_ID_LIST     1     /**< @brief Identifier list mode. */
#define CAN_FILTER_SCALE_16         0     /**< @brief Dual 16-bit scale.    */
#define CAN_FILTER_SCALE_32         1     /**< @brief Single 32-bit scale.  */
#define STM32_CAN_FILTER_SLOTS_PER_BANK ((1 + STM32_CAN_FILTER_MODE) * (2 - STM32_CAN_FILTER_SCALE))
#define STM32_CAN_FILTER_SLOTS (STM32_CAN_MAX_FILTERS * STM32_CAN_FILTER_SLOTS_PER_BANK)

#define STM32_CAN_FILTER_MODE CAN_FILTER_MODE_ID_MASK
#define STM32_CAN_FILTER_SCALE CAN_FILTER_SCALE_32

void rtcan_lld_init(void);
void rtcan_lld_tx_cb(CANDriver *canp, flagsmask_t flags);
void rtcan_lld_transmit_sync(RTCANDriver * const rtcanp);
bool_t rtcan_lld_transmit(const RTCANDriver * const rtcanp,
		const rtcan_msg_t * const msg_p, uint8_t *mbox);
bool_t rtcan_lld_addfilter(RTCANDriver* rtcan_p, rtcan_msg_t* msg_p);
void rtcan_lld_receive(RTCANDriver * rtcanp, rtcan_msg_t ** msg_p);

#endif /* _RTCAN_LLD_H */

/** @} */
