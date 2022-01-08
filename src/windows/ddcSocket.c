#include "ddcSocket.h"

#ifdef __WIN64

#include <windows.h>

void dsocket_init(void)
{
	WSADATA wsa_data;
	int iresult;

	iresult = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (iresult != 0) exit(1);
}

struct dsocket_tcp_server make_dsocket_tcp_server(int port)
{
	struct dsocket_tcp_server output;
	output.server_len = sizeof(output.server);
	output.port = port;
	output.opt = 1;
	return output;
}

int dsocket_tcp_server_bind(struct dsocket_tcp_server* sck)
{
	if ((sck->dscr = socket(AF_INET, SOCK_STREAM, 0)) == 0)
		return 1;
	if (setsockopt(sck->dscr, SOL_SOCKET, SO_REUSEADDR, (char*)&(sck->opt), sizeof(sck->opt)))
		return 1;
	if (setsockopt(sck->dscr, IPPROTO_TCP, TCP_NODELAY, (int*)&(int){1}, sizeof(int)))
		return 1;
	sck->server.sin_family = AF_INET;
	sck->server.sin_addr.s_addr = INADDR_ANY;
	sck->server.sin_port = htons(sck->port);
	if (bind(sck->dscr, (struct sockaddr*)&sck->server, sizeof(sck->server)) < 0)
		return 1;
	return 0;
}
int dsocket_tcp_server_start_listen(struct dsocket_tcp_server* sck)
{
	if (listen(sck->dscr, 3) < 0)
		return 1;
	return 0;
}
int dsocket_tcp_server_listen(struct dsocket_tcp_server* sck)
{
	int client_out = -1;
	if ((client_out = accept(sck->dscr, (struct sockaddr*)&sck->server, (int*)&(sck->server_len))) < 0)
		return -1;
	return client_out;
}
int dsocket_tcp_server_send(struct dsocket_tcp_server sck, int client, const char* data, long length)
{
	send(client, data, length, 0);
	return 1;
}
int dsocket_tcp_server_receive(struct dsocket_tcp_server sck, int client, char* buffer, long length)
{
	return recv(client, buffer, length, 0);
}

struct dsocket_tcp_client make_dsocket_tcp_client(char* addr, int port)
{
	struct dsocket_tcp_client output;
	output.addr = addr;
	output.port = port;
	return output;
}

void raze_dsocket_tcp_client(struct dsocket_tcp_client* sck)
{
}

int dsocket_tcp_client_connect(struct dsocket_tcp_client* sck)
{
	sck->dscr = socket(AF_INET, SOCK_STREAM, 0);
	if (sck->dscr == -1) return 1;
	sck->server.sin_family = AF_INET;
	sck->server.sin_port = htons(sck->port);
	sck->server.sin_addr.s_addr = inet_addr(sck->addr);

	struct timeval timeout;      
	timeout.tv_sec = 0;
	timeout.tv_usec = 150000;
	
	if (setsockopt (sck->dscr, SOL_SOCKET, SO_RCVTIMEO, &timeout,
				sizeof timeout) < 0)
		return 1;

	if (setsockopt (sck->dscr, SOL_SOCKET, SO_SNDTIMEO, &timeout,
				sizeof timeout) < 0)
		return 1;

	if (setsockopt(sck->dscr, IPPROTO_TCP, TCP_NODELAY, (int*)&(int){1}, sizeof(int)))
		return 1;

	return connect(sck->dscr, (struct sockaddr*)&sck->server, sizeof(sck->server));
}
int dsocket_tcp_client_send(struct dsocket_tcp_client sck, const char* data, long length)
{
	return send(sck.dscr, data, length, 0);
}
int dsocket_tcp_client_receive(struct dsocket_tcp_client sck, char* buffer, long length)
{
	return recv(sck.dscr, buffer, length, 0);
}


#endif