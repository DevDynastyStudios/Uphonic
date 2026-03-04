#include "MainMenuBar.h"
#include "Naui.h"
#include "Naui/FileSystem/File.h"
#include "Naui/FileSystem/FileDialog.h"
#include "Layout.h"
#include "Core/ProjectState.h"
#include "Core/ProjectManager.h"
#include "Core/Defer.h"
#include "Audio/AudioEngine.h"
#include "UI/SongTimeline.h"
#include "Actions/Project/SaveProjectAction.h"

static char newLayoutName[64] = {};
static bool popupFocusRequest = false;

static bool requestSaveAsPopup = false;
static bool requestOverridePopup = false;

static std::string pendingOverrideName;
static std::string currentLayout = "Default";

void MainMenuBar::FileMenu()
{
	if (!ImGui::BeginMenu(Naui::TR("menu.file")))
		return;

	if (ImGui::MenuItem(Naui::TR("menu.file.new")))
		ProjectManager::NewProject();

	if (ImGui::MenuItem(Naui::TR("menu.file.open")))
		FileDialog::OpenFile("open_project", "Open Project", ".uph");

	ImGui::Separator();

	if(ImGui::MenuItem(Naui::TR("menu.file.save")))
		ProjectState::GetInstance().actionManager.ExecuteWithoutHistory<SaveProjectAction>(ProjectState::GetInstance());

	if(ImGui::MenuItem(Naui::TR("menu.file.save_as")))
		FileDialog::SaveFile("save_project", "Save Project", ".uph");

	ImGui::Separator();

	if (ImGui::BeginMenu(Naui::TR("menu.file.import")))
	{
		ImGui::MenuItem(Naui::TR("menu.file.import.midi"), nullptr, nullptr, false);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu(Naui::TR("menu.file.export")))
	{
		const char* exportText = "Export As";
		if (ImGui::MenuItem(Naui::TR("menu.file.export.wav")))
			FileDialog::SaveFile("export_project", "Export To Wav", "*.wav", exportText);

		if(ImGui::MenuItem(Naui::TR("menu.file.export.ogg"), nullptr, nullptr, false))
			FileDialog::SaveFile("export_project", "Export To Ogg", "*.ogg", exportText);

		if(ImGui::MenuItem(Naui::TR("menu.file.export.mp3"), nullptr, nullptr, false))
			FileDialog::SaveFile("export_project", "Export To Mp3", "*.mp3", exportText);

		if(ImGui::MenuItem(Naui::TR("menu.file.export.flac"), nullptr, nullptr, false))
			FileDialog::SaveFile("export_project", "Export To FLAC", "*.flac", exportText);

		if(ImGui::MenuItem(Naui::TR("menu.file.export.m4a"), nullptr, nullptr, false))
			FileDialog::SaveFile("export_project", "Export To M4A", "*.m4a", exportText);

		if(ImGui::MenuItem(Naui::TR("menu.file.export.midi"), nullptr, nullptr, false))
			FileDialog::SaveFile("export_project", "Export To MIDI", "*.midi", exportText);

		ImGui::EndMenu();
	}

	ImGui::Separator();

	if (ImGui::MenuItem(Naui::TR("menu.file.exit")))
		ProjectManager::CloseProject();

	ImGui::EndMenu();
}

void MainMenuBar::EditMenu()
{
	Naui::ActionManager& actionManager = ProjectState::GetInstance().actionManager;
	if (!ImGui::BeginMenu(Naui::TR("menu.edit")))
		return;

	if(ImGui::MenuItem(Naui::TR("menu.edit.undo"), nullptr, nullptr, actionManager.CanUndo()))
	{
		actionManager.Undo();
	}

	if(ImGui::MenuItem(Naui::TR("menu.edit.redo"), nullptr, nullptr, actionManager.CanRedo()))
	{
		actionManager.Redo();
	}

	ImGui::Separator();
	ImGui::MenuItem(Naui::TR("menu.edit.cut"), nullptr, nullptr, false);
	ImGui::MenuItem(Naui::TR("menu.edit.copy"), nullptr, nullptr, false);
	ImGui::MenuItem(Naui::TR("menu.edit.paste"), nullptr, nullptr, false);

	ImGui::EndMenu();
}

void MainMenuBar::OptionsMenu()		// (Chimpchi): Old abandoned code. Should probably do something with this.
{
	if (!ImGui::BeginMenu("Options"))
		return;

	ImGui::MenuItem("MIDI Settings");
	ImGui::MenuItem("Audio Settings");
	ImGui::MenuItem("General Settings");

	ImGui::EndMenu();
}

