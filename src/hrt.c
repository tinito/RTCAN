#include "ch.h"

#include "hrt.h"
#include "hrt_calendar.h"
extern RTCANDriver RTCAND;

#ifdef RTCAN_HRT_TEST
#include "uid.h"
extern uint32_t reservation_mask[2];
#endif


void hrt_msg(hrt_msg_t * msg_p, uint16_t id, uint8_t * data_p, uint16_t size, uint16_t period, uint16_t slot) {
	msg_p->next = NULL;
	msg_p->prev = NULL;
	msg_p->status = RTCAN_MSG_READY;
	msg_p->callback = NULL;
	msg_p->params = NULL;
	msg_p->type = RTCAN_HRT;
	msg_p->id = id & 0x7FFF;
	msg_p->slot = slot;
	msg_p->period = period;
	msg_p->data = data_p;
	msg_p->ptr = data_p;
	msg_p->size = size;
	msg_p->fragment = 0;
}

#ifdef RTCAN_HRT_TEST

void hrt_test_1x1(void) {
	static hrt_msg_t msg1;
	static hrt_msg_t msg2;
	static hrt_msg_t msg3;
	uint8_t data1[4] = "AAAA";
	uint8_t data2[4] = "BBBB";
	uint8_t data3[4] = "CCCC";
	uint16_t id = uid8() * 100;
	uint16_t offset;

	switch (uid8()) {
	case 40:
		offset = 0;
		reservation_mask[0] = 0x8e38ffff;
		reservation_mask[1] = 0x038e38ff;
		break;
	case 82:
		offset = 1;
		break;
	case 54:
		offset = 2;
		break;
	default:
		return;
		break;
	}

	hrt_msg(&msg1, id + 1, data1, sizeof(data1), 6, offset + 1);
	hrt_msg(&msg2, id + 2, data2, sizeof(data2), 30, offset + 4);
	hrt_msg(&msg3, id + 3, data3, sizeof(data3), 60, offset + 10);

	hrt_calendar_reserve(&msg1);
	hrt_calendar_reserve(&msg2);
	hrt_calendar_reserve(&msg3);
}

void hrt_test_1x1k_1x200_1x100(void) {
	static hrt_msg_t msg1;
	static hrt_msg_t msg2;
	static hrt_msg_t msg3;
	uint8_t data1[4] = "AAAA";
	uint8_t data2[4] = "BBBB";
	uint8_t data3[4] = "CCCC";
	uint16_t id = uid8() * 100;
	uint16_t offset;

	switch (uid8()) {
	case 40:
		offset = 0;
		reservation_mask[0] = 0x8e38ffff;
		reservation_mask[1] = 0x038e38ff;
		break;
	case 82:
		offset = 1;
		break;
	case 54:
		offset = 2;
		break;
	default:
		return;
		break;
	}

	hrt_msg(&msg1, id + 1, data1, sizeof(data1), 6, offset + 1);
	hrt_msg(&msg2, id + 2, data2, sizeof(data2), 30, offset + 4);
	hrt_msg(&msg3, id + 3, data3, sizeof(data3), 60, offset + 10);

	hrt_calendar_reserve(&msg1);
	hrt_calendar_reserve(&msg2);
	hrt_calendar_reserve(&msg3);
}


void hrt_test_4x200_4x100(void) {
	static hrt_msg_t msg1;
	static hrt_msg_t msg2;
	static hrt_msg_t msg3;
	static hrt_msg_t msg4;
	static hrt_msg_t msg5;
	static hrt_msg_t msg6;
	static hrt_msg_t msg7;
	static hrt_msg_t msg8;
	uint8_t data1[8] = "AAAAAAAA";
	uint8_t data2[8] = "BBBBBBBB";
	uint8_t data3[4] = "CCCC";
	uint16_t id = uid8() * 100;
	uint16_t offset;

	switch (uid8()) {
	case 40:
		offset = 0;
		reservation_mask[0] = 0x9FE7F9FE;
		reservation_mask[1] = 0x00781E07;
		break;
	case 82:
		offset = 10;
		break;
	case 54:
		offset = 20;
		break;
	default:
		return;
		break;
	}

	hrt_msg(&msg1, id + 1, data1, sizeof(data1), 30, offset + 1);
	hrt_msg(&msg2, id + 2, data2, sizeof(data2), 30, offset + 2);
	hrt_msg(&msg3, id + 3, data3, sizeof(data3), 30, offset + 3);
	hrt_msg(&msg4, id + 4, data3, sizeof(data3), 30, offset + 4);
	hrt_msg(&msg5, id + 5, data1, sizeof(data1), 60, offset + 5);
	hrt_msg(&msg6, id + 6, data2, sizeof(data2), 60, offset + 6);
	hrt_msg(&msg7, id + 7, data3, sizeof(data3), 60, offset + 7);
	hrt_msg(&msg8, id + 8, data3, sizeof(data3), 60, offset + 8);

	hrt_calendar_reserve(&msg1);
	hrt_calendar_reserve(&msg2);
	hrt_calendar_reserve(&msg3);
	hrt_calendar_reserve(&msg4);
	hrt_calendar_reserve(&msg5);
	hrt_calendar_reserve(&msg6);
	hrt_calendar_reserve(&msg7);
	hrt_calendar_reserve(&msg8);
}
#endif

void hrt_init(void) {
	hrt_calendar_init();

#ifdef RTCAN_HRT_TEST
//	hrt_test_4x200_4x100();
//	hrt_test_1x1k();
	hrt_test_1x1k_1x200_1x100();
#endif
}


void hrt_update(hrt_msg_t * msg_p) {
	msg_p->slot += msg_p->period;
}


void hrt_send(hrt_msg_t * msg_p) {
	uint8_t mbox;

	if (rtcan_lld_transmit(&RTCAND, msg_p, &mbox)) {
		msg_p->status = RTCAN_MSG_ONAIR;
	}
}


void hrt_tx(void) {
	hrt_msg_t * msg_p = hrt_calendar.next;
	uint32_t current_slot = rtcan_time();

	/* Check for empty local HRT calendar */
	if (msg_p == (hrt_msg_t *)&hrt_calendar) {
		return;
	}

	if (msg_p->slot > current_slot) {
		return;
	}

	/* Check for HRT message in the current time slot */
	if (msg_p->slot == current_slot) {
		hrt_send(msg_p);
		msg_p->status = RTCAN_MSG_ONAIR;
	}

	/* Check for missed deadline */
	if (msg_p->slot < current_slot) {
		msg_p->status = RTCAN_MSG_TIMEOUT;
	}

	/* Remove from calendar */
	hrt_calendar.next = msg_p->next;
	/* Update deadline and insert back into calendar */
	hrt_update(msg_p);
	hrt_calendar_reserve(msg_p);

	return;
}
