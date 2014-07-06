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
v2r_input_process_sync_event(v2r_rdp_t *r, v2r_packet_t *p)
{
	uint32_t toggle_flags;

	/* pad2Octets */
	V2R_PACKET_SEEK_UINT16(p);
	/* toggleFlags */
	V2R_PACKET_READ_UINT32_LE(p, toggle_flags);
	v2r_log_debug("toggle_flags: 0x%x", toggle_flags);

	r->capslock = toggle_flags & TS_SYNC_CAPS_LOCK;
	r->numlock = toggle_flags & TS_SYNC_NUM_LOCK;

	return 0;
}

static int
v2r_input_process_keyboard_event(v2r_rdp_t *r, v2r_packet_t *p)
{
	uint8_t down_flag = 1;
	uint16_t keyboard_flags, key_code, x11_key_code;
	uint32_t key;

	V2R_PACKET_READ_UINT16_LE(p, keyboard_flags);
	V2R_PACKET_READ_UINT16_LE(p, key_code);
	V2R_PACKET_SEEK_UINT16(p);

	v2r_log_debug("keyboard_flags: 0x%x, key_code: 0x%x", keyboard_flags,
				  key_code);

	/* translate RDP scancode to X11 keycode */
	if (keyboard_flags & KBDFLAGS_EXTENDED) {
		x11_key_code = scancode_to_x11_keycode_map[key_code][1];
	} else {
		x11_key_code = scancode_to_x11_keycode_map[key_code][0];
	}

	/* if the keyboard layout is not supported currently, send nothing to
	 * server */
	if (r->keymap == NULL) {
		return 0;
	}

	down_flag = (keyboard_flags & KBDFLAGS_RELEASE) ? 0 : 1;
	/* process special scancode */
	switch (key_code) {
	case SCANCODE_LSHIFT:
		r->lshift = down_flag;
		break;
	case SCANCODE_RSHIFT:
		r->rshift = down_flag;
		break;
	case SCANCODE_LALT:
		r->altgr = down_flag;
	case SCANCODE_CAPSLOCK:
		if (down_flag) {
			r->capslock = !r->capslock;
		}
		break;
	case SCANCODE_NUMLOCK:
		if (down_flag) {
			r->numlock = !r->numlock;
		}
		break;
	default:
		break;
	}

	/* get X11 KeySym by keycode and current status */
	if ((79 <= x11_key_code) && (x11_key_code <= 91)) {
		if (r->numlock) {
			key = r->keymap->shift[x11_key_code];
		} else {
			key = r->keymap->noshift[x11_key_code];
		}
	} else if (r->lshift || r->rshift) {
		if (r->capslock) {
			key = r->keymap->shiftcapslock[x11_key_code];
		} else {
			key = r->keymap->shift[x11_key_code];
		}
	} else if (r->capslock) {
		key = r->keymap->capslock[x11_key_code];
	} else if (r->altgr) {
		key = r->keymap->altgr[x11_key_code];
	} else {
		key = r->keymap->noshift[x11_key_code];
	}

	if (v2r_vnc_send_key_event(r->session->vnc, down_flag, key) == -1) {
		goto fail;
	}

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
			if (v2r_input_process_sync_event(r, p) == -1) {
				goto fail;
			}
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
