#include <mindcontrol/device_control.h>

#ifdef _WIN64

#include <windows.h>
#include <winuser.h>
#include <pthread.h>
#include <powrprof.h>

#include <mindcontrol/dragdrop.h>
#include <mindcontrol/dsocket.h>
#include <mindcontrol/config.h>
#include <mindcontrol/utils.h>
#include <mindcontrol/mcerror.h>

static void stage_key_event(struct key_event ke);
static struct key_event pop_key_event(void);
static void stage_mouse_event(struct mouse_event me);
static struct mouse_event pop_mouse_event(void);
static LRESULT CALLBACK keyboard_proc(int nCode, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK mouse_proc(int nCode, WPARAM wParam, LPARAM lParam);

struct vec g_screen_size;

static struct vec s_last_mouse_pos = {-1,-1};
static bool s_last_mouse_left = false;
static bool s_last_mouse_right = false;
static bool s_last_mouse_middle = false;
static bool s_input_enabled = true;

static HHOOK s_mouse_hook;
static struct mouse_state s_mouse_state;
static struct mouse_event s_mouse_event_buffer[64];
static int s_mouse_event_buffer_pos = 0;

static HHOOK s_key_hook;
static struct key_event s_key_event_buffer[64];
static int s_key_event_buffer_pos = 0;

static char s_clipboard_buffer[16384] = {0};

void device_control_init(void) {
	SetProcessDPIAware();
	dsocket_init();
	init_dragdrop();
	g_screen_size = device_control_get_screen_size();
	s_key_hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_proc, GetModuleHandle(NULL), 0);
	s_mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, mouse_proc, GetModuleHandle(NULL), 0);
}

void device_control_cleanup(void) {
	UnhookWindowsHookEx(s_key_hook);
	UnhookWindowsHookEx(s_mouse_hook);
}

struct vec device_control_get_screen_size(void) {
	return (struct vec){
		.x = GetSystemMetrics(SM_CXSCREEN),
		.y = GetSystemMetrics(SM_CYSCREEN)
	};
}