void MainMenuBar::ViewMenu()
{
	if (!ImGui::BeginMenu(Naui::TR("menu.view")))
		return;

	if (ImGui::MenuItem(Naui::TR("menu.view.show")))
	{
		for (auto& [id, panelPtr] : Naui::GetAllPanels())
		{
			if(panelPtr->GetWindowFlags() & ImGuiWindowFlags_NoSavedSettings)
				continue;

			panelPtr->SetOpen(true);
		}
	}

	if (ImGui::MenuItem(Naui::TR("menu.view.hide")))
	{
		for (auto& [id, panelPtr] : Naui::GetAllPanels())
		{
			if(panelPtr->GetWindowFlags() & ImGuiWindowFlags_NoSavedSettings)
				continue;

			panelPtr->SetOpen(false);
		}
	}

	ImGui::Separator();

	for (auto& [id, panelPtr] : Naui::GetAllPanels())
	{
		if(panelPtr->GetWindowFlags() & ImGuiWindowFlags_NoSavedSettings)
			continue;

		Naui::Panel& panel = *panelPtr;
		ImGui::PushID((int)id);

		if (ImGui::MenuItem(panel.GetTitle().c_str(), nullptr, panel.IsOpen()))
			panel.SetOpen(!panel.IsOpen());

		ImGui::PopID();
	}

	ImGui::EndMenu();
}

void MainMenuBar::LayoutMenu()
{
	if (!ImGui::BeginMenu(Naui::TR("menu.layout")))
		return;

	for (const std::filesystem::path& layoutName : Layout::GetSystemLayouts())
	{
		std::string name = layoutName.string();
		std::string label = name + "###System_" + name;
		bool selected = (label == currentLayout);
	
		if (ImGui::MenuItem(label.c_str(), nullptr, selected))
		{
			currentLayout = label;
			Naui::Defer::Add(Layout::Load, name);
		}
	}
	
	if(Layout::GetUserLayouts().size() > 0)
		ImGui::Separator();
	
	for (const std::filesystem::path& layoutName : Layout::GetUserLayouts())
	{
		std::string name = layoutName.string();
		std::string label = name + "###User_" + name;
		bool selected = (name == currentLayout);
	
		if (ImGui::MenuItem(label.c_str(), nullptr, selected))
		{
			currentLayout = name;
			Naui::Defer::Add(Layout::Load, name);
		}
	}

	ImGui::Separator();

	if (ImGui::MenuItem(Naui::TR("menu.layout.save")))
	{
		newLayoutName[0] = '\0';
		popupFocusRequest = true;
		requestSaveAsPopup = true;
	}

	auto userLayouts = Layout::GetUserLayouts();
	if (ImGui::BeginMenu(Naui::TR("menu.layout.delete"), !userLayouts.empty()))
	{
		for (const std::filesystem::path layoutName : userLayouts)
		{
			std::string name = layoutName.string();
			std::string label = name + "###User_" + name;
			if (ImGui::MenuItem(name.c_str()))
			{
				Layout::Delete(name);
				if (currentLayout == label)
					currentLayout = "Default###User_Default";
			}
		}

		ImGui::EndMenu();
	}

	ImGui::EndMenu();
}

void MainMenuBar::HelpMenu()
{
	if (!ImGui::BeginMenu(Naui::TR("menu.help")))
		return;

	ImGui::MenuItem(Naui::TR("menu.help.tutorials"));
	ImGui::MenuItem(Naui::TR("menu.help.about"));

	ImGui::EndMenu();
}

void MainMenuBar::RenderPopups()
{
	if (requestSaveAsPopup)
	{
		ImGui::OpenPopup(Naui::TR("menu.layout.popup.save"));
		requestSaveAsPopup = false;
	}

	if (requestOverridePopup)
	{
		ImGui::OpenPopup(Naui::TR("menu.layout.popup.override"));
		requestOverridePopup = false;
	}

	if (ImGui::BeginPopup(Naui::TR("menu.layout.popup.save")))
	{
		if (popupFocusRequest)
		{
			ImGui::SetKeyboardFocusHere();
			popupFocusRequest = false;
		}

		ImGui::InputText("Layout Name", newLayoutName, IM_ARRAYSIZE(newLayoutName));

		if (ImGui::Button(Naui::TR("menu.layout.popup.save_btn")) || ImGui::IsKeyPressed(ImGuiKey_Enter))
		{
			std::string name = newLayoutName;
			if (Layout::Exists(name))
			{
				pendingOverrideName = name;
				requestOverridePopup = true;
				ImGui::CloseCurrentPopup();
			}
			else
			{
				Layout::Save(name, false);
				currentLayout = name;
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::SameLine();

		if (ImGui::Button(Naui::TR("menu.layout.popup.cancel_btn")))
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup(Naui::TR("menu.layout.popup.override")))
	{
		ImGui::Text("A layout named '%s' already exists.", pendingOverrideName.c_str());
		ImGui::Text("Do you want to overwrite it?");

		if (ImGui::Button(Naui::TR("menu.layout.popup.overwrite")))
		{
			Layout::Save(pendingOverrideName, true);
			currentLayout = pendingOverrideName;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button(Naui::TR("menu.layout.popup.cancel")))
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}
}
