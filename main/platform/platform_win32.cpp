#include "platform.h"

#if UPH_PLATFORM_WINDOWS

#include "event.h"
#include "event_types.h"

#include <cstdint>
#include <assert.h>

#include <windows.h>
#include <windowsx.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#include <d3d11.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

typedef struct UphWin32Platform
{
    HINSTANCE h_instance;
    HWND hwnd;
    uint32_t window_width, window_height;

    ID3D11Device *device;
    ID3D11DeviceContext *device_context;
    IDXGISwapChain *swap_chain;
    ID3D11RenderTargetView *render_target_view;
}
UphWin32Platform;

static UphWin32Platform *platform;
static double clock_frequency;
static LARGE_INTEGER start_time;

LRESULT CALLBACK win32_process_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

static void create_wide_string_from_utf8(const char *source, WCHAR *target)
{
    uint32_t count;
    count = MultiByteToWideChar(CP_UTF8, 0, source, -1, NULL, 0);
    if (!count)
    {
        MessageBoxA(0, "Failed to convert string from UTF-8", "Error", MB_ICONEXCLAMATION | MB_OK);
        return;
    }
    if (!MultiByteToWideChar(CP_UTF8, 0, source, -1, target, count))
    {
        MessageBoxA(NULL, "Failed to convert string from UTF-8", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return;
    }
}

static void create_render_target(void)
{
    ID3D11Texture2D* back_buffer = nullptr;
    platform->swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    platform->device->CreateRenderTargetView(back_buffer, NULL, &platform->render_target_view);
    back_buffer->Release();
}

static void cleanup_render_target(void)
{
    if (platform->render_target_view) { platform->render_target_view->Release(); platform->render_target_view = nullptr; }
}

static bool create_device_d3d(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
        createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
        &platform->swap_chain, &platform->device, &featureLevel, &platform->device_context) != S_OK)
        return false;

    create_render_target();
    return true;
}

static void cleanup_device_d3d()
{
    cleanup_render_target();
    if (platform->swap_chain) { platform->swap_chain->Release(); platform->swap_chain = nullptr; }
    if (platform->device_context) { platform->device_context->Release(); platform->device_context = nullptr; }
    if (platform->device) { platform->device->Release(); platform->device = nullptr; }
}

void uph_platform_initialize(const UphPlatformCreateInfo *create_info)
{
    platform = (UphWin32Platform*)malloc(sizeof(UphWin32Platform));
    platform->h_instance = GetModuleHandleA(0);

    ImGui_ImplWin32_EnableDpiAwareness();

    HICON icon = LoadIcon(platform->h_instance, IDI_APPLICATION);
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = win32_process_message;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = platform->h_instance;
    wc.hIcon = icon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = "uph_window_class";

    RegisterClassA(&wc);

    RECT wr = { 0, 0, (LONG)create_info->width, (LONG)create_info->height };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    WCHAR wide_title[128];
    create_wide_string_from_utf8(create_info->title, wide_title);

    platform->hwnd = CreateWindowA(
        wc.lpszClassName, (LPCSTR)wide_title,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        NULL, NULL, wc.hInstance, NULL);

    platform->window_width = create_info->width;
    platform->window_height = create_info->height;

    if (!create_device_d3d(platform->hwnd))
    {
        cleanup_device_d3d();
        UnregisterClassA(wc.lpszClassName, wc.hInstance);
        return;
    }

    ShowWindow(platform->hwnd, SW_SHOWDEFAULT);
    UpdateWindow(platform->hwnd);

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    clock_frequency = 1.0 / (double)frequency.QuadPart;
    QueryPerformanceCounter(&start_time);

    ImGui_ImplWin32_Init(platform->hwnd);
    ImGui_ImplDX11_Init(platform->device, platform->device_context);

    wc = {0};
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = platform->h_instance;
    wc.lpszClassName = "uph_child_window_class";
    RegisterClassA(&wc);
}

