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

#ifndef _CAPABILITIES_H_
#define _CAPABILITIES_H_

#include "packet.h"
#include "rdp.h"

#define TS_CAPS_PROTOCOLVERSION				0x0200

#define CAPSTYPE_GENERAL					0x0001
#define CAPSTYPE_BITMAP						0x0002
#define CAPSTYPE_ORDER						0x0003
#define CAPSTYPE_BITMAPCACHE				0x0004
#define CAPSTYPE_CONTROL					0x0005
#define CAPSTYPE_ACTIVATION					0x0007
#define CAPSTYPE_POINTER					0x0008
#define CAPSTYPE_SHARE						0x0009
#define CAPSTYPE_COLORCACHE					0x000A
#define CAPSTYPE_SOUND						0x000C
#define CAPSTYPE_INPUT						0x000D
#define CAPSTYPE_FONT						0x000E
#define CAPSTYPE_BRUSH						0x000F
#define CAPSTYPE_GLYPHCACHE					0x0010
#define CAPSTYPE_OFFSCREENCACHE				0x0011
#define CAPSTYPE_BITMAPCACHE_HOSTSUPPORT	0x0012
#define CAPSTYPE_BITMAPCACHE_REV2			0x0013
#define CAPSTYPE_VIRTUALCHANNEL				0x0014
#define CAPSTYPE_DRAWNINEGRIDCACHE			0x0015
#define CAPSTYPE_DRAWGDIPLUS				0x0016
#define CAPSTYPE_RAIL						0x0017
#define CAPSTYPE_WINDOW						0x0018
#define CAPSETTYPE_COMPDESK					0x0019
#define CAPSETTYPE_MULTIFRAGMENTUPDATE		0x001A
#define CAPSETTYPE_LARGE_POINTER			0x001B
#define CAPSETTYPE_SURFACE_COMMANDS			0x001C
#define CAPSETTYPE_BITMAP_CODECS			0x001D
#define CAPSSETTYPE_FRAME_ACKNOWLEDGE		0x001E

/* General Capability Set - osMajorType */
#define OSMAJORTYPE_UNSPECIFIED				0x0000
#define OSMAJORTYPE_WINDOWS					0x0001
#define OSMAJORTYPE_OS2						0x0002
#define OSMAJORTYPE_MACINTOSH				0x0003
#define OSMAJORTYPE_UNIX					0x0004
#define OSMAJORTYPE_IOS						0x0005
#define OSMAJORTYPE_OSX						0x0006
#define OSMAJORTYPE_ANDROID					0x0007

/* General Capability Set - osMinorType */
#define OSMINORTYPE_UNSPECIFIED				0x0000
#define OSMINORTYPE_WINDOWS_31X				0x0001
#define OSMINORTYPE_WINDOWS_95				0x0002
#define OSMINORTYPE_WINDOWS_NT				0x0003
#define OSMINORTYPE_OS2_V21					0x0004
#define OSMINORTYPE_POWER_PC				0x0005
#define OSMINORTYPE_MACINTOSH				0x0006
#define OSMINORTYPE_NATIVE_XSERVER			0x0007
#define OSMINORTYPE_PSEUDO_XSERVER			0x0008

/* General Capability Set - extraFlags */
#define FASTPATH_OUTPUT_SUPPORTED			0x0001
#define NO_BITMAP_COMPRESSION_HDR			0x0400
#define LONG_CREDENTIALS_SUPPORTED			0x0004
#define AUTORECONNECT_SUPPORTED				0x0008
#define ENC_SALTED_CHECKSUM					0x0010

/* Input Capability Set - inputFlags */
#define INPUT_FLAG_SCANCODES				0x0001
#define INPUT_FLAG_MOUSEX					0x0004
#define INPUT_FLAG_FASTPATH_INPUT			0x0008
#define INPUT_FLAG_UNICODE					0x0010
#define INPUT_FLAG_FASTPATH_INPUT2			0x0020
#define INPUT_FLAG_UNUSED1					0x0040
#define INPUT_FLAG_UNUSED2					0x0080
#define TS_INPUT_FLAG_MOUSE_HWHEEL			0x0100

typedef void (*v2r_cap_write_func)(v2r_rdp_t *r, v2r_packet_t *p);

extern uint16_t v2r_cap_get_write_count();
extern void v2r_cap_write_caps(v2r_rdp_t *r, v2r_packet_t *p);

#endif  // _CAPABILITIES_H_
