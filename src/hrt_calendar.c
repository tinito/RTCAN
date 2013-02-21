#include "ch.h"
#include "chprintf.h"

#include "hrt.h"
#include "hrt_calendar.h"

hrt_calendar_t hrt_calendar = {(hrt_msg_t *)&hrt_calendar};

void hrt_calendar_init(void) {
	hrt_calendar.next = (hrt_msg_t *)&hrt_calendar;
}

void hrt_calendar_add(hrt_msg_t * new_msg_p) {
    hrt_msg_t * msg_p = (hrt_msg_t *)&hrt_calendar;
    
    new_msg_p->slot = -1;

	while (msg_p->next != (hrt_msg_t *)&hrt_calendar) {
			if (new_msg_p->period < (msg_p->next)->period) {
					break;
			}
			msg_p = msg_p->next;
	}

	new_msg_p->next = msg_p->next;
	msg_p->next = new_msg_p;
}

void hrt_calendar_reserve(hrt_msg_t * new_msg_p) {
    hrt_msg_t * msg_p = (hrt_msg_t *)&hrt_calendar;

	while (msg_p->next != (hrt_msg_t *)&hrt_calendar) {
			if (new_msg_p->slot < (msg_p->next)->slot) {
					break;
			}
			msg_p = msg_p->next;
	}

	new_msg_p->next = msg_p->next;
	msg_p->next = new_msg_p;
}


bool_t hrt_calendar_check(uint16_t slot, uint16_t period) {
    hrt_msg_t * msg_p = (hrt_msg_t *)&hrt_calendar;
	uint16_t module = slot % period;

	while (msg_p->next != (hrt_msg_t *)&hrt_calendar) {
		msg_p = msg_p->next;
		if (msg_p->slot == 65535) {
			continue;
		}
		if ((period > msg_p->period) && (period % msg_p->period) != 0) {
			return FALSE;
		}
		if ((period < msg_p->period) && (msg_p->period % period) != 0) {
			return FALSE;
		}
		if (module % msg_p->period == msg_p->slot) {
			return FALSE;
		}
	}
	
	return TRUE;
}

bool_t hrt_set_slot(hrt_msg_t * new_msg_p) {
	hrt_msg_t * msg_p = (hrt_msg_t *)&hrt_calendar;
	int slot = 1;

	/* Empty calendar. */
	if (msg_p->next == (hrt_msg_t *)&hrt_calendar) {
		new_msg_p->slot = slot;
		return TRUE;
	}

	while (slot < new_msg_p->period) {
		if (hrt_calendar_check(slot, new_msg_p->period)) {
			new_msg_p->slot = slot;
			return TRUE;
		}
		msg_p = msg_p->next;
		slot++;
	}
	
	new_msg_p->slot = -1;
	
	return FALSE;
}

void hrt_calendar_prepare(void) {
    hrt_msg_t * msg_p = (hrt_msg_t *)&hrt_calendar;

	while (msg_p->next != (hrt_msg_t *)&hrt_calendar) {
		msg_p = msg_p->next;
		hrt_set_slot(msg_p);
    }
}

void hrt_calendar_dump(void) {
	hrt_msg_t * msg_p = (hrt_msg_t *)&hrt_calendar;
	int i = 0;

	chprintf((BaseSequentialStream *)&SERIAL_DRIVER, "\r\nCALENDAR DUMP at %x\r\n", (uint32_t)&hrt_calendar);
	
	while (msg_p->next != (hrt_msg_t *)&hrt_calendar) {
		msg_p = msg_p->next;
		chprintf((BaseSequentialStream *)&SERIAL_DRIVER, "hrt_calendar[%d] = %x %d %d %x\r\n", i, (uint32_t)msg_p, msg_p->period, msg_p->slot, (uint32_t)msg_p->next);
		i++;
	}
}
