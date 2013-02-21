#ifndef __HRT_CALENDAR_H__
#define __HRT_CALENDAR_H__

#include "ch.h"
#include "hrt.h"

typedef struct {
	hrt_msg_t *next;
} hrt_calendar_t;

extern hrt_calendar_t hrt_calendar;

#ifdef __cplusplus
extern "C" {
#endif

void hrt_calendar_init(void);
void hrt_calendar_add(hrt_msg_t * new_msg_p);
bool_t hrt_calendar_check(uint16_t slot, uint16_t period);
bool_t hrt_set_slot(hrt_msg_t * new_msg_p);
void hrt_calendar_reserve(hrt_msg_t * new_msg_p);
void hrt_calendar_prepare(void);
void hrt_calendar_dump(void);

#ifdef __cplusplus
}
#endif

#endif /* __HRT_CALENDAR_H__ */
