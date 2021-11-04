#include "device_control.h"

#ifdef __unix__

#include <unistd.h>
#include <stdio.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>

static uint16_t __key_to_ascii(uint16_t _c);

struct raw_key_event
{
	struct timeval time;
	uint16_t __type;
	uint16_t key;
	uint32_t action;
};

static Display* display;
static Window root_window;

static int mouse_fd;

static int keyboard_fd;
static uint16_t lkey = 0;

static unsigned char mousedev_imps_seq[] = { 0xf3, 200, 0xf3, 100, 0xf3, 80 };

void device_control_init(void)
{
	display = XOpenDisplay(0);
	root_window = XRootWindow(display, 0);

	mouse_fd = open("/dev/input/by-id/usb-Tablet_PTK-640-mouse", O_RDWR);
	write(mouse_fd, mousedev_imps_seq, 6);
	int flags = fcntl(mouse_fd, F_GETFL, 0);
	fcntl(mouse_fd, F_SETFL, flags | O_NONBLOCK);

	keyboard_fd = open("/dev/input/event3", O_RDONLY | O_NONBLOCK);
	flags = fcntl(mouse_fd, F_GETFL, 0);
	fcntl(mouse_fd, F_SETFL, flags | O_NONBLOCK);
}

void device_control_keyboard_disable(void)
{
	FILE* fp = popen("xinput --disable 10 && xinput --disable 17", "r");
	pclose(fp);
}

