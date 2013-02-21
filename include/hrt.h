#ifndef __HRT_H__
#define __HRT_H__

#include "ch.h"
#include "rtcan.h"

typedef rtcan_msg_t hrt_msg_t;

#ifdef __cplusplus
extern "C" {
#endif

void hrt_init(void);
void hrt_reserve(hrt_msg_t * new_msg_p);
void hrt_update(hrt_msg_t * msg_p);
void hrt_send(hrt_msg_t * msg_p);

#ifdef __cplusplus
}
#endif

#endif /* __HRT_H__ */
