#include "device_control.h"

#ifdef __unix__

#include <unistd.h>
#include <stdio.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>
#include <fcntl.h>
#include <linux/input.h>

struct raw_key_event
{
	struct timeval time;
	uint16_t __type;
	uint16_t key;
	uint32_t action;
};

static Display* display;
static Window root_window;

struct vec screen_size;

static int mouse_fd;

static int keyboard_fd;
static uint16_t lkey = 0;

static unsigned char mousedev_imps_seq[] = { 0xf3, 200, 0xf3, 100, 0xf3, 80 };

void device_control_init(void)
{
	display = XOpenDisplay(0);
	root_window = XRootWindow(display, 0);

	mouse_fd = open("/dev/input/mice", O_RDWR);
	write(mouse_fd, mousedev_imps_seq, 6);
	int flags = fcntl(mouse_fd, F_GETFL, 0);
	fcntl(mouse_fd, F_SETFL, flags | O_NONBLOCK);

	keyboard_fd = open("/dev/input/by-id/usb-Kingston_HyperX_Alloy_FPS_Pro_Mechanical_Gaming_Keyboard-event-kbd", O_RDONLY | O_NONBLOCK);
	flags = fcntl(mouse_fd, F_GETFL, 0);
	fcntl(mouse_fd, F_SETFL, flags | O_NONBLOCK);

	screen_size = device_control_get_screen_size();
}

struct vec device_control_get_screen_size(void)
{
	int default_screen = DefaultScreen(display);
	struct vec size;
	size.x = DisplayWidth(display, default_screen);
	size.y = DisplayHeight(display, default_screen);
	return size;
}

void device_control_keyboard_disable(void)
{
	FILE* fp = popen("xinput --disable 10 && xinput --disable 9", "r");
	pclose(fp);
}

void device_control_keyboard_enable(void)
{
	FILE* fp = popen("xinput --enable 10 && xinput --enable 9", "r");
	pclose(fp);
}

void device_control_cursor_move(int x, int y)
{
	XSelectInput(display, root_window, KeyReleaseMask);
	XWarpPointer(display, None, None, 0, 0, 0, 0, x, y);
	XFlush(display);
}

void device_control_cursor_move_to(int x, int y)
{
	XSelectInput(display, root_window, KeyReleaseMask);
	XWarpPointer(display, None, root_window, 0, 0, 0, 0, x, y);
	XFlush(display);
}

void device_control_cursor_left_down(void)
{
	XTestFakeButtonEvent(display, Button1, true, 0);
	XFlush(display);
}

void device_control_cursor_left_up(void)
{
	XTestFakeButtonEvent(display, Button1, false, 0);
	XFlush(display);
}

void device_control_cursor_right_down(void)
{
	XTestFakeButtonEvent(display, Button3, true, 0);
	XFlush(display);
}

void device_control_cursor_right_up(void)
{
	XTestFakeButtonEvent(display, Button3, false, 0);
	XFlush(display);
}

void device_control_keyboard_send_press(int keycode)
{
	keycode = XKeysymToKeycode(display, keycode);
	XTestFakeKeyEvent(display, keycode, True, 0);
	XFlush(display);
}

void device_control_keyboard_send_release(int keycode)
{
	keycode = XKeysymToKeycode(display, keycode);
	XTestFakeKeyEvent(display, keycode, False, 0);
	XFlush(display);
}

void device_control_keyboard_send(int keycode)
{
	keycode = XKeysymToKeycode(display, keycode);
	XTestFakeKeyEvent(display, keycode, True, 0);
	XTestFakeKeyEvent(display, keycode, False, 0);
	XFlush(display);
}

void device_control_cursor_scroll(int dir)
{
	if (dir == 1)
	{
		XTestFakeButtonEvent(display, Button5, true, 0);
		XFlush(display);
		usleep(10000);
		XTestFakeButtonEvent(display, Button5, false, 0);
		XFlush(display);
	}
	else if (dir == -1)
	{
		XTestFakeButtonEvent(display, Button4, true, 0);
		XFlush(display);
		usleep(10000);
		XTestFakeButtonEvent(display, Button4, false, 0);
		XFlush(display);
	}
}