void device_control_keyboard_enable(void)
{
	FILE* fp = popen("xinput --enable 10 && xinput --enable 17", "r");
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

struct key_event device_control_get_keyboard_event(void) {
	struct raw_key_event ev[MAX_OVER_READ] = {0};
	int bytes = read(keyboard_fd, ev, sizeof(ev));
	if (bytes == -1) return (struct key_event){0};
	ev[1].key = __key_to_ascii(ev[1].key);
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

static uint16_t __key_to_ascii(uint16_t _c) {
	switch (_c) {
		case KEY_RESERVED            : return 0x00 ;
		case KEY_ESC                : return XK_Escape ;
		case KEY_1                    : return XK_1 ;
		case KEY_2                    : return XK_2 ;
		case KEY_3                    : return XK_3 ;
		case KEY_4                    : return XK_4 ;
		case KEY_5                    : return XK_5 ;
		case KEY_6                    : return XK_6 ;
		case KEY_7                    : return XK_7 ;
		case KEY_8                    : return XK_8 ;
		case KEY_9                    : return XK_9 ;
		case KEY_0                    : return XK_0 ;
		case KEY_MINUS                : return XK_minus ;
		case KEY_EQUAL                : return XK_equal ;
		case KEY_BACKSPACE            : return XK_BackSpace ;
		case KEY_TAB                : return XK_Tab ;
		case KEY_Q                    : return XK_q ;
		case KEY_W                    : return XK_w ;
		case KEY_E                    : return XK_e ;
		case KEY_R                    : return XK_r ;
		case KEY_T                    : return XK_t ;
		case KEY_Y                    : return XK_y ;
		case KEY_U                    : return XK_u ;
		case KEY_I                    : return XK_i ;
		case KEY_O                    : return XK_o ;
		case KEY_P                    : return XK_p ;
		case KEY_LEFTBRACE            : return XK_bracketleft ;
		case KEY_RIGHTBRACE            : return XK_bracketright ;
		case KEY_ENTER                : return XK_Return ;
		case KEY_A                    : return XK_a ;
		case KEY_S                    : return XK_s ;
		case KEY_D                    : return XK_d ;
		case KEY_F                    : return XK_f ;
		case KEY_G                    : return XK_g ;
		case KEY_H                    : return XK_h ;
		case KEY_J                    : return XK_j ;
		case KEY_K                    : return XK_k ;
		case KEY_L                    : return XK_l ;
		case KEY_SEMICOLON            : return XK_semicolon ;
		case KEY_APOSTROPHE            : return XK_apostrophe ;
		case KEY_GRAVE                : return XK_grave ;
		case KEY_BACKSLASH            : return XK_backslash ;
		case KEY_Z                    : return XK_z ;
		case KEY_X                    : return XK_x ;
		case KEY_C                    : return XK_c ;
		case KEY_V                    : return XK_v ;
		case KEY_B                    : return XK_b ;
		case KEY_N                    : return XK_n ;
		case KEY_M                    : return XK_m ;
		case KEY_COMMA                : return XK_comma ;
		case KEY_DOT                : return XK_period ;
		case KEY_SLASH                : return XK_slash ;
		case KEY_SPACE                : return XK_space ;
		case KEY_LEFTCTRL            : return XK_Control_L ;
		case KEY_LEFTSHIFT            : return XK_Shift_L ;
		case KEY_RIGHTSHIFT            : return XK_Shift_R ;
		case KEY_KPASTERISK            : return XK_asterisk ;
		case KEY_LEFTALT            : return XK_Alt_L ;
		case KEY_CAPSLOCK            : return XK_Caps_Lock ;
		case KEY_F1                    : return XK_F1 ;
		case KEY_F2                    : return XK_F2 ;
		case KEY_F3                    : return XK_F3 ;
		case KEY_F4                    : return XK_F4 ;
		case KEY_F5                    : return XK_F5 ;
		case KEY_F6                    : return XK_F6 ;
		case KEY_F7                    : return XK_F7 ;
		case KEY_F8                    : return XK_F8 ;
		case KEY_F9                    : return XK_F9 ;
		case KEY_F10                : return XK_F10 ;
		case KEY_NUMLOCK            : return XK_Num_Lock ;
		case KEY_SCROLLLOCK            : return XK_Scroll_Lock ;
		case KEY_KP7                : return XK_KP_7 ;
		case KEY_KP8                : return XK_KP_8 ;
		case KEY_KP9                : return XK_KP_9 ;
		case KEY_KPMINUS            : return XK_minus ;
		case KEY_KP4                : return XK_KP_4 ;
		case KEY_KP5                : return XK_KP_5 ;
		case KEY_KP6                : return XK_KP_6 ;
		case KEY_KPPLUS                : return XK_plus ;
		case KEY_KP1                : return XK_KP_1 ;
		case KEY_KP2                : return XK_KP_2 ;
		case KEY_KP3                : return XK_KP_3 ;
		case KEY_KP0                : return XK_KP_0 ;
		case KEY_KPDOT                : return XK_period ;
//        case KEY_ZENKAKUHANKAKU        : return XK_ZENKAKUHANKAKU ;
//        case KEY_102ND                : return XK_102ND ;
		case KEY_F11                : return XK_F11 ;
		case KEY_F12                : return XK_F12 ;
//        case KEY_RO                    : return XK_RO ;
		case KEY_RIGHTALT            : return XK_Alt_R ;
		case KEY_LINEFEED            : return XK_Linefeed ;
		case KEY_HOME                : return XK_Home ;
		case KEY_UP                    : return XK_Up ;
		case KEY_PAGEUP                : return XK_Page_Up ;
		case KEY_LEFT                : return XK_Left ;
		case KEY_RIGHT                : return XK_Right ;
		case KEY_END                : return XK_End ;
		case KEY_DOWN                : return XK_Down ;
		case KEY_PAGEDOWN            : return XK_Page_Down ;
		case KEY_INSERT                : return XK_Insert ;
		case KEY_DELETE                : return XK_Delete ;
		case KEY_KPEQUAL            : return XK_equal ;
		case KEY_LEFTMETA            : return XK_Super_L ;
		case KEY_RIGHTMETA            : return XK_Super_R ;
		case KEY_F13                : return XK_F13 ;
		case KEY_F14                : return XK_F14 ;
		case KEY_F15                : return XK_F15 ;
		case KEY_F16                : return XK_F16 ;
		case KEY_F17                : return XK_F17 ;
		case KEY_F18                : return XK_F18 ;
		case KEY_F19                : return XK_F19 ;
		case KEY_F20                : return XK_F20 ;
		case KEY_F21                : return XK_F21 ;
		case KEY_F22                : return XK_F22 ;
		case KEY_F23                : return XK_F23 ;
		case KEY_F24                : return XK_F24 ;
		default: return _c;
	}
}

#endif