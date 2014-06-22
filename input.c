/**
 * vnc2rdp: proxy for RDP client connect to VNC server
 *
 * Copyright 2014 Yiwei Li <leeyiw@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "input.h"
#include "log.h"
#include "vnc.h"

static int
v2r_input_process_keyboard_event(v2r_rdp_t *r, v2r_packet_t *p)
{
	uint16_t keyboard_flags, key_code;

	V2R_PACKET_READ_UINT16_LE(p, keyboard_flags);
	V2R_PACKET_READ_UINT16_LE(p, key_code);
	V2R_PACKET_SEEK_UINT16(p);

	v2r_log_debug("keyboard_flags: 0x%x, key_code: 0x%x", keyboard_flags,
				  key_code);

	// TODO XKeycodeToKeysym

	//if (v2r_vnc_send_key_event(r->session->vnc, 1, 0x0051) == -1) {
	//	goto fail;
	//}
	//if (v2r_vnc_send_key_event(r->session->vnc, 0, 0x0051) == -1) {
	//	goto fail;
	//}

	return 0;

fail:
	return -1;
}

static int
v2r_input_process_mouse_event(v2r_rdp_t *r, v2r_packet_t *p)
{
	uint8_t button_mask = 0;
	static uint8_t button1_clicked, button2_clicked, button3_clicked;
	uint16_t pointer_flags;
	static uint16_t x_pos, y_pos;

	V2R_PACKET_READ_UINT16_LE(p, pointer_flags);
	if (pointer_flags & PTRFLAGS_MOVE) {
		V2R_PACKET_READ_UINT16_LE(p, x_pos);
		V2R_PACKET_READ_UINT16_LE(p, y_pos);
	} else {
		V2R_PACKET_SEEK(p, 4);
	}
	if (pointer_flags & PTRFLAGS_WHEEL) {
		if (pointer_flags & PTRFLAGS_WHEEL_NEGATIVE) {
			button_mask = RFB_POINTER_WHEEL_DOWNWARDS;
		} else {
			button_mask = RFB_POINTER_WHEEL_UPWARDS;
		}
		if (v2r_vnc_send_pointer_event(r->session->vnc, button_mask, x_pos,
									   y_pos) == -1) {
			goto fail;
		}
		if (v2r_vnc_send_pointer_event(r->session->vnc, 0, x_pos,
									   y_pos) == -1) {
			goto fail;
		}
	} else {
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
		if (v2r_vnc_send_pointer_event(r->session->vnc, button_mask, x_pos,
									   y_pos) == -1) {
			goto fail;
		}
	}

	return 0;

fail:
	return -1;
}

int
v2r_input_process(v2r_rdp_t *r, v2r_packet_t *p)
{
	uint16_t i, num_events, message_type;
	uint32_t event_time;

	V2R_PACKET_READ_UINT16_LE(p, num_events);
	V2R_PACKET_SEEK_UINT16(p);

	for (i = 0; i < num_events; i++) {
		V2R_PACKET_READ_UINT32_LE(p, event_time);
		V2R_PACKET_READ_UINT16_LE(p, message_type);
		switch (message_type) {
		case INPUT_EVENT_SYNC:
			V2R_PACKET_SEEK(p, 6);
			break;
		case INPUT_EVENT_UNUSED:
			V2R_PACKET_SEEK(p, 6);
			break;
		case INPUT_EVENT_SCANCODE:
			if (v2r_input_process_keyboard_event(r, p) == -1) {
				goto fail;
			}
			break;
		case INPUT_EVENT_UNICODE:
			V2R_PACKET_SEEK(p, 6);
			break;
		case INPUT_EVENT_MOUSE:
			if (v2r_input_process_mouse_event(r, p) == -1) {
				goto fail;
			}
			break;
		case INPUT_EVENT_MOUSEX:
			V2R_PACKET_SEEK(p, 6);
			break;
		default:
			v2r_log_warn("unknown input event message type 0x%x", message_type);
			break;
		}
	}

	return 0;

fail:
	return -1;
}
