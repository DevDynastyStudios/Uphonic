#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>

#define UPL_WINDOW_API CLAP_WINDOW_API_WIN32
#define UPL_USE_HOST_TIMER 0
#define UPL_WINDOW_LOGICAL_SIZE 0

#define UPL_WIN32_TIMER_ID 1
#define UPL_WIN32_DIALOG_TITLE_SIZE 256
#define UPL_WIN32_DIALOG_FILTER_SIZE 1024
#define UPL_WIN32_DIALOG_PATH_SIZE 4096

typedef struct
{
	HWND hwnd;
	Upl_D2DSurface *surface;
	bool tracking_leave;
}
Upl_Win32Window;

typedef struct
{
	wchar_t class_name[64];
	uint32_t class_refs;
	HMODULE module;
}
Upl_Win32State;

typedef struct
{
	wchar_t title[UPL_WIN32_DIALOG_TITLE_SIZE];
	wchar_t filter[UPL_WIN32_DIALOG_FILTER_SIZE];
	wchar_t path[UPL_WIN32_DIALOG_PATH_SIZE];
	bool running;
	bool abandoned;
}
Upl_Win32FileDialog;

static Upl_Win32State upl__win32_state;

static double upl__time(void)
{
	LARGE_INTEGER frequency, counter;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&counter);
	return (double)counter.QuadPart / (double)frequency.QuadPart;
}

static void upl__win32_render(Upl_Plugin *plugin)
{
	Upl_Win32Window *window = (Upl_Win32Window *)plugin->gui.window;
	if (!window->surface || !upl__d2d_begin(window->surface))
		return;

	upl__gui_draw(plugin);
	upl__d2d_end(window->surface);
}

static void upl__win32_button(Upl_Plugin *plugin, HWND hwnd, LPARAM lparam, WPARAM wparam, Upl_MouseButton button, bool down)
{
	upl__input_set_shift(plugin, (wparam & MK_SHIFT) != 0);
	upl__input_mouse_move(plugin, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
	upl__input_mouse_button(plugin, button, down);

	if (down)
		SetCapture(hwnd);
	else if (!(wparam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
		ReleaseCapture();
}

static LRESULT CALLBACK upl__win32_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	if (message == WM_NCCREATE)
	{
		const CREATESTRUCTW *create = (const CREATESTRUCTW *)lparam;
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)create->lpCreateParams);
	}

	Upl_Plugin *plugin = (Upl_Plugin *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	if (!plugin || !plugin->gui.window)
		return DefWindowProcW(hwnd, message, wparam, lparam);

	Upl_Win32Window *window = (Upl_Win32Window *)plugin->gui.window;
	switch (message)
	{
		case WM_TIMER:
			if (wparam == UPL_WIN32_TIMER_ID)
				upl__gui_tick(plugin);
			return 0;
		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			BeginPaint(hwnd, &paint);
			upl__win32_render(plugin);
			EndPaint(hwnd, &paint);
			return 0;
		}
		case WM_ERASEBKGND:
			return 1;
		case WM_MOUSEMOVE:
			if (!window->tracking_leave)
			{
				TRACKMOUSEEVENT track = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
				TrackMouseEvent(&track);
				window->tracking_leave = true;
			}
			upl__input_set_shift(plugin, (wparam & MK_SHIFT) != 0);
			upl__input_mouse_move(plugin, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
			return 0;
		case WM_MOUSELEAVE:
			window->tracking_leave = false;
			upl__input_mouse_leave(plugin);
			return 0;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
			upl__win32_button(plugin, hwnd, lparam, wparam, UPL_MOUSE_LEFT, true);
			return 0;
		case WM_LBUTTONUP:
			upl__win32_button(plugin, hwnd, lparam, wparam, UPL_MOUSE_LEFT, false);
			return 0;
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
			upl__win32_button(plugin, hwnd, lparam, wparam, UPL_MOUSE_RIGHT, true);
			return 0;
		case WM_RBUTTONUP:
			upl__win32_button(plugin, hwnd, lparam, wparam, UPL_MOUSE_RIGHT, false);
			return 0;
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
			upl__win32_button(plugin, hwnd, lparam, wparam, UPL_MOUSE_MIDDLE, true);
			return 0;
		case WM_MBUTTONUP:
			upl__win32_button(plugin, hwnd, lparam, wparam, UPL_MOUSE_MIDDLE, false);
			return 0;
		case WM_MOUSEWHEEL:
			upl__input_set_shift(plugin, (GET_KEYSTATE_WPARAM(wparam) & MK_SHIFT) != 0);
			upl__input_mouse_wheel(plugin, (float)GET_WHEEL_DELTA_WPARAM(wparam) / (float)WHEEL_DELTA);
			return 0;
		case WM_CAPTURECHANGED:
			for (Upl_MouseButton button = 0; button < UPL_MOUSE_BUTTON_COUNT; button++)
				upl__input_mouse_button(plugin, button, false);
			return 0;
	}

	return DefWindowProcW(hwnd, message, wparam, lparam);
}

static bool upl__win32_register_class(void)
{
	Upl_Win32State *state = &upl__win32_state;
	if (state->class_refs++ > 0)
		return true;

	GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)(void *)&upl__win32_proc, &state->module);
	swprintf(state->class_name, sizeof(state->class_name) / sizeof(state->class_name[0]), L"UplWindow_%p", (void *)state);

	WNDCLASSEXW window_class = { 0 };
	window_class.cbSize = sizeof(WNDCLASSEXW);
	window_class.style = CS_DBLCLKS;
	window_class.lpfnWndProc = upl__win32_proc;
	window_class.hInstance = state->module;
	window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	window_class.lpszClassName = state->class_name;

	if (!RegisterClassExW(&window_class))
	{
		state->class_refs = 0;
		return false;
	}

	return true;
}