struct vec device_control_cursor_get(void)
{
	struct vec pos;
	XQueryPointer(display, root_window, &(Window){0}, &(Window){0},
				  &pos.x, &pos.y, &(int){0}, &(int){0}, &(unsigned int){0});
	return pos;
}

void device_control_init_events(void)
{
}

struct vec device_control_cursor_on_move_get(void)
{
	struct vec pos = device_control_cursor_get();
	while (1)
	{
		struct vec new_pos = device_control_cursor_get();
		if (!vec_compare(pos, new_pos)) return new_pos;
	}
}

struct vec device_control_cursor_on_move_get_relative(void)
{
	struct vec pos = device_control_cursor_get();
	while (1)
	{
		struct vec new_pos = device_control_cursor_get();
		if (!vec_compare(pos, new_pos)) return vec_sub(new_pos, pos);
	}
}

struct mouse_state device_control_get_mouse_state(void)
{
	unsigned char mice_data[4];
	if (read(mouse_fd, mice_data, 4) == -1)
		return (struct mouse_state){0};
	return (struct mouse_state){
		.ready=true,
		.x=(char)mice_data[1],
		.y=-(char)mice_data[2],
		.scroll=(char)mice_data[3],
		.left=mice_data[0] & 0x1,
		.right=mice_data[0] & 0x2,
		.middle=mice_data[0] & 0x4,
	};
}

struct key_event device_control_get_keyboard_event(void)
{
	struct raw_key_event ev[MAX_OVER_READ] = {0};
	int bytes = read(keyboard_fd, ev, sizeof(ev));
	if (bytes == -1) return (struct key_event){0};
	ev[1].key = key_code_to_generic_code(ev[1].key);
	if (ev[1].key) lkey = ev[1].key;
	else ev[1].key = lkey;
	if (ev[1].__type == 1) return (struct key_event){
		.ready=true,
		.key=ev[1].key,
		.action=ev[1].action,
	};
	return (struct key_event){
		.ready=true,
		.key=lkey,
		.action=KEY_PRESS
	};
}

