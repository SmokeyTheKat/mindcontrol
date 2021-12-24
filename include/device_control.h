#ifndef __MINDCONTROL_DEVICE_CONTROL_H__
#define __MINDCONTROL_DEVICE_CONTROL_H__

#include "vec.h"
#include <stdint.h>

#define KEY_RELEASE 0x00
#define KEY_PRESS    0x01
#define KEY_REPEAT    0x02
#define MAX_OVER_READ 16

struct mouse_state;
struct key_event;

void device_control_init(void);
struct vec device_control_get_screen_size(void);

void device_control_keyboard_disable(void);
void device_control_keyboard_enable(void);
void device_control_keyboard_send(int keycode);
void device_control_keyboard_send_press(int keycode);
void device_control_keyboard_send_release(int keycode);

void device_control_cursor_move(int x, int y);
void device_control_cursor_move_to(int x, int y);
void device_control_cursor_left_down(void);
void device_control_cursor_left_up(void);
void device_control_cursor_right_up(void);
void device_control_cursor_right_down(void);
void device_control_cursor_scroll(int dir);
struct vec device_control_cursor_get(void);
struct vec device_control_cursor_on_move_get(void);
struct vec device_control_cursor_on_move_get_relative(void);

void device_control_keyboard_flush(void);
void device_control_mouse_flush(void);
struct mouse_state device_control_get_mouse_state(void);
struct key_event device_control_get_keyboard_event(void);
char* device_control_clipboard_get(void);
void device_control_clipboard_set(char* data);

uint16_t generic_code_to_system_code(uint16_t _c);
uint16_t key_code_to_generic_code(uint16_t _c);


struct mouse_state
{
	bool ready;
	int x, y;
	int scroll;
	bool left, right, middle;
};

struct key_event
{
	bool ready;
	uint16_t key;
	uint32_t action;
};

extern struct vec screen_size;

#endif