static void upl__win32_unregister_class(void)
{
	Upl_Win32State *state = &upl__win32_state;
	if (state->class_refs == 0 || --state->class_refs > 0)
		return;

	UnregisterClassW(state->class_name, state->module);
}

static bool upl__window_create(Upl_Plugin *plugin, const clap_window_t *parent)
{
	Upl_Win32State *state = &upl__win32_state;
	if (strcmp(parent->api, CLAP_WINDOW_API_WIN32) != 0 || !upl__win32_register_class())
		return false;

	Upl_Win32Window *window = (Upl_Win32Window *)calloc(1, sizeof(Upl_Win32Window));
	plugin->gui.window = window;

	window->hwnd = CreateWindowExW(
		0,
		state->class_name,
		L"",
		WS_CHILD | WS_CLIPSIBLINGS | (plugin->gui.visible ? WS_VISIBLE : 0),
		0, 0,
		plugin->gui.width, plugin->gui.height,
		(HWND)parent->win32,
		NULL,
		state->module,
		plugin
	);

	window->surface = window->hwnd ? upl__d2d_surface_create(window->hwnd, (uint32_t)plugin->gui.width, (uint32_t)plugin->gui.height) : NULL;
	if (!window->surface)
	{
		if (window->hwnd)
		{
			SetWindowLongPtrW(window->hwnd, GWLP_USERDATA, 0);
			DestroyWindow(window->hwnd);
		}

		free(window);
		plugin->gui.window = NULL;
		upl__win32_unregister_class();
		return false;
	}

	SetTimer(window->hwnd, UPL_WIN32_TIMER_ID, UPL_FRAME_INTERVAL_MS, NULL);
	return true;
}

static void upl__window_destroy(Upl_Plugin *plugin)
{
	Upl_Win32Window *window = (Upl_Win32Window *)plugin->gui.window;
	if (!window)
		return;

	KillTimer(window->hwnd, UPL_WIN32_TIMER_ID);
	upl__d2d_surface_destroy(window->surface);
	SetWindowLongPtrW(window->hwnd, GWLP_USERDATA, 0);
	DestroyWindow(window->hwnd);
	free(window);
	plugin->gui.window = NULL;
	upl__win32_unregister_class();
}

