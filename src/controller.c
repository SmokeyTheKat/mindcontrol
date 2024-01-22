#include <mindcontrol/controller.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <mindcontrol/device_control.h>
#include <mindcontrol/commands.h>
#include <mindcontrol/dsocket.h>
#include <mindcontrol/config.h>
#include <mindcontrol/screen.h>
#include <mindcontrol/client.h>
#include <mindcontrol/state.h>
#include <mindcontrol/gui.h>
#include <mindcontrol/utils.h>
#include <mindcontrol/dragdrop.h>

#define MAX_VEL ((struct vec){30, 30})

static void switch_state_to_main(void);
static void switch_to_client_on_edge(int edge_hit);
static void forward_input(void);
static void forward_mouse_input(struct mouse_event e);
static void forward_keyboard_input(struct key_event e);
static void execute_command(struct command* cmd);
static void send_command(char* data, int length);

static struct dsocket_tcp_server server;

static struct client* active_client;

static data_state(int x; int y; int edge_from) control_state;

static char* transfer_client_to_server(struct client* client, char* filepath, long file_length) {
	char* filename = strrstr(filepath, "/\\") + 1;

	char* tmp_filepath = malloc(2048);
	tmp_filepath[0] = 0;

	strcat(tmp_filepath, "/home/william/.tmp/");
	strcat(tmp_filepath, filename);

	FILE* fp = fopen(tmp_filepath, "wb");
	if (fp == 0) {
		return 0;
	}

	char buffer[4096];

	int byte_count;
	int bytes_read = 0;
	while (
		bytes_read < file_length &&
		(byte_count =
			dsocket_tcp_server_receive(
				server,
				client->sck, buffer, sizeof(buffer)
			)
		) > 0
	) {
		fwrite(buffer, 1, MIN(byte_count, file_length - bytes_read), fp);
		bytes_read += byte_count;
	}

	fclose(fp);

	return tmp_filepath;
}

void controller_set_state(state_t state) {
	control_state.state = state;
}

struct client* get_client(void) {
	int sck = dsocket_tcp_server_listen(&server);
	char* ip = inet_ntoa(server.server.sin_addr);
	printf("%s connected!\n", ip);
	
	struct client* client = client_find_by_ip(server_client, ip);
	client->active = true;
	client->sck = sck;
	return client;
}

static void deactivate_client(void) {
	active_client->sck = -1;
	active_client->active = false;
	control_state.state = CONTROL_STATE_SWITCH_TO_MAIN;
	active_client = server_client;
}

static void send_command(char* data, int length) {
	dsocket_tcp_server_send(server, active_client->sck, data, length);
}

static void execute_command(struct command* cmd) {
	switch (cmd->type) {
	case CMD_TRANSFER_FILE: {
//        char filename[8192];
//        data_get_string(&data, filename);
//        data -= 3;
//        printf("%s\n", data);
//        long file_length;
//        data_get_value(&data, "%ld", &file_length);
//        printf("%s : %ld\n", filename, file_length);
//        char* tmp_filepath = transfer_client_to_server(active_client, filename, file_length);
//        set_dragdrop_files(tmp_filepath);
	} break;

	case CMD_NEXT_SCREEN: {
		if (control_state.state == CONTROL_STATE_SWITCH_TO_MAIN) break;
		struct command_next_screen* cns = cmd;
		struct vec pos = screen_unscale_vec(cns->pos);

		struct client* next_client = client_get_client_in_direction(
			active_client, cns->edge
		);

		if (next_client == 0) break;

		if (edge_hit_is_dead_corner(pos, active_client->dead_corners)) {
			break;
		}

		if (next_client == server_client) {
			control_state.y = pos.y;
			control_state.x = pos.x;
			control_state.edge_from = cns->edge;
			control_state.state = CONTROL_STATE_SWITCH_TO_MAIN;
			stage_mouse_to_command(
				screen_scale_vec(vec_div_const(g_screen_size, 2))
			);
			command_queue_flush();
			active_client = next_client;
		} else if (next_client) {
			control_state.state = CONTROL_STATE_CLIENT;
			stage_mouse_to_command(
				screen_scale_vec(vec_div_const(g_screen_size, 2))
			);

			active_client = next_client;

			stage_mouse_to_command(
				get_scaled_vec_close_to_edge(pos, other_edge(cns->edge), 2)
			);
			command_queue_flush();
		}
	} break;

	case CMD_CLIPBOARD: {
		printf("SET CLIP\n");
		struct command_clipboard* cc = cmd;
		device_control_clipboard_set(cc->payload);
	} break;

	default: break;
	}
}

CREATE_THREAD(controller_receive, void*, _, {
	(void)_;
	while (1) {
		if (active_client->sck == -1 || active_client->active == false) {
			SLEEP(REST_TIME);
			continue;
		}

		char buffer[512] = {0};

		int bytes_read = dsocket_tcp_server_receive(server, active_client->sck, buffer, sizeof(buffer));
		if (bytes_read == 0) {
			deactivate_client();
			continue;
		} else if (bytes_read < 0) {
			continue;
		}

		parse_command(buffer, bytes_read);
	}
	return 0;
})