void uph_platform_shutdown(void)
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();

    cleanup_device_d3d();
    DestroyWindow(platform->hwnd);
    UnregisterClassA("uph_window_class", platform->h_instance);
    UnregisterClassA("uph_child_window_class", platform->h_instance);

    free(platform);
}

void uph_platform_begin(void)
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
}

void uph_platform_end(void)
{
    const float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    platform->device_context->OMSetRenderTargets(1, &platform->render_target_view, NULL);
    platform->device_context->ClearRenderTargetView(platform->render_target_view, clear_color);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    platform->swap_chain->Present(1, 0);
}

double uph_get_time(void)
{
    LARGE_INTEGER now_time;
    QueryPerformanceCounter(&now_time);
    return (double)(now_time.QuadPart - start_time.QuadPart) * clock_frequency;
}

UphChildWindow uph_create_child_window(const UphChildWindowCreateInfo *create_info)
{
    UphChildWindow window;

    WCHAR wide_title[128];
    create_wide_string_from_utf8(create_info->title, wide_title);

    HWND hwndPlugin = CreateWindowExA(
        0,
        "uph_child_window_class",
        (LPCSTR)wide_title,
        WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        create_info->width, create_info->height + 32,
        platform->hwnd,
        NULL,
        platform->h_instance,
        NULL
    );

    LONG style = GetWindowLong(hwndPlugin, GWL_STYLE);
    style &= ~WS_MAXIMIZEBOX;
    style &= ~WS_SIZEBOX;
    SetWindowLong(hwndPlugin, GWL_STYLE, style);

    HMENU hSysMenu = GetSystemMenu(hwndPlugin, FALSE);
    if (hSysMenu)
        DeleteMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND);

    LONG exStyle = GetWindowLong(hwndPlugin, GWL_EXSTYLE);
    SetWindowLong(hwndPlugin, GWL_EXSTYLE, exStyle);

    ShowWindow(hwndPlugin, SW_SHOWDEFAULT);
    UpdateWindow(hwndPlugin);

    window.handle = hwndPlugin;

    return window;
}

void uph_destroy_child_window(const UphChildWindow *window)
{
    DestroyWindow((HWND)window->handle);
}

UphLibrary uph_load_library(const char *path)
{
    return (UphLibrary)LoadLibraryA(path);
}

void uph_unload_library(UphLibrary library)
{
    FreeLibrary((HMODULE)library);
}

UphProcAddress uph_get_proc_address(UphLibrary library, const char *name)
{
    return (UphProcAddress)GetProcAddress((HMODULE)library, name);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK win32_process_message(HWND hwnd, uint32_t msg, WPARAM w_param, LPARAM l_param)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, w_param, l_param))
        return true;

    switch (msg)
    {
        case WM_ERASEBKGND:
            return 1;
        break;
        case WM_CLOSE:
        {
            UphQuitEvent data;
            uph_event_call(UphSystemEventCode::Quit, (void*)&data);
        }
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
        {
            RECT r;
            GetClientRect(hwnd, &r);

            platform->window_width = r.right - r.left; 
            platform->window_height = r.bottom - r.top; 

            if (platform && platform->device != NULL && w_param != SIZE_MINIMIZED)
            {
                cleanup_render_target();
                platform->swap_chain->ResizeBuffers(0, (UINT)LOWORD(l_param), (UINT)HIWORD(l_param), DXGI_FORMAT_UNKNOWN, 0);
                create_render_target();
            }
        }
        break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            bool pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);

            UphKeyEvent data;
            data.key = (UphKey)w_param;
            uph_event_call(pressed ? UphSystemEventCode::KeyPressed : UphSystemEventCode::KeyReleased, (void*)&data);
        }
        break;
        case WM_CHAR:
        {
            UphCharEvent data;
            data.ch = (char)w_param;
            uph_event_call(UphSystemEventCode::Char, (void*)&data);
        }
        break;
        default:
            return DefWindowProc(hwnd, msg, w_param, l_param);
    }
}

#endif