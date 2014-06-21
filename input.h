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

extern int r2v_input_process(r2v_rdp_t *r, r2v_packet_t *p);

#endif  // _INPUT_H_