char* device_control_get_hostname(void) {
	static char* hostname_out = 0;

	if (hostname_out != 0) return hostname_out;

	static char hostname_buffer[256];

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

void device_control_disable_input(void) {
	s_input_enabled = false;
}

void device_control_enable_input(void) {
	s_input_enabled = true;
}

void device_control_mouse_move(int x, int y) {
	INPUT ip = {0};
	ip.type = INPUT_MOUSE;
	ip.mi.mouseData = 0;
	ip.mi.dx = x;
	ip.mi.dy = y;
	ip.mi.time = 0;
	ip.mi.dwFlags = MOUSEEVENTF_MOVE;
	SendInput(1, &ip, sizeof(ip));
}

void device_control_mouse_move_to(int x, int y) {
	SetCursorPos(x, y);
	s_last_mouse_pos.x = x;
	s_last_mouse_pos.y = y;
}

void device_control_mouse_left_down(void) {
	INPUT ip = {0};
	ip.type = INPUT_MOUSE;
	ip.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	SendInput(1, &ip, sizeof(ip));
}

void device_control_mouse_left_up(void) {
	INPUT ip = {0};
	ip.type = INPUT_MOUSE;
	ip.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	SendInput(1, &ip, sizeof(ip));
}

void device_control_mouse_right_down(void) {
	INPUT ip = {0};
	ip.type = INPUT_MOUSE;
	ip.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
	SendInput(1, &ip, sizeof(ip));
}

void device_control_mouse_right_up(void) {
	INPUT ip = {0};
	ip.type = INPUT_MOUSE;
	ip.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
	SendInput(1, &ip, sizeof(ip));
}

void device_control_keyboard_send_press(int keycode) {
	INPUT ip = {0};
	ip.type = INPUT_KEYBOARD;
	ip.ki.wVk = keycode;
	ip.ki.wScan = 0;
	ip.ki.dwFlags = 0;
	ip.ki.time = 0;
	SendInput(1, &ip, sizeof(ip));
}

void device_control_keyboard_send_release(int keycode) {
	INPUT ip = {0};
	ip.type = INPUT_KEYBOARD;
	ip.ki.wVk = keycode;
	ip.ki.wScan = 0;
	ip.ki.dwFlags = 2;
	ip.ki.time = 0;
	SendInput(1, &ip, sizeof(ip));
}

void device_control_keyboard_send(int keycode) {
	device_control_keyboard_send_press(keycode);
	device_control_keyboard_send_release(keycode);
}

void device_control_mouse_scroll(int dir) {
	INPUT ip = {0};
	ip.type = INPUT_MOUSE;
	ip.mi.mouseData = -dir;
	ip.mi.dwFlags = MOUSEEVENTF_WHEEL;
	SendInput(1, &ip, sizeof(ip));
}

struct vec device_control_mouse_get_position(void) {
	POINT pos;
	GetCursorPos(&pos);
	return (struct vec){pos.x, pos.y};
}

void device_control_keyboard_flush(void) {
	s_key_event_buffer_pos = 0;
}

void device_control_mouse_flush(void) {
	s_mouse_event_buffer_pos = 0;
}

bool device_control_poll_input(long long delay) {
	return device_control_poll_mouse_input(delay) || device_control_poll_mouse_input(delay);
}

bool device_control_poll_mouse_input(long long delay) {
	return true;
}

bool device_control_poll_keyboard_input(long long delay) {
	return true;
}

struct mouse_state device_control_get_mouse_state(void) {
	return s_mouse_state;
}

struct mouse_event device_control_get_mouse_event(void) {
	return pop_mouse_event();
}

struct key_event device_control_get_keyboard_event(void) {
	return pop_key_event();
}

bool device_control_clipboard_changed(void) {
	bool changed = false;
	if (OpenClipboard(NULL)) {
		HANDLE handle = GetClipboardData(CF_TEXT);
		if (handle != NULL) {
			char* text = (char*)GlobalLock(handle);
			if (text != NULL) {
				changed = strcmp(s_clipboard_buffer, text) != 0;
				GlobalUnlock(handle);
			}
		}
		CloseClipboard();
	}
	return changed;
}

char* device_control_clipboard_get(void) {
	if (OpenClipboard(NULL)) {
		HANDLE handle = GetClipboardData(CF_TEXT);
		if (handle != NULL) {
			char* text = (char*)GlobalLock(handle);
			if (text != NULL) {
				strcpy_s(s_clipboard_buffer, sizeof(s_clipboard_buffer), text);
				GlobalUnlock(handle);
			}
		}
		CloseClipboard();
	}
	return s_clipboard_buffer;
}

void device_control_clipboard_set(char* src) {
	if (OpenClipboard(NULL)) {
		EmptyClipboard();
		HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, strlen(src) + 1);
		if (handle != NULL) {
			char* text = (char*)GlobalLock(handle);
			strcpy_s(text, strlen(src) + 1, src);
			GlobalUnlock(handle);
			SetClipboardData(CF_TEXT, handle);
		}
		CloseClipboard();
	}
}

bool device_control_is_sleeping(void) {
	BOOL sleeping;

	SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &sleeping, 0);
	return sleeping;
}

static void stage_key_event(struct key_event ke) {
	if (s_key_event_buffer_pos == ARRAY_SIZE(s_key_event_buffer)) {
		s_key_event_buffer[s_key_event_buffer_pos - 1] = ke;
	} else {
		s_key_event_buffer[s_key_event_buffer_pos++] = ke;
	}
}

static struct key_event pop_key_event(void) {
	if (s_key_event_buffer_pos > 0) {
		struct key_event ke = s_key_event_buffer[0];
		memmove(&s_key_event_buffer[0], &s_key_event_buffer[1], sizeof(s_key_event_buffer));
		s_key_event_buffer_pos--;
		return ke;
	} else {
		return (struct key_event){
			.ready = false,
		};
	}
}

static void stage_mouse_event(struct mouse_event me) {
	if (s_mouse_event_buffer_pos == ARRAY_SIZE(s_mouse_event_buffer)) {
		s_mouse_event_buffer[s_mouse_event_buffer_pos - 1] = me;
	} else {
		s_mouse_event_buffer[s_mouse_event_buffer_pos++] = me;
	}
}

