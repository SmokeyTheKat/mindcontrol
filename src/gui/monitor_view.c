#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "list.h"

#define REDRAW(widget) gtk_widget_queue_draw_area(widget, 0, 0, 1000000, 1000000)

struct screen
{
	char name[64];
	char ip[16];
	int x, y;
	int width, height;
};

struct color
{
	float r, g, b;
};

struct color colors[] = {
	{ 0.6, 0.6, 0.6 },
	{ 0.3, 0.3, 1.0 },
	{ 0.3, 1.0, 0.3 },
	{ 1.0, 0.3, 0.3 },
	{ 0.6, 0.6, 0.6 },
};

struct screen* selected = 0;
float zoom_speed = 0.1;
float zoom = 1; 
float xscale = 0.1;
float yscale = 0.1;
int width, height;
struct list screens;
static int lmx = 0;
static int lmy = 0;

gboolean c_draw(GtkWidget* widget, cairo_t* cr, void*)
{
	guint width, height;
	GdkRGBA color;
	GtkStyleContext *context;

	context = gtk_widget_get_style_context(widget);

	width = gtk_widget_get_allocated_width(widget);
	height = gtk_widget_get_allocated_height(widget);

	gtk_render_background(context, cr, 0, 0, width, height);

	cairo_set_font_size(cr, 15);
cairo_scale(cr,
			 zoom,
			 zoom);

	cairo_translate(cr,
				 -((float)width / 8.0) * (zoom - 1.0),
				 -((float)height / 8.0) * (zoom - 1.0));

	cairo_set_source_rgb(cr, 0, 0, 0); 
	cairo_rectangle(cr, 0, 0, 30, 30);
	cairo_fill(cr);

	for (list_iterate(&screens, s, struct screen))
	{
		struct color c = colors[list_pos(&screens, s, struct screen)];
		cairo_set_source_rgb(cr, c.r, c.g, c.b); 
		cairo_rectangle(cr, s->x, s->y, s->width, s->height);
		cairo_stroke(cr);
		
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); 
		cairo_move_to(cr, (s->x + 3), (s->y + 12));
		cairo_show_text(cr, s->name);
		cairo_move_to(cr, (s->x + 3), (s->y + 12 + 20));
		cairo_show_text(cr, s->ip);  
	}

	return FALSE;
}

int intersecting(struct screen a, struct screen b)
{
	int dr = b.x + b.width - a.x;
	int dl = a.x + a.width - b.x;
	int db = b.y + b.height - a.y;
	int dt = a.y + a.height - b.y;

	printf("r: %d   l: %d   b: %d   t: %d\n", dr, dl, db, dt);


	if (dr < 0 || dl < 0 || db < 0 || dt < 0)
		return 0;

	dr = ABS(dr);
	dl = ABS(dl);
	db = ABS(db);
	dt = ABS(dt);

	int mr = (dr * 10000) / a.width;
	int ml = (dl * 10000) / b.width;
	int mb = (db * 10000) / a.height;
	int mt = (dt * 10000) / b.height;

	if (mr > ml && mr > mb && mr > mt)
		return 1;
	if (ml > mr && ml > mb && ml > mt)
		return 2;
	if (mb > mr && mb > ml && mb > mt)
		return 3;
	if (mt > mr && mt > ml && mt > mb)
		return 4;
}

void center(GtkWidget* widget)
{
	struct screen* topmost = &list_first(&screens, struct screen);
	struct screen* bottommost = topmost;
	struct screen* leftmost = topmost;
	struct screen* rightmost = topmost;

	for (list_iterate(&screens, s, struct screen))
	{
		if (s->y < topmost->y)
			topmost = s;
		if (s->y > bottommost->y)
			bottommost = s;
		if (s->x < leftmost->x)
			leftmost = s;
		if (s->x > rightmost->x)
			rightmost = s;
	}

	int entire_width = (rightmost->x + rightmost->width) - leftmost->x;
	int entire_height = (bottommost->y + bottommost->height) - topmost->y;

	int dx = (width / 2) - (leftmost->x + (entire_width / 2));
	int dy = (height / 2) - (topmost->y + (entire_height / 2));


	for (list_iterate(&screens, s, struct screen))
	{
		s->x += dx;
		s->y += dy;
		printf("i: %d   dx: %d   dy: %d\n", list_pos(&screens, s, struct screen), s->x, s->y);
	}

	REDRAW(widget);
}

void update_screen(GtkWidget* widget, int x, int y)
{
	selected->x = x - selected->width / 2;
	selected->y = y - selected->height / 2;

	for (list_iterate(&screens, i, struct screen))
	{
		if (i == selected) continue;
		int itrs = intersecting(*selected, *i);
		if (itrs)
		{
			printf("INTERSECTING %d\n", itrs);
			if (itrs == 1)
				selected->x = i->x - selected->width;
			else if (itrs == 2)
				selected->x = i->x + i->width;
			else if (itrs == 3)
				selected->y = i->y - selected->height;
			else if (itrs == 4)
				selected->y = i->y + i->height;

			int snap_size = 5;

			if (selected->y >= i->y - snap_size  && selected->y <= i->y + snap_size)
				selected->y = i->y;
			else if (selected->y + selected->height >= i->y + i->height - snap_size  && selected->y + selected->height <= i->y + i->height + snap_size)
				selected->y = i->y + i->height - selected->height;

			if (selected->x >= i->x - snap_size  && selected->x <= i->x + snap_size)
				selected->x = i->x;
			else if (selected->x + selected->width >= i->x + i->width - snap_size  && selected->x + selected->width <= i->x + i->width + snap_size)
				selected->x = i->x + i->width - selected->width;
		}
	}
	
	REDRAW(widget);
}

