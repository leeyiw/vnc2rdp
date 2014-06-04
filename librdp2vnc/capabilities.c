#include "capabilities.h"
#include "vnc.h"

static void
r2v_cap_write_general_cap(r2v_rdp_t *r, r2v_packet_t *p)
{
	/* capabilitySetType */
	R2V_PACKET_WRITE_UINT16_LE(p, CAPSTYPE_GENERAL);
	/* lengthCapability */
	R2V_PACKET_WRITE_UINT16_LE(p, 24);
	/* osMajorType */
	R2V_PACKET_WRITE_UINT16_LE(p, OSMAJORTYPE_WINDOWS);
	/* osMinorType */
	R2V_PACKET_WRITE_UINT16_LE(p, OSMINORTYPE_WINDOWS_NT);
	/* protocolVersion */
	R2V_PACKET_WRITE_UINT16_LE(p, TS_CAPS_PROTOCOLVERSION);
	/* pad2octetsA */
	R2V_PACKET_WRITE_UINT16_LE(p, 0);
	/* generalCompressionTypes */
	R2V_PACKET_WRITE_UINT16_LE(p, 0);
	/* extraFlags */
	R2V_PACKET_WRITE_UINT16_LE(p, 0);
	/* updateCapabilityFlag */
	R2V_PACKET_WRITE_UINT16_LE(p, 0);
	/* remoteUnshareFlag */
	R2V_PACKET_WRITE_UINT16_LE(p, 0);
	/* generalCompressionLevel */
	R2V_PACKET_WRITE_UINT16_LE(p, 0);
	/* refreshRectSupport */
	R2V_PACKET_WRITE_UINT8(p, 0x00);
	/* suppressOutputSupport */
	R2V_PACKET_WRITE_UINT8(p, 0x00);
}

static void
r2v_cap_write_bitmap_cap(r2v_rdp_t *r, r2v_packet_t *p)
{
	/* capabilitySetType */
	R2V_PACKET_WRITE_UINT16_LE(p, CAPSTYPE_BITMAP);
	/* lengthCapability */
	R2V_PACKET_WRITE_UINT16_LE(p, 29);
	/* preferredBitsPerPixel */
	R2V_PACKET_WRITE_UINT16_LE(p, 32);
	/* receive1BitPerPixel */
	R2V_PACKET_WRITE_UINT16_LE(p, 0x0001);
	/* receive4BitsPerPixel */
	R2V_PACKET_WRITE_UINT16_LE(p, 0x0001);
	/* receive8BitsPerPixel */
	R2V_PACKET_WRITE_UINT16_LE(p, 0x0001);
	/* desktopWidth */
	R2V_PACKET_WRITE_UINT16_LE(p, r->session->vnc->framebuffer_width);
	/* desktopHeight */
	R2V_PACKET_WRITE_UINT16_LE(p, r->session->vnc->framebuffer_height);
	/* pad2octets */
	R2V_PACKET_WRITE_UINT16_LE(p, 0);
	/* desktopResizeFlag */
	R2V_PACKET_WRITE_UINT16_LE(p, 0x0001);
	/* bitmapCompressionFlag */
	R2V_PACKET_WRITE_UINT16_LE(p, 0x0001);
	/* highColorFlags */
	R2V_PACKET_WRITE_UINT16_LE(p, 0);
	/* drawingFlags */
	R2V_PACKET_WRITE_UINT8(p, 0x00);
	/* multipleRectangleSupport */
	R2V_PACKET_WRITE_UINT16_LE(p, 0x0001);
	/* pad2octetsB */
	R2V_PACKET_WRITE_UINT16_LE(p, 0);
}

static void
r2v_cap_write_input_cap(r2v_rdp_t *r, r2v_packet_t *p)
{
	/* capabilitySetType */
	R2V_PACKET_WRITE_UINT16_LE(p, CAPSTYPE_INPUT);
	/* lengthCapability */
	R2V_PACKET_WRITE_UINT16_LE(p, 88);
	/* inputFlags */
	R2V_PACKET_WRITE_UINT16_LE(p, INPUT_FLAG_SCANCODES);
	/* pad2octetsA */
	R2V_PACKET_WRITE_UINT16_LE(p, 0);
	/* keyboardLayout */
	R2V_PACKET_WRITE_UINT32_LE(p, 0);
	/* keyboardType */
	R2V_PACKET_WRITE_UINT32_LE(p, 0);
	/* keyboardSubType */
	R2V_PACKET_WRITE_UINT32_LE(p, 0);
	/* keyboardFunctionKey */
	R2V_PACKET_WRITE_UINT32_LE(p, 0);
	/* imeFileName */
	R2V_PACKET_SEEK(p, 64);
}

r2v_cap_write_func r2v_cap_write_func_list[] = {
	r2v_cap_write_general_cap,
	r2v_cap_write_bitmap_cap,
	r2v_cap_write_input_cap
};

uint16_t
r2v_cap_get_write_count()
{
	return sizeof(r2v_cap_write_func_list)/sizeof(r2v_cap_write_func_list[0]);
}

void
r2v_cap_write_caps(r2v_rdp_t *r, r2v_packet_t *p)
{
	int i = 0, write_count = r2v_cap_get_write_count();
	for (i = 0; i < write_count; i++) {
		r2v_cap_write_func_list[i](r, p);
	}
}
