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

#ifndef _KEYMAPS_H_
#define _KEYMAPS_H_

#include <stdint.h>

#define SCANCODE_LSHIFT			0x2a
#define SCANCODE_RSHIFT			0x36
#define SCANCODE_LALT			0x38
#define SCANCODE_CAPSLOCK		0x3a
#define SCANCODE_NUMLOCK		0x45

typedef struct _v2r_keymap_t {
	uint32_t noshift[256];
	uint32_t shift[256];
	uint32_t altgr[256];
	uint32_t capslock[256];
	uint32_t shiftcapslock[256];
} v2r_keymap_t;

extern uint8_t scancode_to_x11_keycode_map[][2];
extern v2r_keymap_t *get_keymap_by_layout(uint32_t keyboard_layout);

#endif  // _KEYMAPS_H_
