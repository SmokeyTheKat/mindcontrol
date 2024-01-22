#include <mindcontrol/client.h>

#include <mindcontrol/dsocket.h>
#include <mindcontrol/device_control.h>
#include <mindcontrol/commands.h>
#include <mindcontrol/vec.h>
#include <mindcontrol/utils.h>
#include <mindcontrol/config.h>
#include <mindcontrol/screen.h>
#include <mindcontrol/list.h>
#include <mindcontrol/dragdrop.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

static void execute_command(struct command* cmd);
static void send_command(char* data, int length);

static struct dsocket_tcp_client client;

long get_file_length(char* filepath)
{
	FILE* fp = fopen(filepath, "rb");
	if (fp == 0) return 0;
	fseek(fp, 0L, SEEK_END);
	long length = ftell(fp) - 1;
	fclose(fp);
	return length;
}

void transfer_file_to_server(char* filepath)
{
	long file_length = get_file_length(filepath);
	printf("len: %ld\n", file_length);
//    send_command(COMMAND_TRANSFER_FILE, "%s\x01 %ld", filepath, file_length);
//
//    FILE* fp = fopen(filepath, "rb");
//
//    if (fp == 0)
//        return;
//
//    int byte_count = 0;
//    int bytes_read = 0;
//    char buffer[4096];
//    while (byte_count < file_length && (byte_count = fread(buffer, 1, sizeof(buffer), fp)) > 0)
//    {
//        if (dsocket_tcp_client_send(client, buffer, byte_count) <= 0)
//            break;
//        bytes_read += byte_count;
//    }
//
//    fclose(fp);
}

static void send_command(char* data, int length) {
	dsocket_tcp_client_send(client, data, length);
}

static void execute_command(struct command* cmd) {
	switch (cmd->type) {
	case CMD_LEFT_UP: {
		device_control_mouse_left_up();
	} break;

	case CMD_LEFT_DOWN: {
		device_control_mouse_left_down();
	} break;

	case CMD_RIGHT_UP: {
		device_control_mouse_right_up();
	} break;

	case CMD_RIGHT_DOWN: {
		device_control_mouse_right_down();
	} break;

	case CMD_MIDDLE_UP: {
	} break;

	case CMD_MIDDLE_DOWN: {
	} break;

	case CMD_SCROLL: {
		struct command_scroll* cs = cmd;
		for (int i = 0; i < cs->multiplyer; i++) {
			device_control_mouse_scroll(cs->scroll);
		}
	} break;

	case CMD_KEY_PRESS: {
		struct command_key_press* ckp = cmd;
		device_control_keyboard_send_press(generic_code_to_system_code(ckp->key));
	} break;

	case CMD_KEY_RELEASE: {
		struct command_key_press* ckr = cmd;
		device_control_keyboard_send_release(generic_code_to_system_code(ckr->key));
	} break;

	case CMD_CURSOR_MOVE: {
		struct command_mouse_move* ccu = cmd;
		struct vec pos = vec_mul_float(ccu->d, mouse_speed);
		device_control_mouse_move(pos.x, pos.y);
	} break;

	case CMD_CURSOR_TO: {
		struct command_mouse_to* cct = cmd;
		struct vec pos = screen_unscale_vec(cct->pos);
		device_control_mouse_move_to(pos.x, pos.y);
	} break;

	case CMD_CLIPBOARD: {
		struct command_clipboard* cc = cmd;
		device_control_clipboard_set(cc->payload);
	} break;

	case CMD_GO_BY_EDGE_AT: {
		struct command_go_by_edge_at* cgbea = cmd;
		struct vec pos = get_unscaled_vec_at_edge_pos(cgbea->edge, cgbea->edge_pos);
		device_control_mouse_move_to(pos.x, pos.y);
	} break;

	case CMD_PING: {
		printf("PING\n");
		stage_ping_command();
	} break;

	default: break;
	}
}

void receiver_cleanup(void) {
	close(client.dscr);
}

void receiver_main(char* ip, int port) {
	command_execute_callback = execute_command;
	command_send_callback = send_command;

RECEIVER_INIT_RETRY:
	printf("connecting to controller on %s:%d\n", ip, port);
	client = make_dsocket_tcp_client(ip, port);
	if (dsocket_tcp_client_connect(&client) != 0) {
		close(client.dscr);
		SLEEP(250);
		goto RECEIVER_INIT_RETRY;
	}

	while (1) {
		char buffer[9024] = {0};
		int bytes_read = dsocket_tcp_client_receive(client, buffer, sizeof(buffer));

		if (bytes_read == 0) {
			close(client.dscr);
			goto RECEIVER_INIT_RETRY;
		} else if (bytes_read < 0) {
			continue;
		}

		parse_command(buffer, bytes_read);

		struct vec pos = device_control_mouse_get_position();
		int edge_hit = get_edge_hit(pos);
		if (edge_hit != EDGE_NONE) {
			if (device_control_clipboard_changed()) {
//                stage_clipboard_command(device_control_clipboard_get());
			}
			stage_next_screen_command(edge_hit, screen_scale_vec(pos));
			command_queue_flush();
		}
	}
}