static struct mouse_event pop_mouse_event(void) {
	if (s_mouse_event_buffer_pos > 0) {
		struct mouse_event me = s_mouse_event_buffer[0];
		memmove(&s_mouse_event_buffer[0], &s_mouse_event_buffer[1], sizeof(s_mouse_event_buffer));
		s_mouse_event_buffer_pos--;
		return me;
	} else {
		return (struct mouse_event){0};
	}
}

static LRESULT CALLBACK keyboard_proc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode >= 0) {
		switch (wParam) {
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN: {
			KBDLLHOOKSTRUCT* pKeyData = (KBDLLHOOKSTRUCT*)lParam;
			struct key_event ke = {
				.ready = true,
				.key = key_code_to_generic_code(pKeyData->vkCode),
				.action = KEY_PRESS
			};
			stage_key_event(ke);
		} break;

		case WM_SYSKEYUP:
		case WM_KEYUP: {
			KBDLLHOOKSTRUCT* pKeyData = (KBDLLHOOKSTRUCT*)lParam;
			struct key_event ke = {
				.ready = true,
				.key = key_code_to_generic_code(pKeyData->vkCode),
				.action = KEY_RELEASE
			};
			stage_key_event(ke);
		} break;
		default: break;
		}
	}

	if (s_input_enabled) {
		return CallNextHookEx(s_key_hook, nCode, wParam, lParam);
	} else {
		return 1;
	}
}

static LRESULT CALLBACK mouse_proc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode >= 0) {
		switch (wParam) {
		case WM_MOUSEMOVE: {
			MSLLHOOKSTRUCT* pMouseStruct = (MSLLHOOKSTRUCT*)lParam;
			struct vec pos = {
				.x = pMouseStruct->pt.x,
				.y = pMouseStruct->pt.y
			};
			if (s_last_mouse_pos.x < 0) {
				s_last_mouse_pos = pos;
			}
			struct vec vel = vec_sub(pos, s_last_mouse_pos);
			s_last_mouse_pos = pos;
			struct mouse_event me = {
				.flags = MOUSE_EVENT_MOVE,
				.move = {{
					.x = vel.x,
					.y = vel.y,
				}}
			};
			stage_mouse_event(me);
		} break;

		case WM_MOUSEWHEEL: {
			short delta = HIWORD(wParam);
			struct mouse_event me = {
				.flags = MOUSE_EVENT_SCROLL,
				.scroll = {
					.dir = delta,
				}
			};
			stage_mouse_event(me);
		} break;

		case WM_LBUTTONDOWN: {
			s_last_mouse_left = true;
			struct mouse_event me = {
				.flags = MOUSE_EVENT_BUTTON_DOWN,
				.button = {
					.button = MOUSE_BUTTON_LEFT,
				}
			};
			stage_mouse_event(me);
		} break;

		case WM_LBUTTONUP: {
			s_last_mouse_left = false;
			struct mouse_event me = {
				.flags = MOUSE_EVENT_BUTTON_UP,
				.button = {
					.button = MOUSE_BUTTON_LEFT,
				}
			};
			stage_mouse_event(me);
		} break;

		case WM_RBUTTONDOWN: {
			s_last_mouse_right = true;
			struct mouse_event me = {
				.flags = MOUSE_EVENT_BUTTON_DOWN,
				.button = {
					.button = MOUSE_BUTTON_RIGHT,
				}
			};
			stage_mouse_event(me);
		} break;

		case WM_RBUTTONUP: {
			s_last_mouse_right = false;
			struct mouse_event me = {
				.flags = MOUSE_EVENT_BUTTON_UP,
				.button = {
					.button = MOUSE_BUTTON_RIGHT,
				}
			};
			stage_mouse_event(me);
		} break;
		default: break;
		}
	}

	if (s_input_enabled) {
		return CallNextHookEx(s_mouse_hook, nCode, wParam, lParam);
	} else {
		return 1;
	}
}