uint16_t key_code_to_generic_code(uint16_t _c)
{
	switch (_c) {
		case KEY_RESERVED            : return 1;
		case KEY_ESC                : return 2;
		case KEY_1                    : return 3;
		case KEY_2                    : return 4;
		case KEY_3                    : return 5;
		case KEY_4                    : return 6;
		case KEY_5                    : return 7;
		case KEY_6                    : return 8;
		case KEY_7                    : return 9;
		case KEY_8                    : return 10;
		case KEY_9                    : return 11;
		case KEY_0                    : return 12;
		case KEY_MINUS                : return 13;
		case KEY_EQUAL                : return 14;
		case KEY_BACKSPACE            : return 15;
		case KEY_TAB                : return 16;
		case KEY_Q                    : return 17;
		case KEY_W                    : return 18;
		case KEY_E                    : return 19;
		case KEY_R                    : return 20;
		case KEY_T                    : return 21;
		case KEY_Y                    : return 22;
		case KEY_U                    : return 23;
		case KEY_I                    : return 24;
		case KEY_O                    : return 25;
		case KEY_P                    : return 26;
		case KEY_LEFTBRACE            : return 27;
		case KEY_RIGHTBRACE            : return 28;
		case KEY_ENTER                : return 29;
		case KEY_A                    : return 30;
		case KEY_S                    : return 31;
		case KEY_D                    : return 32;
		case KEY_F                    : return 33;
		case KEY_G                    : return 34;
		case KEY_H                    : return 35;
		case KEY_J                    : return 36;
		case KEY_K                    : return 37;
		case KEY_L                    : return 38;
		case KEY_SEMICOLON            : return 39;
		case KEY_APOSTROPHE            : return 40;
		case KEY_GRAVE                : return 41;
		case KEY_BACKSLASH            : return 42;
		case KEY_Z                    : return 43;
		case KEY_X                    : return 44;
		case KEY_C                    : return 45;
		case KEY_V                    : return 46;
		case KEY_B                    : return 47;
		case KEY_N                    : return 48;
		case KEY_M                    : return 49;
		case KEY_COMMA                : return 50;
		case KEY_DOT                : return 51;
		case KEY_SLASH                : return 52;
		case KEY_SPACE                : return 53;
		case KEY_LEFTCTRL            : return 54;
		case KEY_LEFTSHIFT            : return 55;
		case KEY_RIGHTSHIFT            : return 56;
		case KEY_KPASTERISK            : return 57;
		case KEY_LEFTALT            : return 58;
		case KEY_CAPSLOCK            : return 59;
		case KEY_F1                    : return 60;
		case KEY_F2                    : return 61;
		case KEY_F3                    : return 62;
		case KEY_F4                    : return 63;
		case KEY_F5                    : return 64;
		case KEY_F6                    : return 65;
		case KEY_F7                    : return 66;
		case KEY_F8                    : return 67;
		case KEY_F9                    : return 68;
		case KEY_F10                : return 69;
		case KEY_NUMLOCK            : return 70;
		case KEY_SCROLLLOCK            : return 71;
		case KEY_KP7                : return 72;
		case KEY_KP8                : return 73;
		case KEY_KP9                : return 74;
		case KEY_KPMINUS            : return 75;
		case KEY_KP4                : return 76;
		case KEY_KP5                : return 77;
		case KEY_KP6                : return 78;
		case KEY_KPPLUS                : return 79;
		case KEY_KP1                : return 80;
		case KEY_KP2                : return 81;
		case KEY_KP3                : return 82;
		case KEY_KP0                : return 83;
		case KEY_KPDOT                : return 84;
		case KEY_F11                : return 85;
		case KEY_F12                : return 86;
		case KEY_RIGHTALT            : return 87;
		case KEY_LINEFEED            : return 88;
		case KEY_HOME                : return 89;
		case KEY_UP                    : return 90;
		case KEY_PAGEUP                : return 91;
		case KEY_LEFT                : return 92;
		case KEY_RIGHT                : return 93;
		case KEY_END                : return 94;
		case KEY_DOWN                : return 95;
		case KEY_PAGEDOWN            : return 96;
		case KEY_INSERT                : return 97;
		case KEY_DELETE                : return 98;
		case KEY_KPEQUAL            : return 99;
		case KEY_LEFTMETA            : return 100;
		case KEY_RIGHTMETA            : return 101;
		case KEY_F13                : return 102;
		case KEY_F14                : return 103;
		case KEY_F15                : return 104;
		case KEY_F16                : return 105;
		case KEY_F17                : return 106;
		case KEY_F18                : return 107;
		case KEY_F19                : return 108;
		case KEY_F20                : return 109;
		case KEY_F21                : return 110;
		case KEY_F22                : return 111;
		case KEY_F23                : return 112;
		case KEY_F24                : return 113;
		default: return _c;
	}
}

