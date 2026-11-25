#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>

#define UPL_WINDOW_API CLAP_WINDOW_API_WIN32
#define UPL_USE_HOST_TIMER 0
#define UPL_WINDOW_LOGICAL_SIZE 0

#define UPL_WIN32_TIMER_ID 1

typedef struct
{
	HWND hwnd;
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

static Upl_Win32State upl__win32_state;

static double upl__time(void)
{
	LARGE_INTEGER frequency, counter;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&counter);
	return (double)counter.QuadPart / (double)frequency.QuadPart;
}

static void upl__win32_blit(Upl_Plugin *plugin, HDC dc)
{
	const Upl_Gui *gui = &plugin->gui;
	if (!gui->pixels)
		return;

	BITMAPINFO info = { 0 };
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth = gui->width;
	info.bmiHeader.biHeight = -gui->height;
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;
	SetDIBitsToDevice(dc, 0, 0, (DWORD)gui->width, (DWORD)gui->height, 0, 0, 0, (UINT)gui->height, gui->pixels, &info, DIB_RGB_COLORS);
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
			HDC dc = BeginPaint(hwnd, &paint);
			upl__win32_blit(plugin, dc);
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

	if (!window->hwnd)
	{
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
}

static void upl__window_set_visible(Upl_Plugin *plugin, bool visible)
{
	Upl_Win32Window *window = (Upl_Win32Window *)plugin->gui.window;
	ShowWindow(window->hwnd, visible ? SW_SHOWNA : SW_HIDE);
}

static void upl__window_present(Upl_Plugin *plugin)
{
	Upl_Win32Window *window = (Upl_Win32Window *)plugin->gui.window;
	HDC dc = GetDC(window->hwnd);
	upl__win32_blit(plugin, dc);
	ReleaseDC(window->hwnd, dc);
}

static void upl__window_poll(Upl_Plugin *plugin)
{
	(void)plugin;
}
