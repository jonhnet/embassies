#include <input/keycodes.h>
#include "KeyTranslate.h"
#include "pal_abi/pal_ui.h"

struct NitpickerToZoogMapping {
	int nitpicker;
	int zoog;
};

using namespace Input;
static NitpickerToZoogMapping nitpicker_to_zoog_mapping[] = {
	{ KEY_ESC, 96 },

	{ KEY_GRAVE, 65307 },
	{ KEY_1, 49 },
	{ KEY_2, 50 },
	{ KEY_3, 51 },
	{ KEY_4, 52 },
	{ KEY_5, 53 },
	{ KEY_6, 54 },
	{ KEY_7, 55 },
	{ KEY_8, 56 },
	{ KEY_9, 57 },
	{ KEY_0, 48 },
	{ KEY_MINUS, 91 },
	{ KEY_EQUAL, 93 },
	{ KEY_BACKSPACE, 65288 },

	{ KEY_TAB, 65289 },
	{ KEY_Q, 39 },
	{ KEY_W, 44 },
	{ KEY_E, 46 },
	{ KEY_R, 112 },
	{ KEY_T, 121 },
	{ KEY_Y, 102 },
	{ KEY_U, 103 },
	{ KEY_I, 99 },
	{ KEY_O, 114 },
	{ KEY_P, 108 },
	{ KEY_LEFTBRACE, 47 },
	{ KEY_RIGHTBRACE, 61 },
	{ KEY_BACKSLASH, 92 },

	{ KEY_CAPSLOCK, 65507 },
	{ KEY_A, 97 },
	{ KEY_S, 111 },
	{ KEY_D, 101 },
	{ KEY_F, 117 },
	{ KEY_G, 105 },
	{ KEY_H, 100 },
	{ KEY_J, 104 },
	{ KEY_K, 116 },
	{ KEY_L, 110 },
	{ KEY_SEMICOLON, 115 },
	{ KEY_APOSTROPHE, 45 },
	{ KEY_ENTER, 65293 },

	{ KEY_LEFTSHIFT, 65505 },
	{ KEY_Z, 59 },
	{ KEY_X, 113 },
	{ KEY_C, 106 },
	{ KEY_V, 107 },
	{ KEY_B, 120 },
	{ KEY_N, 98 },
	{ KEY_M, 109 },
	{ KEY_COMMA, 119 },
	{ KEY_DOT, 118 },
	{ KEY_SLASH, 122 },
	{ KEY_RIGHTSHIFT, 65506 },
	{ KEY_UP, 65362 },

	{ KEY_LEFTCTRL, 65507 },
//	{ KEY_FOO, 65515 },
	{ KEY_LEFTALT, 65513 },
	{ KEY_SPACE, 32 },
	{ KEY_RIGHTALT, 65514 },
	{ KEY_MENU, 65383 },
	{ KEY_RIGHTCTRL, 65508 },
	{ KEY_LEFT, 65361 },
	{ KEY_DOWN, 65364 },
	{ KEY_RIGHT, 65363 },

	{ BTN_LEFT, zoog_button_to_keysym(0) },
	{ BTN_RIGHT, zoog_button_to_keysym(1) },
	{ BTN_MIDDLE, zoog_button_to_keysym(2) },
};

static int nitpicker_to_zoog_mapping_count =
	sizeof(nitpicker_to_zoog_mapping)/sizeof(nitpicker_to_zoog_mapping[0]);

int KeyTranslate::map_nitpicker_key_to_zoog(int nitpicker_key)
{
	for (int i=0; i<nitpicker_to_zoog_mapping_count; i++)
	{
		if (nitpicker_key == nitpicker_to_zoog_mapping[i].nitpicker)
		{
			return nitpicker_to_zoog_mapping[i].zoog;
		}
	}
	return INVALID;
}
