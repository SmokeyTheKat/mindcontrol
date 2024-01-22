#include <mindcontrol/device_control.h>

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
#include <sys/select.h>

#include <X11/Intrinsic.h>
#include <X11/extensions/sync.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>

#include <mindcontrol/mcerror.h>
#include <mindcontrol/utils.h>
#include <mindcontrol/input.h>
#include <mindcontrol/list.h>
#include <mindcontrol/gui.h>

struct raw_key_event {
	struct timeval time;
	uint16_t type;
	uint16_t key;
	uint32_t action;
};

struct xdevice {
	char name[128];
	char path[256];
	int eid;
	int xid;
	int fd;
};

static void load_input_data(void);

struct vec g_screen_size;

static Display* display;
static Window root_window;

static unsigned char mousedev_imps_seq[] = { 0xf3, 200, 0xf3, 100, 0xf3, 80 };

static struct xdevice keyboard = {0};
static struct xdevice mouse = {0};

static int cursor_is_hiding = 0;

static struct mouse_state s_mouse_state;

static char s_clipboard_buffer[16384] = {0};

void init_dragdrop(void) {}
struct list* get_dragdrop_files(void) { return 0; }

void device_control_init(void) {
	display = XOpenDisplay(0);
	root_window = XRootWindow(display, 0);

	g_screen_size = device_control_get_screen_size();

	load_input_data();

	mouse.fd = open(mouse.path, O_RDWR);
	write(mouse.fd, mousedev_imps_seq, 6);

	keyboard.fd = open(keyboard.path, O_RDONLY);
}

void device_control_cleanup(void) {
}

bool device_control_is_sleeping(void) {
	return false;
}

char* device_control_get_hostname(void) {
	static char hostname_buffer[256];
	static char* hostname_out = 0;
	if (hostname_out != 0) return hostname_out;

	gethostname(hostname_buffer, sizeof(hostname_buffer));
	hostname_out = hostname_buffer;
	return hostname_out;
}

char* device_control_get_ip(void) {
	static char* ip_out = 0;
	if (ip_out != 0) return ip_out;

	char hostname_buffer[256];

	int hostname = gethostname(hostname_buffer, sizeof(hostname_buffer));
	if (hostname == -1) {
		mcerror("could not get hostname\n", 0);
		return "0.0.0.0";
	}

	struct hostent* host_entry = gethostbyname(hostname_buffer);
	if (host_entry == 0) {
		mcerror("could not get hostname\n", 0);
		return "0.0.0.0";
	}

	char* ip_str = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
	ip_out = ip_str;
	return ip_out;
}

struct vec device_control_get_screen_size(void) {
	int default_screen = DefaultScreen(display);
	return (struct vec){
		.x = DisplayWidth(display, default_screen),
		.y = DisplayHeight(display, default_screen)
	};
}

void device_control_disable_input(void) {
	char buffer[1024];
	sprintf(
		buffer,
		"xinput disable %d && for i in $(xinput list | grep '%s' | grep -o 'id=[0-9]*' | cut -c 4-); do xinput disable $i; done",
		mouse.xid,
		keyboard.name
	);
	FILE* fp = popen(buffer, "r");
	pclose(fp);
	device_control_mouse_cursor_hide();
}

void device_control_enable_input(void) {
	char buffer[1024];
	sprintf(
		buffer,
		"xinput enable %d && for i in $(xinput list | grep '%s' | grep -o 'id=[0-9]*' | cut -c 4-); do xinput enable $i; done",
		mouse.xid,
		keyboard.name
	);
	FILE* fp = popen(buffer, "r");
	pclose(fp);
	device_control_mouse_cursor_show();
}

void device_control_mouse_move(int x, int y) {
	XSelectInput(display, root_window, KeyReleaseMask);
	XWarpPointer(display, None, None, 0, 0, 0, 0, x, y);
	XFlush(display);
}

void device_control_mouse_move_to(int x, int y) {
	XSelectInput(display, root_window, KeyReleaseMask);
	XWarpPointer(display, None, root_window, 0, 0, 0, 0, x, y);
	XFlush(display);
}

void device_control_mouse_left_down(void) {
	XTestFakeButtonEvent(display, Button1, true, 0);
	XFlush(display);
}

void device_control_mouse_left_up(void) {
	XTestFakeButtonEvent(display, Button1, false, 0);
	XFlush(display);
}