uint16_t generic_code_to_system_code(uint16_t _c)
{
	switch (_c)
	{
		case 1: return 0x00 ;
		case 2: return XK_Escape ;
		case 3: return XK_1 ;
		case 4: return XK_2 ;
		case 5: return XK_3 ;
		case 6: return XK_4 ;
		case 7: return XK_5 ;
		case 8: return XK_6 ;
		case 9: return XK_7 ;
		case 10: return XK_8 ;
		case 11: return XK_9 ;
		case 12: return XK_0 ;
		case 13: return XK_minus ;
		case 14: return XK_equal ;
		case 15: return XK_BackSpace ;
		case 16: return XK_Tab ;
		case 17: return XK_q ;
		case 18: return XK_w ;
		case 19: return XK_e ;
		case 20: return XK_r ;
		case 21: return XK_t ;
		case 22: return XK_y ;
		case 23: return XK_u ;
		case 24: return XK_i ;
		case 25: return XK_o ;
		case 26: return XK_p ;
		case 27: return XK_bracketleft ;
		case 28: return XK_bracketright ;
		case 29: return XK_Return ;
		case 30: return XK_a ;
		case 31: return XK_s ;
		case 32: return XK_d ;
		case 33: return XK_f ;
		case 34: return XK_g ;
		case 35: return XK_h ;
		case 36: return XK_j ;
		case 37: return XK_k ;
		case 38: return XK_l ;
		case 39: return XK_semicolon ;
		case 40: return XK_apostrophe ;
		case 41: return XK_grave ;
		case 42: return XK_backslash ;
		case 43: return XK_z ;
		case 44: return XK_x ;
		case 45: return XK_c ;
		case 46: return XK_v ;
		case 47: return XK_b ;
		case 48: return XK_n ;
		case 49: return XK_m ;
		case 50: return XK_comma ;
		case 51: return XK_period ;
		case 52: return XK_slash ;
		case 53: return XK_space ;
		case 54: return XK_Control_L ;
		case 55: return XK_Shift_L ;
		case 56: return XK_Shift_R ;
		case 57: return XK_asterisk ;
		case 58: return XK_Alt_L ;
		case 59: return XK_Caps_Lock ;
		case 60: return XK_F1 ;
		case 61: return XK_F2 ;
		case 62: return XK_F3 ;
		case 63: return XK_F4 ;
		case 64: return XK_F5 ;
		case 65: return XK_F6 ;
		case 66: return XK_F7 ;
		case 67: return XK_F8 ;
		case 68: return XK_F9 ;
		case 69: return XK_F10 ;
		case 70: return XK_Num_Lock ;
		case 71: return XK_Scroll_Lock ;
		case 72: return XK_KP_7 ;
		case 73: return XK_KP_8 ;
		case 74: return XK_KP_9 ;
		case 75: return XK_minus ;
		case 76: return XK_KP_4 ;
		case 77: return XK_KP_5 ;
		case 78: return XK_KP_6 ;
		case 79: return XK_plus ;
		case 80: return XK_KP_1 ;
		case 81: return XK_KP_2 ;
		case 82: return XK_KP_3 ;
		case 83: return XK_KP_0 ;
		case 84: return XK_period ;
		case 85: return XK_F11 ;
		case 86: return XK_F12 ;
		case 87: return XK_Alt_R ;
		case 88: return XK_Linefeed ;
		case 89: return XK_Home ;
		case 90: return XK_Up ;
		case 91: return XK_Page_Up ;
		case 92: return XK_Left ;
		case 93: return XK_Right ;
		case 94: return XK_End ;
		case 95: return XK_Down ;
		case 96: return XK_Page_Down ;
		case 97: return XK_Insert ;
		case 98: return XK_Delete ;
		case 99: return XK_equal ;
		case 100: return XK_Super_L ;
		case 101: return XK_Super_R ;
		case 102: return XK_F13 ;
		case 103: return XK_F14 ;
		case 104: return XK_F15 ;
		case 105: return XK_F16 ;
		case 106: return XK_F17 ;
		case 107: return XK_F18 ;
		case 108: return XK_F19 ;
		case 109: return XK_F20 ;
		case 110: return XK_F21 ;
		case 111: return XK_F22 ;
		case 112: return XK_F23 ;
		case 113: return XK_F24 ;
		default: return _c;
	}
}







