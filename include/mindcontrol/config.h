#ifndef __MINDCONTROL_CONFIG_H__
#define __MINDCONTROL_CONFIG_H__

#define SCREEN_SCALE 10000
#define REST_TIME 10

void read_args(int argc, char** argv);

extern char* server_ip;
extern int server_port;
extern char user_type;
extern int mouse_speed;
extern int scroll_speed;

#endif