CREATE_THREAD(accept_clients, void*, _, {
	(void)_;
	while (1) get_client();
	return 0;
})

CREATE_THREAD(flush_commands_timer, void*, _, {
	(void)_;
	while (1) {
		SLEEP(10);
		command_queue_flush();
	}
	return 0;
})

static void switch_state_to_main(void) {
	device_control_enable_input();
	struct vec edge_pos = get_vec_close_to_edge(
		(struct vec){ control_state.x, control_state.y },
		other_edge(control_state.edge_from),
		2
	);
	device_control_mouse_move_to(edge_pos.x, edge_pos.y);
	control_state.state = CONTROL_STATE_MAIN;
}

static void switch_to_client_on_edge(int edge_hit) {
	struct vec pos = device_control_mouse_get_position();

	struct client* next_client = client_get_client_in_direction(active_client, edge_hit);
	if (next_client == 0 || next_client->active == false) {
		return;
	}

	if (edge_hit_is_dead_corner(pos, active_client->dead_corners)) {
		return;
	}

	active_client = next_client;

	stage_go_by_edge_at_command(
		other_edge(edge_hit),
		get_scaled_pos_on_edge(edge_hit, pos)
	);

	if (device_control_clipboard_changed()) {
		stage_clipboard_command(device_control_clipboard_get());
	}

	control_state.state = CONTROL_STATE_CLIENT;
	device_control_disable_input();

	device_control_keyboard_flush();
	device_control_mouse_flush();
}

static void switch_client_if_edge_hit(void) {
	struct vec pos = device_control_mouse_get_position();
	int edge_hit = get_edge_hit(pos);
	if (edge_hit != EDGE_NONE) {
		switch_to_client_on_edge(edge_hit);
	}
	SLEEP(REST_TIME);
}

static void forward_mouse_input(struct mouse_event e) {
	if (e.flags & MOUSE_EVENT_MOVE) {
		queue_mouse_move_command(
			vec_mul_float(e.move.vel, active_client->mouse_speed)
		);
		device_control_mouse_move_to(g_screen_size.x / 2, g_screen_size.y / 2);
	}

	if (e.flags & MOUSE_EVENT_SCROLL) {
		queue_scroll_command(
			e.scroll.dir,
			active_client->scroll_speed
		);
	}

	if (e.flags & MOUSE_EVENT_BUTTON_DOWN) {
		switch (e.button.button) {
		case MOUSE_BUTTON_LEFT: queue_left_down_command(); break;
		case MOUSE_BUTTON_RIGHT: queue_right_down_command(); break;
		case MOUSE_BUTTON_MIDDLE: queue_middle_down_command(); break;
		}
	}

	if (e.flags & MOUSE_EVENT_BUTTON_UP) {
		switch (e.button.button) {
		case MOUSE_BUTTON_LEFT: queue_left_up_command(); break;
		case MOUSE_BUTTON_RIGHT: queue_right_up_command(); break;
		case MOUSE_BUTTON_MIDDLE: queue_middle_up_command(); break;
		}
	}
}

static void forward_keyboard_input(struct key_event e) {
	switch (e.action) {
	case KEY_PRESS: queue_key_press_command(e.key); break;
	case KEY_RELEASE: queue_key_release_command(e.key); break;
	default: break;
	}
}

static void forward_input(void) {
	if (device_control_is_sleeping()) {
		control_state.state = CONTROL_STATE_SWITCH_TO_MAIN;
	}

	if (device_control_poll_input(REST_TIME) == false) return;

	struct mouse_event mouse_event = device_control_get_mouse_event();
	if (mouse_event.flags) {
		forward_mouse_input(mouse_event);
	}

	struct key_event key_event = device_control_get_keyboard_event();
	if (key_event.ready) {
		forward_keyboard_input(key_event);
	}
}

void controller_main(int port) {
	command_execute_callback = execute_command;
	command_send_callback = send_command;

	printf("starting controller on port %d...\n", port);
	active_client = server_client;

	control_state.state = CONTROL_STATE_MAIN;
	server = make_dsocket_tcp_server(port);
	dsocket_tcp_server_bind(&server);
	dsocket_tcp_server_start_listen(&server);

	THREAD controller_receive_thread = MAKE_THREAD();
	THREAD_CALL(&controller_receive_thread, controller_receive, &active_client->sck);

	THREAD accept_clients_thread = MAKE_THREAD();
	THREAD_CALL(&accept_clients_thread, accept_clients, 0);

	THREAD flush_commands_thread = MAKE_THREAD();
	THREAD_CALL(&flush_commands_thread, flush_commands_timer, 0);

	while (1) {
		switch (control_state.state) {
		case CONTROL_STATE_CLIENT: forward_input(); break;
		case CONTROL_STATE_SWITCH_TO_MAIN: switch_state_to_main(); break;
		case CONTROL_STATE_MAIN: switch_client_if_edge_hit(); break;
		case CONTROL_STATE_QUIT: {
			THREAD_KILL(&controller_receive_thread);
			THREAD_KILL(&accept_clients_thread);
			THREAD_KILL(&flush_commands_thread);
			return;
		} break;
		}
	}
}