/*
VK_LBUTTON
0x01

	Left mouse button

VK_RBUTTON
0x02

	Right mouse button

VK_CANCEL
0x03

	Control-break processing

VK_MBUTTON
0x04

	Middle mouse button (three-button mouse)

VK_XBUTTON1
0x05

	X1 mouse button

VK_XBUTTON2
0x06

	X2 mouse button

-
0x07

	Undefined

VK_BACK
0x08

	BACKSPACE key

VK_TAB
0x09

	TAB key

-
0x0A-0B

	Reserved

VK_CLEAR
0x0C

	CLEAR key

VK_RETURN
0x0D

	ENTER key

-
0x0E-0F

	Undefined

VK_SHIFT
0x10

	SHIFT key

VK_CONTROL
0x11

	CTRL key

VK_MENU
0x12

	ALT key

VK_PAUSE
0x13

	PAUSE key

VK_CAPITAL
0x14

	CAPS LOCK key

VK_KANA
0x15

	IME Kana mode

VK_HANGUEL
0x15

	IME Hanguel mode (maintained for compatibility; use VK_HANGUL)

VK_HANGUL
0x15

	IME Hangul mode

VK_IME_ON
0x16

	IME On

VK_JUNJA
0x17

	IME Junja mode

VK_FINAL
0x18

	IME final mode

VK_HANJA
0x19

	IME Hanja mode

VK_KANJI
0x19

	IME Kanji mode

VK_IME_OFF
0x1A

	IME Off

VK_ESCAPE
0x1B

	ESC key

VK_CONVERT
0x1C

	IME convert

VK_NONCONVERT
0x1D

	IME nonconvert

VK_ACCEPT
0x1E

	IME accept

VK_MODECHANGE
0x1F

	IME mode change request

VK_SPACE
0x20

	SPACEBAR

VK_PRIOR
0x21

	PAGE UP key

VK_NEXT
0x22

	PAGE DOWN key

VK_END
0x23

	END key

VK_HOME
0x24

	HOME key

VK_LEFT
0x25

	LEFT ARROW key

VK_UP
0x26

	UP ARROW key

VK_RIGHT
0x27

	RIGHT ARROW key

VK_DOWN
0x28

	DOWN ARROW key

VK_SELECT
0x29

	SELECT key

VK_PRINT
0x2A

	PRINT key

VK_EXECUTE
0x2B

	EXECUTE key

VK_SNAPSHOT
0x2C

	PRINT SCREEN key

VK_INSERT
0x2D

	INS key

VK_DELETE
0x2E

	DEL key

VK_HELP
0x2F

	HELP key

0x30

	0 key

0x31

	1 key

0x32

	2 key

0x33

	3 key

0x34

	4 key

0x35

	5 key

0x36

	6 key

0x37

	7 key

0x38

	8 key

0x39

	9 key

-
0x3A-40

	Undefined

0x41

	A key

0x42

	B key

0x43

	C key

0x44

	D key

0x45

	E key

0x46

	F key

0x47

	G key

0x48

	H key

0x49

	I key

0x4A

	J key

0x4B

	K key

0x4C

	L key

0x4D

	M key

0x4E

	N key

0x4F

	O key

0x50

	P key

0x51

	Q key

0x52

	R key

0x53

	S key

0x54

	T key

0x55

	U key

0x56

	V key

0x57

	W key

0x58

	X key

0x59

	Y key

0x5A

	Z key

VK_LWIN
0x5B

	Left Windows key (Natural keyboard)

VK_RWIN
0x5C

	Right Windows key (Natural keyboard)

VK_SLEEP
0x5F

	Computer Sleep key

VK_NUMPAD0
0x60

	Numeric keypad 0 key

VK_NUMPAD1
0x61

	Numeric keypad 1 key

VK_NUMPAD2
0x62

	Numeric keypad 2 key

VK_NUMPAD3
0x63

	Numeric keypad 3 key

VK_NUMPAD4
0x64

	Numeric keypad 4 key

VK_NUMPAD5
0x65

	Numeric keypad 5 key

VK_NUMPAD6
0x66

	Numeric keypad 6 key

VK_NUMPAD7
0x67

	Numeric keypad 7 key

VK_NUMPAD8
0x68

	Numeric keypad 8 key

VK_NUMPAD9
0x69

	Numeric keypad 9 key

VK_MULTIPLY
0x6A

	Multiply key

VK_ADD
0x6B

	Add key

VK_SEPARATOR
0x6C

	Separator key

VK_SUBTRACT
0x6D

	Subtract key

VK_DECIMAL
0x6E

	Decimal key

VK_DIVIDE
0x6F

	Divide key

-
VK_NUMLOCK
0x90

	NUM LOCK key

VK_SCROLL
0x91

	SCROLL LOCK key
VK_RCONTROL
0xA3

	Right CONTROL key

VK_LMENU
0xA4

	Left MENU key

VK_RMENU
0xA5

	Right MENU key
*/

#endif