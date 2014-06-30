#include "keymaps.h"

static v2r_keymap_t keymap_us[128] = {
	{0},
	{0xff1b},
	{0x0031, 0x0021},
	{0x0032, 0x0040},
	{0x0033},
	{0x0034},
	{0x0035},
	{0x0036},
	{0x0037},
	{0x0038},
	{0x0039},
	{0x0030},
	{12},
	{13},
	{14},
	{15},
	{16},
	{17},
	{18},
	{19},
	{20},
	{21},
	{22},
	{23},
	{24},
	{25},
	{26},
	{27},
	{28},
	{29},
	{0x0061},
	{0x0073},
	{0x0064},
	{33},
	{34},
	{35},
	{36},
	{37},
	{38},
	{39},
	{40},
	{41},
	{42},
	{43},
	{44},
	{45},
	{46},
	{47},
	{48},
	{49},
	{50}
};

v2r_keymap_t *
get_keymap_by_layout(uint32_t keyboard_layout)
{
	v2r_keymap_t *k = 0;

	switch (keyboard_layout) {
	case 0x00000804:
		k = keymap_us;
		break;
	default:
		break;
	}

	return k;
}
