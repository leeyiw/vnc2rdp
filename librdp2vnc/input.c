#include "input.h"
#include "log.h"
#include "vnc.h"

static int
r2v_input_process_mouse_event(r2v_rdp_t *r, r2v_packet_t *p)
{
	uint8_t button_mask;
	uint16_t pointer_flags, x_pos, y_pos;
	static uint8_t button1_clicked, button2_clicked, button3_clicked;

	R2V_PACKET_READ_UINT16_LE(p, pointer_flags);
	R2V_PACKET_READ_UINT16_LE(p, x_pos);
	R2V_PACKET_READ_UINT16_LE(p, y_pos);

	button_mask = 0;
	if (pointer_flags & PTRFLAGS_BUTTON1) {
		button1_clicked = pointer_flags & PTRFLAGS_DOWN ? 1 : 0;
	} else if (pointer_flags & PTRFLAGS_BUTTON2) {
		button2_clicked = pointer_flags & PTRFLAGS_DOWN ? 1 : 0;
	} else if (pointer_flags & PTRFLAGS_BUTTON3) {
		button3_clicked = pointer_flags & PTRFLAGS_DOWN ? 1 : 0;
	}
	if (button1_clicked) {
		button_mask |= RFB_POINTER_BUTTON_LEFT;
	}
	if (button2_clicked) {
		button_mask |= RFB_POINTER_BUTTON_RIGHT;
	}
	if (button3_clicked) {
		button_mask |= RFB_POINTER_BUTTON_MIDDLE;
	}
	if (r2v_vnc_send_pointer_event(r->session->vnc, button_mask, x_pos,
								   y_pos) == -1) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

int
r2v_input_process(r2v_rdp_t *r, r2v_packet_t *p)
{
	uint16_t i, num_events, message_type;
	uint32_t event_time;

	R2V_PACKET_READ_UINT16_LE(p, num_events);
	R2V_PACKET_SEEK_UINT16(p);
	r2v_log_debug("receive %d input events", num_events);

	for (i = 0; i < num_events; i++) {
		R2V_PACKET_READ_UINT32_LE(p, event_time);
		R2V_PACKET_READ_UINT16_LE(p, message_type);
		r2v_log_debug("input events %d of %d, message type: 0x%x", i + 1,
					  num_events, message_type);
		switch (message_type) {
		case INPUT_EVENT_SYNC:
			R2V_PACKET_SEEK(p, 6);
			break;
		case INPUT_EVENT_UNUSED:
			R2V_PACKET_SEEK(p, 6);
			break;
		case INPUT_EVENT_SCANCODE:
			R2V_PACKET_SEEK(p, 6);
			break;
		case INPUT_EVENT_UNICODE:
			R2V_PACKET_SEEK(p, 6);
			break;
		case INPUT_EVENT_MOUSE:
			if (r2v_input_process_mouse_event(r, p) == -1) {
				goto fail;
			}
			break;
		case INPUT_EVENT_MOUSEX:
			R2V_PACKET_SEEK(p, 6);
			break;
		default:
			r2v_log_warn("unknown input event message type 0x%x", message_type);
		}
	}

	return 0;

fail:
	return -1;
}
