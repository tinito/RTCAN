#include "hrt.h"
#include "calendar.h"

static cycle_t current_cycle;

void calendar_init(calendar_t * calendar) {
	calendar->next = NULL;
	current_cycle = 0x00;
}

