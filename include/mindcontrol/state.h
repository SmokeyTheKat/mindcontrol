#ifndef __MINDCONTROL_STATE_H__
#define __MINDCONTROL_STATE_H__

#include <mindcontrol/utils.h>
#include <mindcontrol/config.h>

#include <stdatomic.h>

typedef volatile atomic_int state_t;
#define data_state(data) volatile struct {state_t state; volatile struct {data;};}

#define STATE_LOW 0
#define STATE_HIGH 1

#define STATE_AWAIT(_state, _until, _then) while((_state) != (_until)) {SLEEP(REST_TIME);}; _then

#endif
