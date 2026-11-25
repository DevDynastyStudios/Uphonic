#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define UPL_WINDOW_API CLAP_WINDOW_API_X11
#define UPL_USE_HOST_TIMER 1
#define UPL_WINDOW_LOGICAL_SIZE 0

#define UPL_X11_DIALOG_PATH_SIZE 4096
#define UPL_X11_DIALOG_TEXT_SIZE 1024

extern char **environ;

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

typedef struct
{
	pid_t pid;
	int fd;
	size_t length;
	bool overflow;
	char path[UPL_X11_DIALOG_PATH_SIZE];
}
Upl_X11FileDialog;

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

static void upl__x11_dialog_patterns(const Upl_FileDialog *dialog, char *out, size_t size)
{
	size_t length = 0;
	out[0] = '\0';

	for (const char *const *extension = dialog->extensions; extension && *extension && length < size; extension++)
		length += (size_t)snprintf(out + length, size - length, "%s*.%s", length ? " " : "", *extension);
}

static bool upl__x11_spawn(char *const argv[], int output, pid_t *pid)
{
	posix_spawn_file_actions_t actions;
	posix_spawn_file_actions_init(&actions);
	posix_spawn_file_actions_adddup2(&actions, output, STDOUT_FILENO);
	posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);

	const int result = posix_spawnp(pid, argv[0], &actions, NULL, argv, environ);
	posix_spawn_file_actions_destroy(&actions);
	return result == 0;
}

static bool upl__x11_spawn_dialog(const Upl_FileDialog *dialog, int output, pid_t *pid)
{
	const char *title = dialog->title ? dialog->title : "Open File";
	const char *filter_name = dialog->filter_name ? dialog->filter_name : "Supported files";
	const bool filtered = dialog->extensions && dialog->extensions[0];

	char patterns[UPL_X11_DIALOG_TEXT_SIZE];
	char zenity_title[UPL_X11_DIALOG_TEXT_SIZE];
	char zenity_filter[UPL_X11_DIALOG_TEXT_SIZE];
	char kdialog_filter[UPL_X11_DIALOG_TEXT_SIZE];

	upl__x11_dialog_patterns(dialog, patterns, sizeof(patterns));
	snprintf(zenity_title, sizeof(zenity_title), "--title=%s", title);
	snprintf(zenity_filter, sizeof(zenity_filter), "--file-filter=%s | %s", filter_name, patterns);
	snprintf(kdialog_filter, sizeof(kdialog_filter), "%s|%s", patterns, filter_name);

	char *const zenity[] = {
		"zenity", "--file-selection", zenity_title,
		filtered ? zenity_filter : NULL,
		"--file-filter=All files | *",
		NULL
	};

	char *const kdialog[] = {
		"kdialog", "--title", (char *)title, "--getopenfilename", ".",
		filtered ? kdialog_filter : NULL,
		NULL
	};

	return upl__x11_spawn(zenity, output, pid) || upl__x11_spawn(kdialog, output, pid);
}

static bool upl__file_dialog_begin(Upl_Plugin *plugin, const Upl_FileDialog *dialog)
{
	int fds[2];
	if (pipe(fds) != 0)
		return false;

	fcntl(fds[0], F_SETFD, FD_CLOEXEC);
	fcntl(fds[1], F_SETFD, FD_CLOEXEC);

	Upl_X11FileDialog *state = (Upl_X11FileDialog *)calloc(1, sizeof(Upl_X11FileDialog));
	if (!state || !upl__x11_spawn_dialog(dialog, fds[1], &state->pid))
	{
		free(state);
		close(fds[0]);
		close(fds[1]);
		return false;
	}

	close(fds[1]);
	fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL) | O_NONBLOCK);
	state->fd = fds[0];
	plugin->file_dialog.platform = state;
	return true;
}

static void upl__file_dialog_poll(Upl_Plugin *plugin)
{
	Upl_X11FileDialog *state = (Upl_X11FileDialog *)plugin->file_dialog.platform;
	if (!state)
		return;

	for (;;)
	{
		char discard[256];
		const size_t room = sizeof(state->path) - 1 - state->length;
		char *target = room > 0 ? state->path + state->length : discard;
		const ssize_t count = read(state->fd, target, room > 0 ? room : sizeof(discard));

		if (count > 0)
		{
			if (room > 0)
				state->length += (size_t)count;
			else
				state->overflow = true;

			continue;
		}

		if (count < 0 && errno == EINTR)
			continue;

		if (count < 0 && errno == EAGAIN)
			return;

		break;
	}

	int status = 0;
	const bool reaped = waitpid(state->pid, &status, 0) == state->pid;
	const bool accepted = !state->overflow && (!reaped || (WIFEXITED(status) && WEXITSTATUS(status) == 0));

	char path[UPL_X11_DIALOG_PATH_SIZE];
	size_t length = state->length;
	while (length > 0 && (state->path[length - 1] == '\n' || state->path[length - 1] == '\r'))
		length--;

	memcpy(path, state->path, length);
	path[length] = '\0';

	close(state->fd);
	free(state);
	plugin->file_dialog.platform = NULL;
	upl__file_dialog_finish(plugin, accepted ? path : NULL);
}

static void upl__file_dialog_cancel(Upl_Plugin *plugin)
{
	Upl_X11FileDialog *state = (Upl_X11FileDialog *)plugin->file_dialog.platform;
	if (state)
	{
		kill(state->pid, SIGTERM);
		waitpid(state->pid, NULL, 0);
		close(state->fd);
		free(state);
		plugin->file_dialog.platform = NULL;
	}

	upl__file_dialog_finish(plugin, NULL);
}
