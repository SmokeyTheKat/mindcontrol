#include "config.h"

#include <string.h>
#include <stdlib.h>

char* server_ip = "0.0.0.0";
int server_port = 0;
char user_type = 't';

void read_args(int argc, char** argv)
{
	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-ip"))
		{
			server_ip = argv[i+1];
			i++;
		}
		else if (!strcmp(argv[i], "-port"))
		{
			server_port = atoi(argv[i+1]);
			i++;
		}
		else user_type = argv[i][0];
	}
}