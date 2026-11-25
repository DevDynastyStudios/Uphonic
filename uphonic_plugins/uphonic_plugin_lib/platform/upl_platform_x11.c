#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <time.h>

#define UPL_WINDOW_API CLAP_WINDOW_API_X11
#define UPL_USE_HOST_TIMER 1
#define UPL_WINDOW_LOGICAL_SIZE 0

typedef struct
{
	Display *display;
	Window window;
	XImage *image;
	GC gc;
	int32_t image_width;
	int32_t image_height;
}
Upl_X11Window;

static double upl__time(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static void upl__x11_release_image(Upl_X11Window *window)
{
	if (!window->image)
		return;

	window->image->data = NULL;
	XDestroyImage(window->image);
	window->image = NULL;
}

static bool upl__window_create(Upl_Plugin *plugin, const clap_window_t *parent)
{
	if (strcmp(parent->api, CLAP_WINDOW_API_X11) != 0)
		return false;

	Display *display = XOpenDisplay(NULL);
	if (!display)
		return false;

	const int screen = DefaultScreen(display);
	XSetWindowAttributes attributes = {
		.background_pixel = 0,
		.border_pixel = 0,
		.colormap = DefaultColormap(display, screen),
		.event_mask = ExposureMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask | StructureNotifyMask
	};

	const Window handle = XCreateWindow(
		display,
		(Window)parent->x11,
		0, 0,
		(unsigned int)plugin->gui.width, (unsigned int)plugin->gui.height,
		0,
		DefaultDepth(display, screen),
		InputOutput,
		DefaultVisual(display, screen),
		CWBackPixel | CWBorderPixel | CWColormap | CWEventMask,
		&attributes
	);

	if (!handle)
	{
		XCloseDisplay(display);
		return false;
	}

	Upl_X11Window *window = (Upl_X11Window *)calloc(1, sizeof(Upl_X11Window));
	window->display = display;
	window->window = handle;
	window->gc = XCreateGC(display, handle, 0, NULL);
	plugin->gui.window = window;

	if (plugin->gui.visible)
		XMapWindow(display, handle);

	XFlush(display);
	return true;
}

static void upl__window_destroy(Upl_Plugin *plugin)
{
	Upl_X11Window *window = (Upl_X11Window *)plugin->gui.window;
	if (!window)
		return;

	upl__x11_release_image(window);
	XFreeGC(window->display, window->gc);
	XDestroyWindow(window->display, window->window);
	XCloseDisplay(window->display);
	free(window);
	plugin->gui.window = NULL;
}

static void upl__window_resize(Upl_Plugin *plugin)
{
	Upl_X11Window *window = (Upl_X11Window *)plugin->gui.window;
	upl__x11_release_image(window);
	XResizeWindow(window->display, window->window, (unsigned int)plugin->gui.width, (unsigned int)plugin->gui.height);
	XFlush(window->display);
}

static void upl__window_set_visible(Upl_Plugin *plugin, bool visible)
{
	Upl_X11Window *window = (Upl_X11Window *)plugin->gui.window;
	if (visible)
		XMapRaised(window->display, window->window);
	else
		XUnmapWindow(window->display, window->window);

	XFlush(window->display);
}

static void upl__window_present(Upl_Plugin *plugin)
{
	Upl_X11Window *window = (Upl_X11Window *)plugin->gui.window;
	Upl_Gui *gui = &plugin->gui;

	if (window->image && (window->image_width != gui->width || window->image_height != gui->height))
		upl__x11_release_image(window);

	if (!window->image)
	{
		const int screen = DefaultScreen(window->display);
		window->image = XCreateImage(window->display, DefaultVisual(window->display, screen), (unsigned int)DefaultDepth(window->display, screen), ZPixmap, 0, NULL, (unsigned int)gui->width, (unsigned int)gui->height, 32, 0);
		if (!window->image)
			return;

		window->image_width = gui->width;
		window->image_height = gui->height;
	}

	window->image->data = (char *)gui->pixels;
	XPutImage(window->display, window->window, window->gc, window->image, 0, 0, 0, 0, (unsigned int)gui->width, (unsigned int)gui->height);
	XFlush(window->display);
}

static Upl_MouseButton upl__x11_button(unsigned int button)
{
	switch (button)
	{
		case Button1: return UPL_MOUSE_LEFT;
		case Button2: return UPL_MOUSE_MIDDLE;
		case Button3: return UPL_MOUSE_RIGHT;
		default: return UPL_MOUSE_BUTTON_COUNT;
	}
}

static void upl__window_poll(Upl_Plugin *plugin)
{
	Upl_X11Window *window = (Upl_X11Window *)plugin->gui.window;
	bool expose = false;

	while (XPending(window->display))
	{
		XEvent event;
		XNextEvent(window->display, &event);

		switch (event.type)
		{
			case Expose:
				expose = true;
				break;
			case MotionNotify:
				upl__input_set_shift(plugin, (event.xmotion.state & ShiftMask) != 0);
				upl__input_mouse_move(plugin, event.xmotion.x, event.xmotion.y);
				break;
			case EnterNotify:
				upl__input_mouse_move(plugin, event.xcrossing.x, event.xcrossing.y);
				break;
			case LeaveNotify:
				upl__input_mouse_leave(plugin);
				break;
			case ButtonPress:
			case ButtonRelease:
			{
				const bool down = event.type == ButtonPress;
				upl__input_set_shift(plugin, (event.xbutton.state & ShiftMask) != 0);
				upl__input_mouse_move(plugin, event.xbutton.x, event.xbutton.y);

				if (event.xbutton.button == Button4 || event.xbutton.button == Button5)
				{
					if (down)
						upl__input_mouse_wheel(plugin, event.xbutton.button == Button4 ? 1.0f : -1.0f);
					break;
				}

				upl__input_mouse_button(plugin, upl__x11_button(event.xbutton.button), down);
				break;
			}
		}
	}

	if (expose && plugin->gui.dirty_frames == 0)
		upl__window_present(plugin);
}
