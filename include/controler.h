#ifndef __MINDCONTROL_CONTROLER_H__
#define __MINDCONTROL_CONTROLER_H__

#include "state.h"

#define CONTROL_STATE_MAIN 0
#define CONTROL_STATE_SWITCH_TO_MAIN 1
#define CONTROL_STATE_CLIENT 2
#define CONTROL_STATE_QUIT 3


typedef data_state(int x; int y; int edge_from) control_state_t;

extern control_state_t control_state;

void controler_init(int port);

#endif