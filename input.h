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

#ifndef _INPUT_H_
#define _INPUT_H_

#include "rdp.h"

/* Slow-Path Input Event - messageType */
#define INPUT_EVENT_SYNC			0x0000
#define INPUT_EVENT_UNUSED			0x0002
#define INPUT_EVENT_SCANCODE		0x0004
#define INPUT_EVENT_UNICODE			0x0005
#define INPUT_EVENT_MOUSE			0x8001
#define INPUT_EVENT_MOUSEX			0x8002

/* Keyboard Event - keyboardFlags */
#define KBDFLAGS_EXTENDED			0x0100
#define KBDFLAGS_DOWN				0x4000
#define KBDFLAGS_RELEASE			0x8000

/* Mouse Event - pointerFlags */
#define PTRFLAGS_HWHEEL				0x0400
#define PTRFLAGS_WHEEL				0x0200
#define PTRFLAGS_WHEEL_NEGATIVE		0x0100
#define WheelRotationMask			0x01FF
#define PTRFLAGS_MOVE				0x0800
#define PTRFLAGS_DOWN				0x8000
#define PTRFLAGS_BUTTON1			0x1000
#define PTRFLAGS_BUTTON2			0x2000
#define PTRFLAGS_BUTTON3			0x4000
#define PTRFLAGS_BUTTON_ALL			0x7000

/* Synchronize Event - toggleFlags */
#define TS_SYNC_SCROLL_LOCK			0x00000001
#define TS_SYNC_NUM_LOCK			0x00000002
#define TS_SYNC_CAPS_LOCK			0x00000004
#define TS_SYNC_KANA_LOCK			0x00000008

extern int v2r_input_process(v2r_rdp_t *r, v2r_packet_t *p);

#endif  // _INPUT_H_
