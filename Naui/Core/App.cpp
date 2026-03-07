#include "App.h"
#include "Panel.h"
#include "PanelRenderer.h"
#include "Modal.h"
#include "ModalRenderer.h"
#include "Theme.h"
#include "AssetManager.h"
#include "Defer.h"
#include "Debug.h"
#include "Platform/Window.h"
#include "Renderer/Renderer.h"

#include <cstdint>

namespace Naui {

static void ImGuiInitialize()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.IniFilename = nullptr;
}

static void ImGuiShutdown()
{
	ImGui::DestroyContext();
}

void App::Render()
{
	m_window->PollEvents();
	m_renderer->Begin();
	ImGui::NewFrame();
	ImGui::DockSpaceOverViewport();

	OnRender();
	PanelRenderer::Render();
	ModalRenderer::Render();
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
	m_window->SetFileDropEvent([&](const char* path)
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
	DestroyAllModals();
	DestroyAllPanels();
	Defer::Flush();
	AssetManager::Shutdown(*m_renderer);
	delete m_renderer;
	delete m_window;
	ImGuiShutdown();
}

}