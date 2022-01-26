#ifndef __MINDCONTROL_PAIR_H__
#define __MINDCONTROL_PAIR_H__

#include "list.h"

void client_pair_with_controller(void);
void controller_pair_with_clients(struct list* client_list);

#endif