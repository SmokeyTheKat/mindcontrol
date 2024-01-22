#include <mindcontrol/commands.h>

#include <mindcontrol/utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void command_callback_none(struct command* _) {
	(void)_;
}

static void command_send_callback_none(char* _, int __) {
	(void)_;
	(void)__;
}

command_callback_t command_execute_callback = command_callback_none;
command_send_callback_t command_send_callback = command_send_callback_none;

struct command command_queue[COMMAND_QUEUE_MAX_LENGTH];
int command_queue_length = 0;

void command_queue_flush(void) {
	if (command_queue_length == 0) return;
	for (int i = 0; i < command_queue_length; i++) {
		command_send_callback(&command_queue[i], command_queue[i].length);
		switch (command_queue[i].type) {
		case CMD_CLIPBOARD: {
			struct command_clipboard* cc = &command_queue[i];
			command_send_callback(cc->payload, cc->payload_length);
			free(cc->payload);
		} break;
		default: break;
		}
	}
	command_queue_length = 0;
}

void command_queue_remove(int idx) {
	for (int i = idx; i + 1 < command_queue_length; i++) {
		command_queue[i] = command_queue[i + 1];
	}
	command_queue_length--;
}

bool command_queue_reduce(struct command* command) {
	switch (command->type) {
	case CMD_CURSOR_MOVE: {
		struct command_mouse_move* cmd_ccm = command;
		for (int i = command_queue_length - 1; i >= 0; i--) {
			struct command* reduce = &command_queue[i];
			switch (reduce->type) {
			case CMD_CURSOR_MOVE: {
				struct command_mouse_move* reduce_ccm = reduce;
				reduce_ccm->d = vec_add(reduce_ccm->d, cmd_ccm->d);
				return true;
			} break;

			case CMD_NEXT_SCREEN:
			case CMD_GO_BY_EDGE_AT:
			case CMD_CURSOR_TO: {
				return false;
			} break;

			default: break;
			}
		}
	} break;

	case CMD_CURSOR_TO:
	case CMD_GO_BY_EDGE_AT: {
		for (int i = command_queue_length - 1; i >= 0; i--) {
			struct command* reduce = &command_queue[i];
			switch (reduce->type) {
			case CMD_GO_BY_EDGE_AT:
			case CMD_CURSOR_MOVE:
			case CMD_CURSOR_TO: {
				command_queue_remove(i);
			} break;


			default: break;
			}
		}
	} break;

	default: break;

	}

	return false;
}

void stage_command(struct command* command) {
	if (command_queue_length == COMMAND_QUEUE_MAX_LENGTH) {
		command_queue_flush();
	}

	if (command_queue_reduce(command) == false) {
		memcpy(
			command_queue + command_queue_length,
			command,
			command->length
		);
		command_queue_length++;
	}
	command_queue_flush();
}

void stage_mouse_to_command(struct vec pos) {
	struct command_mouse_to cmd = {
		sizeof(struct command_mouse_to),
		CMD_CURSOR_TO,
		pos,
	};
	stage_command(&cmd);
}

void queue_mouse_move_command(struct vec d) {
	struct command_mouse_move cmd = {
		sizeof(struct command_mouse_move),
		CMD_CURSOR_MOVE,
		d,
	};
	stage_command(&cmd);
}

void queue_scroll_command(int scroll, int multiplyer) {
	struct command_scroll cmd = {
		sizeof(struct command_scroll),
		CMD_SCROLL,
		scroll,
		multiplyer,
	};
	stage_command(&cmd);
}

void queue_left_down_command(void) {
	struct command_left_down cmd = {
		sizeof(struct command_left_down),
		CMD_LEFT_DOWN,
	};
	stage_command(&cmd);
}

void queue_left_up_command(void) {
	struct command_left_up cmd = {
		sizeof(struct command_left_up),
		CMD_LEFT_UP,
	};
	stage_command(&cmd);
}

void queue_right_down_command(void) {
	struct command_right_down cmd = {
		sizeof(struct command_right_down),
		CMD_RIGHT_DOWN,
	};
	stage_command(&cmd);
}

void queue_right_up_command(void) {
	struct command_right_up cmd = {
		sizeof(struct command_right_up),
		CMD_RIGHT_UP,
	};
	stage_command(&cmd);
}

void queue_middle_down_command(void) {
	struct command_middle_down cmd = {
		sizeof(struct command_middle_down),
		CMD_MIDDLE_DOWN,
	};
	stage_command(&cmd);
}

void queue_middle_up_command(void) {
	struct command_middle_up cmd = {
		sizeof(struct command_middle_up),
		CMD_MIDDLE_UP,
	};
	stage_command(&cmd);
}

void queue_key_press_command(int key) {
	struct command_key_press cmd = {
		sizeof(struct command_key_press),
		CMD_KEY_PRESS,
		key,
	};
	stage_command(&cmd);
}

void queue_key_release_command(int key) {
	struct command_key_release cmd = {
		sizeof(struct command_key_release),
		CMD_KEY_RELEASE,
		key,
	};
	stage_command(&cmd);
}

void stage_next_screen_command(int edge, struct vec pos) {
	struct command_next_screen cmd = {
		sizeof(struct command_next_screen),
		CMD_NEXT_SCREEN,
		edge,
		pos,
	};
	stage_command(&cmd);
}

void stage_ping_command(void) {
	struct command_ping cmd = {
		sizeof(struct command_ping),
		CMD_PING,
	};
	stage_command(&cmd);
}

void stage_clipboard_command(char* text) {
	char* payload = malloc(strlen(text) + 1);
	strcpy(payload, text);
	struct command_clipboard cmd = {
		sizeof(struct command_clipboard),
		CMD_CLIPBOARD,
		strlen(text) + 1,
		payload
	};
	stage_command(&cmd);
}

void stage_go_by_edge_at_command(int edge, int edge_pos) {
	struct command_go_by_edge_at cmd = {
		sizeof(struct command_go_by_edge_at),
		CMD_GO_BY_EDGE_AT,
		edge,
		edge_pos,
	};
	stage_command(&cmd);
}

void parse_command(const char* data, int length) {
	static char command_bytes[sizeof(struct command)];
	static char* data_out = command_bytes;
	static int bytes_read = 0;
	static int data_length = 0;
	static char* payload = NULL;

	while (length > 0) {
		if (data_length == 0) {
			data_out = command_bytes;
			payload = NULL;
			data_length = data[0];
			command_bytes[0] = data_length;
			bytes_read = 1;
			data++;
			length--;
		}

		int bytes_to_copy = MIN(length, data_length - bytes_read);
		memcpy(data_out + bytes_read, data, bytes_to_copy);
		bytes_read += bytes_to_copy;
		data += bytes_to_copy;
		length -= bytes_to_copy;

		if (bytes_read == data_length) {
			struct command* command = &command_bytes;
			if (payload != NULL) {
				command_execute_callback(&command_bytes);
				data_length = 0;
			} else switch (command->type) {
				case CMD_CLIPBOARD: {
					struct command_clipboard* cc = command;
					cc->payload = malloc(cc->payload_length);
					payload = cc->payload;
					data_out = payload;
					data_length = cc->payload_length;
					bytes_read = 0;
				} break;
				default: {
					command_execute_callback(&command_bytes);
					data_length = 0;
				} break;
			}
		}
	}
}
