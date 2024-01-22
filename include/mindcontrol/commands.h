#ifndef __MINDCONTROL_COMMANDS_H__
#define __MINDCONTROL_COMMANDS_H__

#include <mindcontrol/vec.h>

#include <stdint.h>

#define COMMAND_QUEUE_MAX_LENGTH 32

enum {
	CMD_NONE = 0,
	CMD_CURSOR_MOVE,
	CMD_NEXT_SCREEN,
	CMD_CURSOR_TO,
	CMD_SCROLL,
	CMD_LEFT_DOWN,
	CMD_LEFT_UP,
	CMD_RIGHT_DOWN,
	CMD_RIGHT_UP,
	CMD_MIDDLE_DOWN,
	CMD_MIDDLE_UP,
	CMD_KEY_PRESS,
	CMD_KEY_RELEASE,
	CMD_GO_BY_EDGE_AT,
	CMD_TRANSFER_FILE,
	CMD_PING,
	CMD_CLIPBOARD,
};

struct command_mouse_move {
	uint8_t length;
	uint8_t type;
	struct vec d;
};

struct command_next_screen {
	uint8_t length;
	uint8_t type;
	int edge;
	struct vec pos;
};

struct command_mouse_to {
	uint8_t length;
	uint8_t type;
	struct vec pos;
};

struct command_scroll {
	uint8_t length;
	uint8_t type;
	int scroll, multiplyer;
};

struct command_left_down {
	uint8_t length;
	uint8_t type;
};

struct command_left_up {
	uint8_t length;
	uint8_t type;
};

struct command_right_down {
	uint8_t length;
	uint8_t type;
};

struct command_right_up {
	uint8_t length;
	uint8_t type;
};

struct command_middle_down {
	uint8_t length;
	uint8_t type;
};

struct command_middle_up {
	uint8_t length;
	uint8_t type;
};

struct command_key_press {
	uint8_t length;
	uint8_t type;
	int key;
};

struct command_key_release {
	uint8_t length;
	uint8_t type;
	int key;
};

struct command_update_clipboard {
	uint8_t length;
	uint8_t type;
};

struct command_go_by_edge_at {
	uint8_t length;
	uint8_t type;
	int edge;
	int edge_pos;
};

struct command_transfer_file {
	uint8_t length;
	uint8_t type;
	int file_length;
};

struct command_ping {
	uint8_t length;
	uint8_t type;
};

struct command_clipboard {
	uint8_t length;
	uint8_t type;
	int payload_length;
	char* payload;
};

struct command {
	union {
		struct {
			uint8_t length;
			uint8_t type;
		};
		struct command_mouse_move mouse_move;
		struct command_next_screen next_screen;
		struct command_mouse_to mouse_to;
		struct command_scroll scroll;
		struct command_left_down left_down;
		struct command_left_up left_up;
		struct command_right_down right_down;
		struct command_right_up right_up;
		struct command_middle_down middle_down;
		struct command_middle_up middle_up;
		struct command_key_press keypress;
		struct command_key_release keyrelease;
		struct command_update_clipboard update_clipboard;
		struct command_go_by_edge_at go_by_edge_at;
		struct command_transfer_file transfer_file;
		struct command_ping ping;
		struct command_clipboard clipboard;
	};
};

void command_queue_flush(void);
void command_queue_remove(int idx);
bool command_queue_reduce(struct command* command);

void stage_command(struct command* command);
void stage_mouse_to_command(struct vec pos);
void queue_mouse_move_command(struct vec d);
void queue_scroll_command(int scroll, int multiplyer);
void queue_left_down_command(void);
void queue_left_up_command(void);
void queue_right_down_command(void);
void queue_right_up_command(void);
void queue_middle_down_command(void);
void queue_middle_up_command(void);
void queue_key_press_command(int key);
void queue_key_release_command(int key);
void stage_next_screen_command(int edge, struct vec pos);
void stage_ping_command(void);
void stage_clipboard_command(char* text);
void stage_go_by_edge_at_command(int edge, int edge_pos);

void parse_command(const char* data, int length);

typedef void(*command_callback_t)(struct command*);
typedef void(*command_send_callback_t)(char*, int);
extern command_callback_t command_execute_callback;
extern command_send_callback_t command_send_callback;
extern struct command command_queue[COMMAND_QUEUE_MAX_LENGTH];
extern int command_queue_length;

#endif
