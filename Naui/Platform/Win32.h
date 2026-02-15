#pragma once

#include "Window.h"

#include <string>

#include <windows.h>
#include <cstdint>

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#include <imgui.h>
#include <imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Naui
{

class PlatformWin32Window : public PlatformWindow
{
public:
    PlatformWin32Window(int width, int height, const char *title, PlatformWindow *parent);
    ~PlatformWin32Window(void);

    void *GetNativeHandle(void) const override { return m_hwnd; }
    bool IsOpen(void) const override { return m_isOpen; }
    void Close(void) override { m_isOpen = false; }

    uint32_t GetWidth(void) const override { return m_width; }
    uint32_t GetHeight(void) const override { return m_height; }

    void PollEvents(void) override;
    void Show(bool value) override;

    void SetResizeEvent(std::function<void(uint32_t, uint32_t)> callback) override { m_resizeCallback = callback; }
    void SetFileDropEvent(std::function<void(const char *path)> callback) override { m_fileDropCallback = callback; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    bool SetTitlebarDarkMode(void);

    HWND m_hwnd = nullptr;
    HWND m_hwndParent = nullptr;
    uint32_t m_width;
    uint32_t m_height;
    bool m_isOpen = true;

    std::function<void(uint32_t, uint32_t)> m_resizeCallback = nullptr;
    std::function<void(const char *path)> m_fileDropCallback = nullptr;
};

LRESULT CALLBACK ChildWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    PlatformWin32Window* window = nullptr;
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lparam);
        window = static_cast<PlatformWin32Window*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }
    else
    {
        window = reinterpret_cast<PlatformWin32Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    switch (msg)
    {
        case WM_CLOSE:
            if (window)
                window->Show(false);
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK PlatformWin32Window::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{    
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
        return true;

    PlatformWin32Window* window = nullptr;
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lparam);
        window = static_cast<PlatformWin32Window*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }
    else
    {
        window = reinterpret_cast<PlatformWin32Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    switch (msg)
    {
        case WM_CLOSE:
            if (window)
                window->Close();
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_SIZE:
            if (window)
            {
                window->m_width = LOWORD(lparam);
                window->m_height = HIWORD(lparam);

                if (window->m_resizeCallback)
                    window->m_resizeCallback(window->m_width, window->m_height);
            }
            return 0;
        
        case WM_DROPFILES:
            if (window && window->m_fileDropCallback)
            {
                for (uint32_t i = 0; i < DragQueryFileA((HDROP)wparam, -1, nullptr, 0); i++)
                {
                    char path[MAX_PATH];
                    DragQueryFileA((HDROP)wparam, i, path, MAX_PATH);
                    window->m_fileDropCallback(path);
                }
            }
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

bool PlatformWin32Window::SetTitlebarDarkMode(void)
{
    if (!m_hwnd)
        return false;

    const BOOL useDark = TRUE;

    // Try attribute 20 (Windows 10 1809)
    HRESULT hr = DwmSetWindowAttribute(
        m_hwnd,
        20,
        &useDark,
        sizeof(useDark)
    );

    // If that fails, try attribute 19 (Windows 11)
    if (FAILED(hr))
    {
        hr = DwmSetWindowAttribute(
            m_hwnd,
            19,
            &useDark,
            sizeof(useDark)
        );
    }

    return SUCCEEDED(hr);
}

static void CreateWideStringFromUTF8(const char *source, WCHAR *target)
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

static uint32_t s_windowCount = 0;

PlatformWin32Window::PlatformWin32Window(int width, int height, const char *title, PlatformWindow *parent)
{
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    
    if (s_windowCount == 0)
    {
        WNDCLASSEXA wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = "NauiWin32WindowClass";
        RegisterClassExA(&wc);

        wc.lpfnWndProc = ChildWindowProc;
        wc.lpszClassName = "NauiWin32ChildWindowClass";
        RegisterClassExA(&wc);
    }
    s_windowCount++;

    if (parent)
        ImGui_ImplWin32_EnableDpiAwareness();

    m_hwndParent = parent ? static_cast<HWND>(parent->GetNativeHandle()) : nullptr;
    
    // For child windows, disable minimize and maximize buttons
    DWORD windowStyle = m_hwndParent ? 
        (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU) : 
        WS_OVERLAPPEDWINDOW;

    RECT rect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    AdjustWindowRect(&rect, windowStyle, FALSE);
    
    WCHAR wide_title[128];
    CreateWideStringFromUTF8(title, wide_title);

    m_hwnd = CreateWindowExA(
        0,
        parent ? "NauiWin32ChildWindowClass" : "NauiWin32WindowClass",
        (LPCSTR)wide_title,
        windowStyle,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        m_hwndParent,
        nullptr,
        hInstance,
        this
    );
    
    if (!m_hwnd)
    {
        m_isOpen = false;
        return;
    }

    SetTitlebarDarkMode();
    DragAcceptFiles(m_hwnd, TRUE);
    
    UpdateWindow(m_hwnd);

    if (m_hwndParent)
        return;

    ImGui::GetStyle().FontScaleDpi = ImGui_ImplWin32_GetDpiScaleForHwnd(m_hwnd);
    ImGui_ImplWin32_Init(m_hwnd);
}

PlatformWin32Window::~PlatformWin32Window(void)
{
    if (!m_hwndParent)
        ImGui_ImplWin32_Shutdown();
    
    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }

    s_windowCount--;

    if (s_windowCount == 0)
    {
        HINSTANCE hInstance = GetModuleHandle(nullptr);
        UnregisterClassA("NauiWin32WindowClass", hInstance);
        UnregisterClassA("NauiWin32ChildWindowClass", hInstance);
    }
}

void PlatformWin32Window::PollEvents(void)
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ImGui_ImplWin32_NewFrame();
}

void PlatformWin32Window::Show(bool value)
{
    ShowWindow(m_hwnd, value ? SW_SHOW : SW_HIDE);
}

}
