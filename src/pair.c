#include <mindcontrol/pair.h>

#include <mindcontrol/client.h>
#include <mindcontrol/device_control.h>
#include <mindcontrol/gui.h>
#include <mindcontrol/dsocket.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <glib.h>

static bool client_pair_thread(void* _)
{
	(void)_;
	struct dsocket_tcp_server srv = make_dsocket_tcp_server(6969);
	dsocket_tcp_server_bind(&srv);
	dsocket_tcp_server_start_listen(&srv);
	while (1)
	{
		dsocket_tcp_server_listen(&srv);
		char* ip = inet_ntoa(srv.server.sin_addr);
		g_idle_add(client_update_controller_ip, ip);
	}

	return 0;
}

void client_pair_with_controller(void)
{
	g_thread_new("thread", client_pair_thread, 0);
}

static bool controller_pair_thread(struct list* clients)
{
	if (clients->data == 0)
		*clients = make_list(4, struct client_info);

	char base_ip[16];
	strcpy(base_ip, device_control_get_ip());
	strrchr(base_ip, '.')[1] = 0;

	for (int i = 0; i < 255; i++)
	{
		char ip[24] = {0};
		sprintf(ip, "%s%d", base_ip, i);

		printf("trying %s:%d\n", ip, 6969);

		struct dsocket_tcp_client cli = make_dsocket_tcp_client(ip, 6969);
		if (dsocket_tcp_client_connect(&cli) != 0)
		{
			close(cli.dscr);
			continue;
		}

		struct client_info client_info;
		strcpy(client_info.ip, ip);

		list_push_back(clients, client_info, struct client_info);

		printf("found %s!\n", ip);

		close(cli.dscr);
	}

	g_idle_add(close_server_scan_dialog, 0);
	return 0;
}

void controller_pair_with_clients(struct list* client_list)
{
	g_thread_new("thread", controller_pair_thread, client_list);
}

