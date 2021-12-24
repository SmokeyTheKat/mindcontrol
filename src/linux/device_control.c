#include "device_control.h"

#ifdef __unix__

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>
#include <fcntl.h>
#include <linux/input.h>
#include <limits.h>
#include <string.h>
#include <X11/Xmu/Atoms.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mcerror.h"
#include "utils.h"
#include "clipboard.h"

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

static unsigned char mousedev_imps_seq[] = { 0xf3, 200, 0xf3, 100, 0xf3, 80 };

#define MOUSE_X_NAME "Logitech M720 Triathlon"
#define KEYBOARD_X_NAME "Kingston HyperX Alloy FPS Pro Mechanical Gaming Keyboard"


void device_control_init(void)
{
	init_clipboard();
	display = XOpenDisplay(0);
	root_window = XRootWindow(display, 0);

	mouse_fd = open("/dev/input/mice", O_RDWR);
	write(mouse_fd, mousedev_imps_seq, 6);
	int flags = fcntl(mouse_fd, F_GETFL, 0);
	fcntl(mouse_fd, F_SETFL, flags | O_NONBLOCK);

	char keyboard_event_path[1024] = {0};
	load_shell_command("find /dev/input/ | grep 'event-kbd' | tac | head -n1", keyboard_event_path, sizeof(keyboard_event_path));
	keyboard_fd = open(keyboard_event_path, O_RDONLY | O_NONBLOCK);

	flags = fcntl(mouse_fd, F_GETFL, 0);
	fcntl(mouse_fd, F_SETFL, flags | O_NONBLOCK);

	screen_size = device_control_get_screen_size();
}

char* device_control_get_hostname(void)
{
	static char* hostname_out = 0;

	if (hostname_out != 0) return hostname_out;

	static char hostname_buffer[256];

	gethostname(hostname_buffer, sizeof(hostname_buffer));

	hostname_out = hostname_buffer;
	return hostname_out;
}

char* device_control_get_ip(void)
{
	static char* ip_out = 0;

	if (ip_out != 0) return ip_out;

	static char ip_buffer[95];

	load_shell_command("ip a | grep -Eo 'inet (addr:)?([0-9]*\\.){3}[0-9]*' | grep -Eo '([0-9]*\\.){3}[0-9]*' | grep -v '127.0.0.1'",
					   ip_buffer, sizeof(ip_out));

	ip_out = ip_buffer;
	return ip_out;
}

char* device_control_get_ip_by_hostname(void)
{
	static char* ip_out = 0;

	if (ip_out != 0) return ip_out;

	char hostname_buffer[256];

	int hostname = gethostname(hostname_buffer, sizeof(hostname_buffer));
	if (hostname == -1)
		mcerror("could not get hostname\n", 0);

	struct hostent* host_entry = gethostbyname(hostname_buffer);
	if (host_entry == 0)
		mcerror("could not get hostname\n", 0);

	char* ip_str = inet_ntoa(*((struct in_addr*)
						host_entry->h_addr_list[0]));

	ip_out = ip_str;
	return ip_out;
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
	FILE* fp = popen("xinput | grep '" KEYBOARD_X_NAME "' | grep -o 'id=[0-9]\\?[0-9]' | sed 's/id=//g' | xargs -I{} xinput disable {} && \
					  xinput | grep '" MOUSE_X_NAME "' | grep -o 'id=[0-9]\\?[0-9]' | sed 's/id=//g' | xargs -I{} xinput disable {}", "r");
	pclose(fp);
}

void device_control_keyboard_enable(void)
{
	FILE* fp = popen("xinput | grep '" KEYBOARD_X_NAME "' | grep -o 'id=[0-9]\\?[0-9]' | sed 's/id=//g' | xargs -I{} xinput enable {} && \
					  xinput | grep '" MOUSE_X_NAME "' | grep -o 'id=[0-9]\\?[0-9]' | sed 's/id=//g' | xargs -I{} xinput enable {}", "r");
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

void device_control_keyboard_flush(void)
{
	char buffer[128];
	while (read(keyboard_fd, buffer, sizeof(buffer)) > 0);
}

void device_control_mouse_flush(void)
{
	char buffer[128];
	while (read(mouse_fd, buffer, sizeof(buffer)) > 0);
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
	static uint16_t lkey = 0;

	struct raw_key_event ev = {0};
	read(keyboard_fd, &ev, sizeof(ev));

	ev.key = key_code_to_generic_code(ev.key);

	if (ev.key) lkey = ev.key;
	else ev.key = lkey;

	if (ev.__type == 1)
	{
		return (struct key_event){
			.ready=true,
			.key=ev.key,
			.action=ev.action,
		};
	}
	else return (struct key_event){0};
}

char* device_control_clipboard_get(void)
{
	return clipboard_get();
}
void device_control_clipboard_set(char* data)
{
	clipboard_set(data);
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

#endif