uint16_t generic_code_to_system_code(uint16_t _c) {
	switch (_c) {
	case 1: return 0x00 ;
	case 2: return VK_ESCAPE ;
	case 3: return 0x31 ;
	case 4: return 0x32 ;
	case 5: return 0x33 ;
	case 6: return 0x34 ;
	case 7: return 0x35 ;
	case 8: return 0x36 ;
	case 9: return 0x37 ;
	case 10: return 0x38 ;
	case 11: return 0x39 ;
	case 12: return 0x30 ;
	case 13: return VK_OEM_MINUS ;
	case 14: return VK_OEM_PLUS ;
	case 15: return VK_BACK ;
	case 16: return VK_TAB ;
	case 17: return 0x51 ;
	case 18: return 0x57 ;
	case 19: return 0x45 ;
	case 20: return 0x52 ;
	case 21: return 0x54 ;
	case 22: return 0x59 ;
	case 23: return 0x55 ;
	case 24: return 0x49 ;
	case 25: return 0x4f ;
	case 26: return 0x50 ;
	case 27: return VK_OEM_4 ;
	case 28: return VK_OEM_6 ;
	case 29: return VK_RETURN ;
	case 30: return 0x41 ;
	case 31: return 0x53 ;
	case 32: return 0x44 ;
	case 33: return 0x46 ;
	case 34: return 0x47 ;
	case 35: return 0x48 ;
	case 36: return 0x4a ;
	case 37: return 0x4b ;
	case 38: return 0x4c ;
	case 39: return VK_OEM_1 ;
	case 40: return VK_OEM_7 ;
	case 41: return VK_OEM_3 ;
	case 42: return VK_OEM_5 ;
	case 43: return 0x5a ;
	case 44: return 0x58 ;
	case 45: return 0x43 ;
	case 46: return 0x56 ;
	case 47: return 0x42 ;
	case 48: return 0x4e ;
	case 49: return 0x4d ;
	case 50: return VK_OEM_COMMA ;
	case 51: return VK_OEM_PERIOD ;
	case 52: return VK_OEM_2 ;
	case 53: return VK_SPACE ;
	case 54: return VK_LCONTROL ;
	case 55: return VK_LSHIFT ;
	case 56: return VK_RSHIFT ;
	case 57: return VK_MULTIPLY ;
	case 58: return VK_LMENU ;
	case 59: return VK_CAPITAL ;
	case 60: return VK_F1 ;
	case 61: return VK_F2 ;
	case 62: return VK_F3 ;
	case 63: return VK_F4 ;
	case 64: return VK_F5 ;
	case 65: return VK_F6 ;
	case 66: return VK_F7 ;
	case 67: return VK_F8 ;
	case 68: return VK_F9 ;
	case 69: return VK_F10 ;
	case 70: return VK_NUMLOCK ;
	case 71: return VK_SCROLL ;
	case 75: return VK_SUBTRACT ;
//      case 76: return XK_KP_4 ;
//      case 77: return XK_KP_5 ;
//      case 78: return XK_KP_6 ;
	case 79: return VK_ADD ;
//      case 80: return XK_KP_1 ;
//      case 81: return XK_KP_2 ;
//      case 82: return XK_KP_3 ;
//      case 83: return XK_KP_0 ;
//      case 84: return XK_period ;
	case 85: return VK_F11 ;
	case 86: return VK_F12 ;
//      case 87: return XK_Alt_R ;
//      case 88: return XK_Linefeed ;
//      case 89: return XK_Home ;
	case 90: return VK_UP ;
//      case 91: return XK_Page_Up ;
	case 92: return VK_LEFT ;
	case 93: return VK_RIGHT;
//      case 94: return XK_End ;
	case 95: return VK_DOWN ;
//      case 96: return XK_Page_Down ;
	case 97: return VK_INSERT ;
	case 98: return VK_DELETE ;
	case 99: return VK_OEM_PLUS ;
	case 100: return VK_LWIN ;
	case 101: return VK_RWIN ;
	case 102: return VK_F13 ;
	case 103: return VK_F14 ;
	case 104: return VK_F15 ;
	case 105: return VK_F16 ;
	case 106: return VK_F17 ;
	case 107: return VK_F18 ;
	case 108: return VK_F19 ;
	case 109: return VK_F20 ;
	case 110: return VK_F21 ;
	case 111: return VK_F22 ;
	case 112: return VK_F23 ;
	case 113: return VK_F24 ;
	default: return _c;
	}
}

