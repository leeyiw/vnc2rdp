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

#include "capabilities.h"
#include "vnc.h"

static void
v2r_cap_write_general_cap(v2r_rdp_t *r, v2r_packet_t *p)
{
	/* capabilitySetType */
	V2R_PACKET_WRITE_UINT16_LE(p, CAPSTYPE_GENERAL);
	/* lengthCapability */
	V2R_PACKET_WRITE_UINT16_LE(p, 24);
	/* osMajorType */
	V2R_PACKET_WRITE_UINT16_LE(p, OSMAJORTYPE_WINDOWS);
	/* osMinorType */
	V2R_PACKET_WRITE_UINT16_LE(p, OSMINORTYPE_WINDOWS_NT);
	/* protocolVersion */
	V2R_PACKET_WRITE_UINT16_LE(p, TS_CAPS_PROTOCOLVERSION);
	/* pad2octetsA */
	V2R_PACKET_WRITE_UINT16_LE(p, 0);
	/* generalCompressionTypes */
	V2R_PACKET_WRITE_UINT16_LE(p, 0);
	/* extraFlags */
	V2R_PACKET_WRITE_UINT16_LE(p, 0);
	/* updateCapabilityFlag */
	V2R_PACKET_WRITE_UINT16_LE(p, 0);
	/* remoteUnshareFlag */
	V2R_PACKET_WRITE_UINT16_LE(p, 0);
	/* generalCompressionLevel */
	V2R_PACKET_WRITE_UINT16_LE(p, 0);
	/* refreshRectSupport */
	V2R_PACKET_WRITE_UINT8(p, 0x00);
	/* suppressOutputSupport */
	V2R_PACKET_WRITE_UINT8(p, 0x01);
}

static void
v2r_cap_write_bitmap_cap(v2r_rdp_t *r, v2r_packet_t *p)
{
	/* capabilitySetType */
	V2R_PACKET_WRITE_UINT16_LE(p, CAPSTYPE_BITMAP);
	/* lengthCapability */
	V2R_PACKET_WRITE_UINT16_LE(p, 29);
	/* preferredBitsPerPixel */
	V2R_PACKET_WRITE_UINT16_LE(p, 32);
	/* receive1BitPerPixel */
	V2R_PACKET_WRITE_UINT16_LE(p, 0x0001);
	/* receive4BitsPerPixel */
	V2R_PACKET_WRITE_UINT16_LE(p, 0x0001);
	/* receive8BitsPerPixel */
	V2R_PACKET_WRITE_UINT16_LE(p, 0x0001);
	/* desktopWidth */
	V2R_PACKET_WRITE_UINT16_LE(p, r->session->vnc->framebuffer_width);
	/* desktopHeight */
	V2R_PACKET_WRITE_UINT16_LE(p, r->session->vnc->framebuffer_height);
	/* pad2octets */
	V2R_PACKET_WRITE_UINT16_LE(p, 0);
	/* desktopResizeFlag */
	V2R_PACKET_WRITE_UINT16_LE(p, 0x0001);
	/* bitmapCompressionFlag */
	V2R_PACKET_WRITE_UINT16_LE(p, 0x0001);
	/* highColorFlags */
	V2R_PACKET_WRITE_UINT16_LE(p, 0);
	/* drawingFlags */
	V2R_PACKET_WRITE_UINT8(p, 0x00);
	/* multipleRectangleSupport */
	V2R_PACKET_WRITE_UINT16_LE(p, 0x0001);
	/* pad2octetsB */
	V2R_PACKET_WRITE_UINT16_LE(p, 0);
}

static void
v2r_cap_write_input_cap(v2r_rdp_t *r, v2r_packet_t *p)
{
	/* capabilitySetType */
	V2R_PACKET_WRITE_UINT16_LE(p, CAPSTYPE_INPUT);
	/* lengthCapability */
	V2R_PACKET_WRITE_UINT16_LE(p, 88);
	/* inputFlags */
	V2R_PACKET_WRITE_UINT16_LE(p, INPUT_FLAG_SCANCODES);
	/* pad2octetsA */
	V2R_PACKET_WRITE_UINT16_LE(p, 0);
	/* keyboardLayout */
	V2R_PACKET_WRITE_UINT32_LE(p, 0);
	/* keyboardType */
	V2R_PACKET_WRITE_UINT32_LE(p, 0);
	/* keyboardSubType */
	V2R_PACKET_WRITE_UINT32_LE(p, 0);
	/* keyboardFunctionKey */
	V2R_PACKET_WRITE_UINT32_LE(p, 0);
	/* imeFileName */
	V2R_PACKET_SEEK(p, 64);
}

v2r_cap_write_func v2r_cap_write_func_list[] = {
	v2r_cap_write_general_cap,
	v2r_cap_write_bitmap_cap,
	v2r_cap_write_input_cap
};

uint16_t
v2r_cap_get_write_count()
{
	return sizeof(v2r_cap_write_func_list)/sizeof(v2r_cap_write_func_list[0]);
}

void
v2r_cap_write_caps(v2r_rdp_t *r, v2r_packet_t *p)
{
	int i = 0, write_count = v2r_cap_get_write_count();
	for (i = 0; i < write_count; i++) {
		v2r_cap_write_func_list[i](r, p);
	}
}