gboolean c_motion(GtkWidget* widget, GdkEventMotion* event, void*)
{
	if (!selected)
		return FALSE;
	if (!(event->state & GDK_BUTTON1_MASK))
		return FALSE;
	int mx = event->x / zoom;
	int my = event->y / zoom;
	update_screen(widget, mx, my);
	return TRUE;
}

struct screen* get_screen_at(int x, int y)
{
	for (list_iterate(&screens, s, struct screen))
	{
		if (
			(x >= s->x && x < (s->x + s->width)) &&
			(y >= s->y && y < (s->y + s->height))
		) return s;
	}
	return 0;
}

bool is_cursor_over_client_modifier(int x, int y)
{
	return (x >= 0 && x < 30) && (y >= 0 && y < 30);
}

gboolean c_mouse_down(GtkWidget* widget, GdkEventButton* event, void*)
{
	int mx = event->x / zoom;
	int my = event->y / zoom;

	lmx = mx;
	lmy = my;

	printf("[%d, %d]\n", mx, my);

	struct screen* clicked_screen = get_screen_at(mx, my);

	if (clicked_screen)
	{
		selected = clicked_screen;
//        update_screen(widget, mx, my);
	}
	else if (is_cursor_over_client_modifier(mx, my))
	{
		list_push_back(&screens, ((struct screen){ {"new device"}, {"0.0.0.0"}, 0, 0, 192, 108 }), struct screen);
		selected = &list_last(&screens, struct screen);
		REDRAW(widget);
		lmx = -1;
		lmy = -1;
	}
	else selected = 0;

	return FALSE;
}

void edit_client(struct screen* screen)
{
	printf("edit client \"%s\"\n", screen->name);
}

gboolean c_mouse_up(GtkWidget* widget, GdkEventButton* event, void*)
{
	if (selected == 0) return FALSE;

	int mx = event->x / zoom;
	int my = event->y / zoom;

	if (mx == lmx && my == lmy)
	{
		edit_client(selected);
	}

	if (is_cursor_over_client_modifier(mx, my))
	{
//        int idx = list_index_of(&screens, *selected, struct screen);
		int idx = __list_index_of(&screens, selected, sizeof(struct screen));
		printf("remove client #%d\n", idx);
		list_remove(&screens, idx, struct screen);
	}

	center(widget);
	REDRAW(widget);
	selected = 0;
}

gboolean c_zoom(GtkWidget* widget, GdkEventScroll* event, void*)
{
	int dir = event->direction;
	printf("(%d)\n", dir);
	if (dir == GDK_SCROLL_UP)
		zoom += zoom_speed;
	else if (dir == GDK_SCROLL_DOWN)
		zoom -= zoom_speed;
	center(widget);
	REDRAW(widget);
}

gboolean c_resize(GtkWidget* widget, GtkAllocation* allocation, void*)
{
	printf("width = %d, height = %d\n", allocation->width, allocation->height);
	width = allocation->width;
	height = allocation->height;
	center(widget);
	REDRAW(widget);
}

GtkWidget* generate_monitor_view(void)
{
	screens = make_list(10, struct screen);
	list_push_back(&screens, ((struct screen){ {"william pc"}, {"192.168.1.19"}, 0, 0, 192, 108 }), struct screen);
	list_push_back(&screens, ((struct screen){ {"william laptop"}, {"192.168.1.37"}, 0, 0, 136, 76 }), struct screen);
	list_push_back(&screens, ((struct screen){ {"school laptop"}, {"192.168.1.39"}, 0, 0, 128, 72 }), struct screen);

	GtkWidget* drawing_area = gtk_drawing_area_new();
	gtk_widget_add_events(drawing_area, GDK_POINTER_MOTION_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_SCROLL_MASK);
	gtk_widget_set_size_request(drawing_area, 100, 80);
	g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(c_draw), 0);
	g_signal_connect(G_OBJECT(drawing_area), "scroll-event", G_CALLBACK(c_zoom), 0);
	g_signal_connect(G_OBJECT(drawing_area), "button-press-event", G_CALLBACK(c_mouse_down), 0);
	g_signal_connect(G_OBJECT(drawing_area), "button-release-event", G_CALLBACK(c_mouse_up), 0);
	g_signal_connect(G_OBJECT(drawing_area), "motion-notify-event", G_CALLBACK(c_motion), 0);
	g_signal_connect(G_OBJECT(drawing_area), "size-allocate", G_CALLBACK(c_resize), 0);
	return drawing_area;
}