void device_control_mouse_right_down(void) {
	XTestFakeButtonEvent(display, Button3, true, 0);
	XFlush(display);
}

void device_control_mouse_right_up(void) {
	XTestFakeButtonEvent(display, Button3, false, 0);
	XFlush(display);
}

void device_control_keyboard_send_press(int keycode) {
	keycode = XKeysymToKeycode(display, keycode);
	XTestFakeKeyEvent(display, keycode, true, 0);
	XFlush(display);
}

void device_control_keyboard_send_release(int keycode) {
	keycode = XKeysymToKeycode(display, keycode);
	XTestFakeKeyEvent(display, keycode, false, 0);
	XFlush(display);
}

void device_control_keyboard_send(int keycode) {
	keycode = XKeysymToKeycode(display, keycode);
	XTestFakeKeyEvent(display, keycode, true, 0);
	XTestFakeKeyEvent(display, keycode, false, 0);
	XFlush(display);
}

void device_control_mouse_scroll(int dir) {
	if (dir == 1) {
		XTestFakeButtonEvent(display, Button5, true, 0);
		XFlush(display);
		usleep(1000);
		XTestFakeButtonEvent(display, Button5, false, 0);
		XFlush(display);
	} else if (dir == -1) {
		XTestFakeButtonEvent(display, Button4, true, 0);
		XFlush(display);
		usleep(1000);
		XTestFakeButtonEvent(display, Button4, false, 0);
		XFlush(display);
	}
}

struct vec device_control_mouse_get_position(void) {
	struct vec pos;
	XQueryPointer(
		display, root_window,
		&(Window){0}, &(Window){0},
		&pos.x, &pos.y,
		&(int){0}, &(int){0},
		&(unsigned int){0}
	);
	return pos;
}

void device_control_keyboard_flush(void) {
	char buffer[128];
	while (device_control_poll_keyboard_input(0)) {
		read(keyboard.fd, buffer, sizeof(buffer));
	}
}

void device_control_mouse_flush(void) {
	while (1) {
		struct mouse_state state = device_control_get_mouse_state();
		if (state.ready == false) break;
	}
}

bool device_control_poll_mouse_input(long long delay) {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(mouse.fd, &fds);

	struct timeval delay_tv = {
		.tv_sec = delay / 1000,
		.tv_usec = (delay % 1000) * 1000,
	};

	return (
		select(
			mouse.fd + 1,
			&fds, NULL, NULL, &delay_tv
		) > 0
	);
}

bool device_control_poll_keyboard_input(long long delay) {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(keyboard.fd, &fds);

	struct timeval delay_tv = {
		.tv_sec = delay / 1000,
		.tv_usec = (delay % 1000) * 1000,
	};

	return (
		select(
			keyboard.fd + 1,
			&fds, NULL, NULL, &delay_tv
		) > 0
	);
}

bool device_control_poll_input(long long delay) {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(mouse.fd, &fds);
	FD_SET(keyboard.fd, &fds);

	struct timeval delay_tv = {
		.tv_sec = delay / 1000,
		.tv_usec = (delay % 1000) * 1000,
	};

	return (
		select(
			MAX(mouse.fd, keyboard.fd) + 1,
			&fds, NULL, NULL, &delay_tv
		) > 0
	);
}

struct mouse_event device_control_get_mouse_event(void) {
	struct mouse_event e = {0};

	struct mouse_state old_state = s_mouse_state;
	struct mouse_state state = device_control_get_mouse_state();

	if (state.x != 0 || state.y != 0) {
		e.flags |= MOUSE_EVENT_MOVE;
		e.move.vel = (struct vec){
			.x = state.x,
			.y = state.y,
		};
	}

	if (state.scroll != 0) {
		e.flags |= MOUSE_EVENT_SCROLL;
		e.scroll.dir = state.scroll;
	}

	if (state.left != old_state.left) {
		e.flags |= (state.left == 0) ? MOUSE_EVENT_BUTTON_UP : MOUSE_EVENT_BUTTON_DOWN;
		e.button.button = MOUSE_BUTTON_LEFT;
	}

	if (state.right != old_state.right) {
		e.flags |= (state.left == 0) ? MOUSE_EVENT_BUTTON_UP : MOUSE_EVENT_BUTTON_DOWN;
		e.button.button = MOUSE_BUTTON_RIGHT;
	}

