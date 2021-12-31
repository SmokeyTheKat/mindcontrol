#include "clipboard.h"

#ifdef __unix__

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>

#include "utils.h"
#include "state.h"

static void clipboard_set_x_data(void);
static void clipboard_load_clipboard_buffer(void);
static void clipboard_handel_nonowner_events(void);
static void send_select_response_event(XSelectionRequestEvent* sev, Atom type_id);
static void clipboard_handel_owner_events(void);

static char clipboard_data[65536] = {0};

static Display* dpy;
static int screen;
static Window root;
static Window owner;

static state_t set_clip = STATE_LOW;
static state_t get_clip = STATE_LOW;
static state_t clipboard_owner = STATE_LOW;

static Atom utf8_id;
static Atom string_id;
static Atom primary_id;
static Atom clipboard_id;
static Atom sel_data_id;
static Atom incr_id;

static void clipboard_set_x_data(void)
{
	clipboard_owner = STATE_HIGH;
	XSetSelectionOwner(dpy, primary_id, owner, CurrentTime);
	set_clip = STATE_LOW;
}

static void clipboard_load_clipboard_buffer(void)
{
	XEvent event;
	XConvertSelection(dpy, primary_id, utf8_id, sel_data_id, owner, CurrentTime);
	do XNextEvent(dpy, &event);
	while (event.type != SelectionNotify || event.xselection.selection != primary_id);
	
	if (event.xselection.property)
	{
		unsigned long ressize;
		char* result;
	
		XGetWindowProperty(dpy, owner, sel_data_id, 0,
						   LONG_MAX/4, False, AnyPropertyType,
						   &utf8_id, NOBJ(int), &ressize,
						   NOBJ(unsigned long), (unsigned char**)&result);
	
		if (sel_data_id != incr_id)
		{
			strncpy(clipboard_data, result, ressize);
			clipboard_data[ressize] = 0;
		}
		XFree(result);
	}
	get_clip = STATE_LOW;
}

static void clipboard_handel_nonowner_events(void)
{
	while (clipboard_owner == STATE_LOW)
	{
		if (set_clip) clipboard_set_x_data();
		if (get_clip) clipboard_load_clipboard_buffer();
		SLEEP(REST_TIME);
	}
}

static void send_select_response_event(XSelectionRequestEvent* sev, Atom type_id)
{
	char* req_name = XGetAtomName(dpy, sev->property);
	if (req_name) XFree(req_name);

	XChangeProperty(dpy, sev->requestor, sev->property, type_id, 8, PropModeReplace,
					(unsigned char *)clipboard_data, strlen(clipboard_data));

	XSelectionEvent ssev;
	ssev.type = SelectionNotify;
	ssev.requestor = sev->requestor;
	ssev.selection = sev->selection;
	ssev.target = sev->target;
	ssev.property = sev->property;
	ssev.time = sev->time;

	XSendEvent(dpy, sev->requestor, True, NoEventMask, (XEvent *)&ssev);
}

static void clipboard_handel_owner_events(void)
{
	XEvent event;
	XNextEvent(dpy, &event);
	switch (event.type)
	{
		case SelectionClear:
		{
			clipboard_owner = STATE_LOW;
		} break;
		case SelectionRequest:
		{
			XSelectionRequestEvent* sev = (XSelectionRequestEvent*)&event.xselectionrequest;
			if (sev->target == utf8_id && sev->property != None)
				send_select_response_event(sev, utf8_id);
			else send_select_response_event(sev, string_id);
		} break;
	}
}

void* clipboard_serve(void* _)
{
	while (1)
	{
		if (clipboard_owner == STATE_LOW) clipboard_handel_nonowner_events();
		else clipboard_handel_owner_events();
	}
}

void init_clipboard(void)
{
	dpy = XOpenDisplay(0);
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	owner = XCreateSimpleWindow(dpy, root, -10, -10, 1, 1, 0, 0, 0);

	utf8_id = XInternAtom(dpy, "UTF8_STRING", False);
	string_id = XInternAtom(dpy, "STRING", False);
	primary_id = XInternAtom(dpy, "PRIMARY", False);
	clipboard_id = XInternAtom(dpy, "CLIPBOARD", False);
	sel_data_id = XInternAtom(dpy, "XSEL_DATA", False);
	incr_id = XInternAtom(dpy, "INCR", False);

	pthread_t serve_thread;
	pthread_create(&serve_thread, 0, clipboard_serve, 0);
}

char* clipboard_get(void)
{
	if (clipboard_owner) return clipboard_data;
	get_clip = STATE_HIGH;
	STATE_AWAIT(get_clip, STATE_LOW, { return clipboard_data; });
}

void clipboard_set(char* data)
{
	strcpy(clipboard_data, data);
	set_clip = STATE_HIGH;
}

#endif