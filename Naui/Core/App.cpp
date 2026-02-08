#include "App.h"
#include "Panel.h"
#include "Theme.h"
#include "Style.h"
#include "AssetManager.h"
#include "Defer.h"
#include "Debug.h"
#include "Platform/Window.h"
#include "Renderer/Renderer.h"

#include <cstdint>

namespace Naui
{

class PanelRenderer
{
private:
    static void Render(void);
    friend class App;
};

void PanelRenderer::Render(void)
{
    auto &panels = GetAllPanels();
    for (const auto &[id, panel_ptr] : panels)
    {
        Naui::Panel& panel = *panel_ptr;

        if (!panel.m_open)
            continue;

        ImGui::SetNextWindowSizeConstraints(panel.m_minSize, panel.m_maxSize);
        ImGui::Begin(panel.GetTitle().c_str(), panel.m_closable ? &panel.m_open : nullptr, panel.m_imguiFlags);
        panel.OnRender();
        ImGui::End();
    }
}

static void ImGuiInitialize(void)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.IniFilename = nullptr;

	Style::LoadDefault();
}

static void ImGuiShutdown(void)
{
    ImGui::DestroyContext();
}

void App::Render(void)
{
    m_window->PollEvents();
    m_renderer->Begin();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport();

	OnRender();
    PanelRenderer::Render();
    Debug::Render();

    ImGui::EndFrame();
    ImGui::Render();
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
	
    m_renderer->End();
}

void App::Run(std::string title, int width, int height)
{
    ImGuiInitialize();
    Theme::LoadDefault();

    m_window = CreatePlatformWindow(width, height, title.c_str());
    m_renderer = CreateRenderer(*m_window);
    m_window->Show(true);
    AssetManager::Initialize(*m_renderer);
    m_window->SetResizeEvent([&](uint32_t resize_width, uint32_t resize_height)
    {
        m_renderer->Resize(resize_width, resize_height);
        Render();
    });
    m_window->SetFileDropEvent([&](const char *path)
    {
        OnFileDrop(path);
    });
    OnEnter();
    while (m_window->IsOpen())
	{
		Render();
		Defer::Process();
	}

    OnExit();
    DestroyAllPanels();
	Defer::Flush();
    AssetManager::Shutdown(*m_renderer);
    delete m_renderer;
    delete m_window;
    ImGuiShutdown();
}

}
