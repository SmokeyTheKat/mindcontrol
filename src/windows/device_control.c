#include "device_control.h"

#ifdef _WIN64

#include <Winuser.h>
#include <Windows.h>

void device_control_init(void)
{
}

void device_control_keyboard_disable(void)
{
}

void device_control_keyboard_enable(void)
{
}

void device_control_cursor_move(int x, int y)
{
	struct vec pos = device_control_cursor_get();
	device_control_cursor_move_to(pos.x + x, pos.y + y);
}

void device_control_cursor_move_to(int x, int y)
{
	SetCursorPos(x, y);
}

void device_control_cursor_left_down(void)
{
	mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
}

void device_control_cursor_left_up(void)
{
	mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

void device_control_cursor_right_down(void)
{
	mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
}

void device_control_cursor_right_up(void)
{
	mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
}

void device_control_keyboard_send_press(int keycode)
{
	keybd_event(VkKeyScanA(keycode), 0x45, 
				KEYEVENTF_EXTENDEDKEY | 0, 0);
}

void device_control_keyboard_send_release(int keycode)
{
	keybd_event(VkKeyScanA(keycode), 0x45, 
				KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
}

void device_control_keyboard_send(int keycode)
{
	device_control_keyboard_send_press(keycode);
	device_control_keyboard_send_release(keycode);
}

void device_control_cursor_scroll(int dir)
{
}

struct vec device_control_cursor_get(void)
{
	POINT pos;
	GetCursorPos(&pos);
	return (struct vec){pos.x, pos.y};
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

#endif