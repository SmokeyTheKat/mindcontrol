#ifndef __MINDCONTROL_CONTROLER_H__
#define __MINDCONTROL_CONTROLER_H__

#include "state.h"

#define CONTROL_STATE_MAIN 0
#define CONTROL_STATE_SWITCH_TO_MAIN 1
#define CONTROL_STATE_CLIENT 2
#define CONTROL_STATE_QUIT 3


void controller_main(int port);
void controller_set_state(state_t state);

#endif