static void upl__window_resize(Upl_Plugin *plugin)
{
	Upl_Win32Window *window = (Upl_Win32Window *)plugin->gui.window;
	SetWindowPos(window->hwnd, NULL, 0, 0, plugin->gui.width, plugin->gui.height, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
	upl__d2d_surface_resize(window->surface, (uint32_t)plugin->gui.width, (uint32_t)plugin->gui.height);
}

static void upl__window_set_visible(Upl_Plugin *plugin, bool visible)
{
	Upl_Win32Window *window = (Upl_Win32Window *)plugin->gui.window;
	ShowWindow(window->hwnd, visible ? SW_SHOWNA : SW_HIDE);
}

static void upl__window_redraw(Upl_Plugin *plugin)
{
	upl__win32_render(plugin);
}

static void upl__window_poll(Upl_Plugin *plugin)
{
	(void)plugin;
}

static void upl__win32_widen(const char *text, wchar_t *out, int size)
{
	if (MultiByteToWideChar(CP_UTF8, 0, text, -1, out, size) == 0)
		out[0] = L'\0';
}

static void upl__win32_dialog_filter(const Upl_FileDialog *dialog, wchar_t *out, int size)
{
	char filter[UPL_WIN32_DIALOG_FILTER_SIZE];
	size_t length = 0;

	if (dialog->extensions && dialog->extensions[0])
	{
		length += (size_t)snprintf(filter, sizeof(filter), "%s\n", dialog->filter_name ? dialog->filter_name : "Supported files");
		for (const char *const *extension = dialog->extensions; *extension && length < sizeof(filter); extension++)
			length += (size_t)snprintf(filter + length, sizeof(filter) - length, "%s*.%s", extension == dialog->extensions ? "" : ";", *extension);

		if (length < sizeof(filter))
			length += (size_t)snprintf(filter + length, sizeof(filter) - length, "\n");
	}

	if (length < sizeof(filter))
		snprintf(filter + length, sizeof(filter) - length, "All files\n*.*\n");

	upl__win32_widen(filter, out, size - 1);
	for (wchar_t *cursor = out; *cursor; cursor++)
	{
		if (*cursor == L'\n')
			*cursor = L'\0';
	}
}

static bool upl__file_dialog_begin(Upl_Plugin *plugin, const Upl_FileDialog *dialog)
{
	Upl_Win32FileDialog *state = (Upl_Win32FileDialog *)calloc(1, sizeof(Upl_Win32FileDialog));
	if (!state)
		return false;

	upl__win32_widen(dialog->title ? dialog->title : "Open File", state->title, UPL_WIN32_DIALOG_TITLE_SIZE);
	upl__win32_dialog_filter(dialog, state->filter, UPL_WIN32_DIALOG_FILTER_SIZE);
	plugin->file_dialog.platform = state;
	return true;
}

static void upl__file_dialog_poll(Upl_Plugin *plugin)
{
	Upl_Win32FileDialog *state = (Upl_Win32FileDialog *)plugin->file_dialog.platform;
	if (!state || state->running)
		return;

	Upl_Win32Window *window = (Upl_Win32Window *)plugin->gui.window;
	OPENFILENAMEW open = {
		.lStructSize = sizeof(open),
		.hwndOwner = window ? GetAncestor(window->hwnd, GA_ROOT) : NULL,
		.lpstrFilter = state->filter,
		.lpstrFile = state->path,
		.nMaxFile = UPL_WIN32_DIALOG_PATH_SIZE,
		.lpstrTitle = state->title,
		.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR
	};

	state->running = true;
	const bool accepted = GetOpenFileNameW(&open) != 0;
	state->running = false;

	if (state->abandoned)
	{
		free(state);
		return;
	}

	char path[UPL_WIN32_DIALOG_PATH_SIZE * 3];
	if (!accepted || WideCharToMultiByte(CP_UTF8, 0, state->path, -1, path, (int)sizeof(path), NULL, NULL) == 0)
		path[0] = '\0';

	free(state);
	plugin->file_dialog.platform = NULL;
	upl__file_dialog_finish(plugin, path);
}

static void upl__file_dialog_cancel(Upl_Plugin *plugin)
{
	Upl_Win32FileDialog *state = (Upl_Win32FileDialog *)plugin->file_dialog.platform;
	if (state && state->running)
		state->abandoned = true;
	else
		free(state);

	plugin->file_dialog.platform = NULL;
	upl__file_dialog_finish(plugin, NULL);
}
