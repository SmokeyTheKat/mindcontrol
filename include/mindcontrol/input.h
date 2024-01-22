#ifndef __MINDCONTROL_INPUT_H__
#define __MINDCONTROL_INPUT_H__

void get_keyboard_name(char* buffer_out);
int get_keyboard_event(void);
void get_keyboard_input_path(char* buffer_out);

void get_mouse_name(char* buffer_out);
int get_mouse_event(void);
void get_mouse_input_path(char* buffer_out);

#endif