	if (state.middle != old_state.middle) {
		e.flags |= (state.left == 0) ? MOUSE_EVENT_BUTTON_UP : MOUSE_EVENT_BUTTON_DOWN;
		e.button.button = MOUSE_BUTTON_MIDDLE;
	}

	return e;
}

struct mouse_state device_control_get_mouse_state(void) {
	s_mouse_state.x = 0;
	s_mouse_state.y = 0;
	s_mouse_state.scroll = 0;
	s_mouse_state.ready = false;

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(mouse.fd, &fds);

	struct timeval delay = {
		.tv_sec = 0,
		.tv_usec = 0,
	};

	if (select(mouse.fd + 1, &fds, NULL, NULL, &delay) != 1) {
		return s_mouse_state;
	}

	unsigned char mice_data[4];
	if (read(mouse.fd, mice_data, 4) == -1) {
		return s_mouse_state;
	}

	s_mouse_state = (struct mouse_state){
		.ready = true,
		.x = (char)mice_data[1],
		.y = -(char)mice_data[2],
		.scroll = (char)mice_data[3],
		.left = mice_data[0] & 0x1,
		.right = mice_data[0] & 0x2,
		.middle = mice_data[0] & 0x4,
	};
	return s_mouse_state;
}

struct key_event device_control_get_keyboard_event(void) {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(keyboard.fd, &fds);

	struct timeval delay = {
		.tv_sec = 0,
		.tv_usec = 0,
	};

	if (select(keyboard.fd + 1, &fds, NULL, NULL, &delay) != 1) {
		return (struct key_event){0};
	}

	static uint16_t lkey = 0;

	struct raw_key_event ev = {0};
	read(keyboard.fd, &ev, sizeof(ev));

	ev.key = key_code_to_generic_code(ev.key);

	if (ev.key) lkey = ev.key;
	else ev.key = lkey;

	if (ev.type == 1) {
		return (struct key_event){
			.ready=true,
			.key=ev.key,
			.action=ev.action,
		};
	} else {
		return (struct key_event){0};
	}
}

bool device_control_clipboard_changed(void) {
	return true;
}

char* device_control_clipboard_get(void) {
//    gui_get_clipboard(s_clipboard_buffer);
//    return s_clipboard_buffer;
	return "";
}

void device_control_clipboard_set(char* data) {
//    gui_set_clipboard(data);
}

bool device_control_mouse_cursor_is_hidden(void) {
	return cursor_is_hiding;
}

void device_control_mouse_cursor_hide(void) {
	if (cursor_is_hiding) return;
	XFixesHideCursor(display, root_window);
	XFlush(display);
	cursor_is_hiding = 1;
}

void device_control_mouse_cursor_show(void) {
	if (!cursor_is_hiding) return;
	XFixesShowCursor(display, root_window);
	XFlush(display);
	cursor_is_hiding = 0;
}

static void load_input_data(void) {
	get_keyboard_name(keyboard.name);
	get_keyboard_input_path(keyboard.path);
	keyboard.eid = get_keyboard_event();

	char keyboard_xid_buffer[128] = {0};
	load_formatted_shell_command(
		"xinput | grep \"%s\" | tac | head -n1 | grep -o \"id=[0-9]*\\s\" | cut -c 4-",
		keyboard_xid_buffer,
		sizeof(keyboard_xid_buffer),
		keyboard.name
	);
	keyboard.xid = atoi(keyboard_xid_buffer);

	get_mouse_name(mouse.name);
	get_mouse_input_path(mouse.path);
	strcpy(mouse.path, "/dev/input/mice");
	mouse.eid = get_mouse_event();

	char mouse_xid_buffer[128] = {0};
	load_formatted_shell_command(
		"xinput | grep \"%s\" | grep -o \"id=[0-9]*\\s\" | cut -c 4-",
		mouse_xid_buffer,
		sizeof(mouse_xid_buffer),
		mouse.name
	);
	mouse.xid = atoi(mouse_xid_buffer);
}

uint16_t key_code_to_generic_code(uint16_t _c) {
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

uint16_t generic_code_to_system_code(uint16_t _c) {
	switch (_c) {
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

bool mouse_state_changed(struct mouse_state a, struct mouse_state b) {
	return !(
		a.x == b.x &&
		a.y == b.y &&
		a.scroll == b.scroll &&
		a.left == b.left &&
		a.right == b.right &&
		a.middle == b.middle
	);
}

#endif
