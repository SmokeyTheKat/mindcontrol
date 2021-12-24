#ifndef __MINDCONTROL_CONTROLER_H__
#define __MINDCONTROL_CONTROLER_H__

#define CONTROL_STATE_MAIN 0
#define CONTROL_STATE_SWITCH_TO_MAIN 1
#define CONTROL_STATE_CLIENT 2

void controler_init(int port);

#endif