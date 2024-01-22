#ifndef __MINDCONTROL_GUI_H__
#define __MINDCONTROL_GUI_H__

#include <gtk/gtk.h>
#include <stdbool.h>

int gui_main(void);
void gui_set_clipboard(const char* text);
void gui_get_clipboard(char* text_out);
GtkWidget* generate_monitor_view(void);
bool client_update_controller_ip(char* ip);
bool close_server_scan_dialog(void* _);

#endif
