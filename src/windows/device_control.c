#include "device_control.h"

#ifdef _WIN64

#include <windows.h>
#include <winuser.h>

#include "ddcSocket.h"
#include "config.h"

struct vec screen_size;

void device_control_init(void)
{
	dsocket_init();
	screen_size = device_control_get_screen_size();
}

struct vec device_control_get_screen_size(void)
{
	HWND hwnd = GetDesktopWindow();
	HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info;
	info.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(monitor, &info);
	struct vec size;
	size.x = info.rcMonitor.right - info.rcMonitor.left;
	size.y = info.rcMonitor.bottom - info.rcMonitor.top;
	return size;
}

void device_control_keyboard_disable(void)
{
}

void device_control_keyboard_enable(void)
{
}

void device_control_cursor_move(int x, int y)
{
	INPUT ip = {0};
	ip.type = INPUT_MOUSE;
	ip.mi.mouseData = 0;
	ip.mi.dx = x;
	ip.mi.dy = y;
	ip.mi.dwFlags = MOUSEEVENTF_MOVE;
	SendInput(1, &ip, sizeof(ip));
}

void device_control_cursor_move_to(int x, int y)
{
	SetCursorPos(x, y);
}

void device_control_cursor_left_down(void)
{
	INPUT ip = {0};
	ip.type = INPUT_MOUSE;
	ip.mi.mouseData = 0;
	ip.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	ip.mi.time = 0;
	ip.ki.dwExtraInfo = (ULONG_PTR)&(long){0};
	SendInput(1, &ip, sizeof(ip));
}

void device_control_cursor_left_up(void)
{
	INPUT ip = {0};
	ip.type = INPUT_MOUSE;
	ip.mi.mouseData = 0;
	ip.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	ip.mi.time = 0;
	ip.ki.dwExtraInfo = (ULONG_PTR)&(long){0};
	SendInput(1, &ip, sizeof(ip));
}

void device_control_cursor_right_down(void)
{
	INPUT ip = {0};
	ip.type = INPUT_MOUSE;
	ip.mi.mouseData = 0;
	ip.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
	ip.mi.time = 0;
	ip.ki.dwExtraInfo = (ULONG_PTR)&(long){0};
	SendInput(1, &ip, sizeof(ip));
}

void device_control_cursor_right_up(void)
{
	INPUT ip = {0};
	ip.type = INPUT_MOUSE;
	ip.mi.mouseData = 0;
	ip.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
	ip.mi.time = 0;
	ip.ki.dwExtraInfo = (ULONG_PTR)&(long){0};
	SendInput(1, &ip, sizeof(ip));
}

void device_control_keyboard_send_press(int keycode)
{
	INPUT ip = {0};
	ip.type = INPUT_KEYBOARD;
	ip.ki.wVk = keycode;
	ip.ki.wScan = 0;
	ip.ki.dwFlags = 0;
	ip.ki.time = 0;
	ip.ki.dwExtraInfo = (ULONG_PTR)&(long){0};
	SendInput(1, &ip, sizeof(ip));
}

void device_control_keyboard_send_release(int keycode)
{
	INPUT ip = {0};
	ip.type = INPUT_KEYBOARD;
	ip.ki.wVk = keycode;
	ip.ki.wScan = 0;
	ip.ki.dwFlags = 2;
	ip.ki.time = 0;
	ip.ki.dwExtraInfo = (ULONG_PTR)&(long){0};
	SendInput(1, &ip, sizeof(ip));
}

void device_control_keyboard_send(int keycode)
{
	device_control_keyboard_send_press(keycode);
	device_control_keyboard_send_release(keycode);
}

void device_control_cursor_scroll(int dir)
{
	INPUT ip = {0};
	ip.type = INPUT_MOUSE;
	ip.mi.mouseData = -dir * scroll_speed;
	ip.mi.dwFlags = MOUSEEVENTF_WHEEL;
	SendInput(1, &ip, sizeof(ip));
}

struct vec device_control_cursor_get(void)
{
	POINT pos;
	GetCursorPos(&pos);
	return (struct vec){pos.x, pos.y};
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
}

void device_control_mouse_flush(void)
{
}

struct mouse_state device_control_get_mouse_state(void)
{
	POINT pos;
	GetCursorPos(&pos);
	bool left = true == GetKeyState(1);
	bool right = true == GetKeyState(2);
	bool middle = true == GetKeyState(4);
	return (struct mouse_state){
		.ready=true,
		.x=pos.x,
		.y=pos.y,
		.scroll=0,
		.left=left,
		.right=right,
		.middle=middle,
	};
}

struct key_event device_control_get_keyboard_event(void)
{
	return (struct key_event){0};
}

char* device_control_clipboard_get(void)
{
	return 0;
}

void device_control_clipboard_set(char* data)
{
	(void)data;
}

uint16_t generic_code_to_system_code(uint16_t _c)
{
	switch (_c)
	{
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


#endif