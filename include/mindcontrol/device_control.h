#ifndef __MINDCONTROL_DEVICE_CONTROL_H__
#define __MINDCONTROL_DEVICE_CONTROL_H__

#include <mindcontrol/vec.h>
#include <stdint.h>

#define KEY_RELEASE 0x00
#define KEY_PRESS    0x01
#define KEY_REPEAT    0x02
#define MAX_OVER_READ 16

struct mouse_state;
struct mouse_event;
struct key_event;

void device_control_init(void);
void device_control_cleanup(void);

char* device_control_get_hostname(void);
char* device_control_get_ip(void);

struct vec device_control_get_screen_size(void);

void device_control_disable_input(void);
void device_control_enable_input(void);

void device_control_keyboard_send(int keycode);
void device_control_keyboard_send_press(int keycode);
void device_control_keyboard_send_release(int keycode);
void device_control_keyboard_flush(void);
bool device_control_poll_keyboard_input(long long delay);
struct key_event device_control_get_keyboard_event(void);

void device_control_mouse_move(int x, int y);
void device_control_mouse_move_to(int x, int y);
void device_control_mouse_left_down(void);
void device_control_mouse_left_up(void);
void device_control_mouse_right_up(void);
void device_control_mouse_right_down(void);
void device_control_mouse_scroll(int dir);
void device_control_mouse_cursor_show(void);
void device_control_mouse_cursor_hide(void);
bool device_control_mouse_cursor_is_hidden(void);
struct vec device_control_mouse_get_position(void);
void device_control_mouse_flush(void);
bool device_control_poll_mouse_input(long long delay);
struct mouse_event device_control_get_mouse_event(void);
struct mouse_state device_control_get_mouse_state(void);

bool device_control_poll_input(long long delay);

bool device_control_clipboard_changed(void);
char* device_control_clipboard_get(void);
void device_control_clipboard_set(char* data);

bool device_control_is_sleeping(void);

uint16_t generic_code_to_system_code(uint16_t _c);
uint16_t key_code_to_generic_code(uint16_t _c);

bool mouse_state_changed(struct mouse_state a, struct mouse_state b);

enum {
	MOUSE_EVENT_MOVE = 1 << 0,
	MOUSE_EVENT_BUTTON_DOWN = 1 << 1,
	MOUSE_EVENT_BUTTON_UP = 1 << 2,
	MOUSE_EVENT_SCROLL = 1 << 3,
};

enum {
	MOUSE_BUTTON_LEFT,
	MOUSE_BUTTON_RIGHT,
	MOUSE_BUTTON_MIDDLE,
};

struct mouse_event {
	int flags;
	struct {
		struct {
			struct vec vel;
		} move;

		struct {
			int button;
		} button;

		struct {
			int dir;
		} scroll;
	};
};

struct mouse_state {
	bool ready;
	int x, y;
	int scroll;
	bool left, right, middle;
};

struct key_event {
	bool ready;
	uint16_t key;
	uint32_t action;
};

extern struct vec g_screen_size;

#endif