uint16_t key_code_to_generic_code(uint16_t _c) {
	switch (_c) {
	case VK_ESCAPE: return 2;
	case 0x31: return 3;
	case 0x32: return 4;
	case 0x33: return 5;
	case 0x34: return 6;
	case 0x35: return 7;
	case 0x36: return 8;
	case 0x37: return 9;
	case 0x38: return 10;
	case 0x39: return 11;
	case 0x30: return 12;
	case VK_OEM_MINUS: return 13;
	case VK_OEM_PLUS: return 14;
	case VK_BACK: return 15;
	case VK_TAB: return 16;
	case 0x51: return 17;
	case 0x57: return 18;
	case 0x45: return 19;
	case 0x52: return 20;
	case 0x54: return 21;
	case 0x59: return 22;
	case 0x55: return 23;
	case 0x49: return 24;
	case 0x4f: return 25;
	case 0x50: return 26;
	case VK_OEM_4: return 27;
	case VK_OEM_6: return 28;
	case VK_RETURN: return 29;
	case 0x41: return 30;
	case 0x53: return 31;
	case 0x44: return 32;
	case 0x46: return 33;
	case 0x47: return 34;
	case 0x48: return 35;
	case 0x4a: return 36;
	case 0x4b: return 37;
	case 0x4c: return 38;
	case VK_OEM_1: return 39;
	case VK_OEM_7: return 40;
	case VK_OEM_3: return 41;
	case VK_OEM_5: return 42;
	case 0x5a: return 43;
	case 0x58: return 44;
	case 0x43: return 45;
	case 0x56: return 46;
	case 0x42: return 47;
	case 0x4e: return 48;
	case 0x4d: return 49;
	case VK_OEM_COMMA: return 50;
	case VK_OEM_PERIOD: return 51;
	case VK_OEM_2: return 52;
	case VK_SPACE: return 53;
	case VK_LCONTROL: return 54;
	case VK_LSHIFT: return 55;
	case VK_RSHIFT: return 56;
	case VK_MULTIPLY: return 57;
	case VK_LMENU: return 58;
	case VK_CAPITAL: return 59;
	case VK_F1: return 60;
	case VK_F2: return 61;
	case VK_F3: return 62;
	case VK_F4: return 63;
	case VK_F5: return 64;
	case VK_F6: return 65;
	case VK_F7: return 66;
	case VK_F8: return 67;
	case VK_F9: return 68;
	case VK_F10: return 69;
	case VK_NUMLOCK: return 70;
	case VK_SCROLL: return 71;
	case VK_SUBTRACT: return 75;
//      case 76: return XK_KP_4 ;
//      case 77: return XK_KP_5 ;
//      case 78: return XK_KP_6 ;
	case VK_ADD: return 79;
//      case 80: return XK_KP_1 ;
//      case 81: return XK_KP_2 ;
//      case 82: return XK_KP_3 ;
//      case 83: return XK_KP_0 ;
//      case 84: return XK_period ;
	case VK_F11: return 85;
	case VK_F12: return 86;
//      case 87: return XK_Alt_R ;
//      case 88: return XK_Linefeed ;
//      case 89: return XK_Home ;
	case VK_UP: return 90;
//      case 91: return XK_Page_Up ;
	case VK_LEFT: return 92;
	case VK_RIGHT: return 93;
//      case 94: return XK_End ;
	case VK_DOWN: return 95;
//      case 96: return XK_Page_Down ;
	case VK_INSERT: return 97;
	case VK_DELETE: return 98;
	case VK_LWIN: return 100;
	case VK_RWIN: return 101;
	case VK_F13: return 102;
	case VK_F14: return 103;
	case VK_F15: return 104;
	case VK_F16: return 105;
	case VK_F17: return 106;
	case VK_F18: return 107;
	case VK_F19: return 108;
	case VK_F20: return 109;
	case VK_F21: return 110;
	case VK_F22: return 111;
	case VK_F23: return 112;
	case VK_F24: return 113;
	default: return _c;
	}
}

#endif
