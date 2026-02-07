#include "MainMenuBar.h"
#include "Naui/FileSystem/File.h"
#include "Naui/FileSystem/FileDialog.h"
#include "Core/ProjectState.h"
#include "Core/ProjectManager.h"
#include "Core/Defer.h"
#include "Audio/AudioEngine.h"
#include "UI/SongTimeline.h"
#include "Layout.h"

static char newLayoutName[64] = {};
static bool popupFocusRequest = false;

static bool requestSaveAsPopup = false;
static bool requestOverridePopup = false;

static std::string pendingOverrideName;
static std::string currentLayout = "Default";

void MainMenuBar::FileMenu()
{
	if (!ImGui::BeginMenu("File"))
		return;

	if (ImGui::MenuItem("New Project"))
		ProjectManager::NewProject();

	if (ImGui::MenuItem("Open"))
		FileDialog::OpenFile("open_project", "Open Project", ".uph");

	ImGui::Separator();

	if(ImGui::MenuItem("Save"))
		ProjectManager::Save();

	if(ImGui::MenuItem("Save As"))
		FileDialog::SaveFile("save_project", "Save Project", ".uph");

	ImGui::Separator();

	if (ImGui::BeginMenu("Import"))
	{
		ImGui::MenuItem("MIDI", nullptr, nullptr, false);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Export"))
	{
		const char* exportText = "Export As";
		if (ImGui::MenuItem("Wave file..."))
			FileDialog::SaveFile("export_project", "Export To Save", ".wav", exportText);

		if(ImGui::MenuItem("Ogg file...", nullptr, nullptr, false))
			FileDialog::SaveFile("export_project", "Export To Ogg", ".ogg", exportText);

		if(ImGui::MenuItem("Mp3 file...", nullptr, nullptr, false))
			FileDialog::SaveFile("export_project", "Export To Mp3", ".mp3", exportText);

		if(ImGui::MenuItem("FLAC file...", nullptr, nullptr, false))
			FileDialog::SaveFile("export_project", "Export To FLAC", ".flac", exportText);

		if(ImGui::MenuItem("M4A file...", nullptr, nullptr, false))
			FileDialog::SaveFile("export_project", "Export To M4A", ".m4a", exportText);

		if(ImGui::MenuItem("MIDI file...", nullptr, nullptr, false))
			FileDialog::SaveFile("export_project", "Export To MIDI", ".midi", exportText);

		ImGui::EndMenu();
	}

	ImGui::Separator();

	if (ImGui::MenuItem("Exit"))
		ProjectManager::CloseProject();

	ImGui::EndMenu();
}

void MainMenuBar::EditMenu()
{
	if (!ImGui::BeginMenu("Edit"))
		return;

	ImGui::MenuItem("Undo", nullptr, nullptr, false);
	ImGui::Separator();
	ImGui::MenuItem("Cut", nullptr, nullptr, false);
	ImGui::MenuItem("Copy", nullptr, nullptr, false);
	ImGui::MenuItem("Paste", nullptr, nullptr, false);

	ImGui::EndMenu();
}

void MainMenuBar::OptionsMenu()
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
	if (!ImGui::BeginMenu("View"))
		return;

	if (ImGui::MenuItem("Show All"))
	{
		for (auto& [id, panelPtr] : Naui::GetAllPanels())
			panelPtr->SetOpen(true);
	}

	if (ImGui::MenuItem("Hide All"))
	{
		for (auto& [id, panelPtr] : Naui::GetAllPanels())
			panelPtr->SetOpen(false);
	}

	ImGui::Separator();

	for (auto& [id, panelPtr] : Naui::GetAllPanels())
	{
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
	if (!ImGui::BeginMenu("Layouts"))
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

	if (ImGui::MenuItem("Save Layout As..."))
	{
		newLayoutName[0] = '\0';
		popupFocusRequest = true;
		requestSaveAsPopup = true;
	}

	auto userLayouts = Layout::GetUserLayouts();
	if (ImGui::BeginMenu("Delete Layout", !userLayouts.empty()))
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
	if (!ImGui::BeginMenu("Help"))
		return;

	ImGui::MenuItem("Tutorials");
	ImGui::MenuItem("About");

	ImGui::EndMenu();
}

void MainMenuBar::RenderPopups()
{
	if (requestSaveAsPopup)
	{
		ImGui::OpenPopup("Save Layout As");
		requestSaveAsPopup = false;
	}

	if (requestOverridePopup)
	{
		ImGui::OpenPopup("Override Layout?");
		requestOverridePopup = false;
	}

	if (ImGui::BeginPopup("Save Layout As"))
	{
		if (popupFocusRequest)
		{
			ImGui::SetKeyboardFocusHere();
			popupFocusRequest = false;
		}

		ImGui::InputText("Layout Name", newLayoutName, IM_ARRAYSIZE(newLayoutName));

		if (ImGui::Button("Save") || ImGui::IsKeyPressed(ImGuiKey_Enter))
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

		if (ImGui::Button("Cancel"))
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("Override Layout?"))
	{
		ImGui::Text("A layout named '%s' already exists.", pendingOverrideName.c_str());
		ImGui::Text("Do you want to overwrite it?");

		if (ImGui::Button("Overwrite"))
		{
			Layout::Save(pendingOverrideName, true);
			currentLayout = pendingOverrideName;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel"))
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}
}
