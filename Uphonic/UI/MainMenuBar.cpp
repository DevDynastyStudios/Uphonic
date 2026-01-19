#include "MainMenuBar.h"
#include "Core/ProjectState.h"
#include "Core/ProjectManager.h"
#include "Core/FileDialog.h"
#include "Core/Defer.h"
#include "Audio/AudioEngine.h"
#include "Layout.h"

static char newLayoutName[64] = {};
static bool popupFocusRequest = false;

static bool requestSaveAsPopup = false;
static bool requestOverridePopup = false;

static std::string pendingOverrideName;
static std::string currentLayout = "Default";

double MainMenuBar::GetTimelineDuration()
{
	ProjectState& state = ProjectState::GetInstance();
	double endBeat = 0.0;

	for (const auto& track : state.tracks)
	{
		for (const auto& block : track.blocks)
		{
			double blockEnd = (track.type == TrackType::Midi)
				? block.midiBlock.startBeat + block.midiBlock.lengthBeats
				: block.sampleBlock.startBeat + block.sampleBlock.lengthBeats;

			if (blockEnd > endBeat)
				endBeat = blockEnd;
		}
	}

	return endBeat;
}

void MainMenuBar::FileMenu()
{
	FileDialog::Display("menubar_open_project", [](const std::filesystem::path& path)
	{
		ProjectManager::OpenProject(path);
	});

	FileDialog::Display("menubar_import_wav", [](const std::filesystem::path& path)
	{
		ProjectState& state = ProjectState::GetInstance();
		AudioEngine::AddSample(path.string().c_str());
	});

	if (!ImGui::BeginMenu("File"))
		return;

	if (ImGui::MenuItem("New Project"))
		ProjectManager::NewProject();

	if (ImGui::MenuItem("Open"))
		FileDialog::OpenFile("menubar_open_project", "Open Project", ".uph");

	ImGui::Separator();

	ImGui::MenuItem("Save");
	ImGui::MenuItem("Save As");

	ImGui::Separator();

	if (ImGui::BeginMenu("Import"))
	{
		ImGui::MenuItem("MIDI");
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Export"))
	{
		if (ImGui::MenuItem("Wave file..."))
			AudioEngine::ExportToWav("test.wav", 0, GetTimelineDuration());

		ImGui::MenuItem("Ogg file...", nullptr, nullptr, false);
		ImGui::MenuItem("Mp3 file...", nullptr, nullptr, false);
		ImGui::MenuItem("FLAC file...", nullptr, nullptr, false);
		ImGui::MenuItem("M4A file...", nullptr, nullptr, false);
		ImGui::MenuItem("MIDI file...", nullptr, nullptr, false);

		ImGui::EndMenu();
	}

	ImGui::Separator();

	if (ImGui::MenuItem("Exit"))
		exit(0);

	ImGui::EndMenu();
}

void MainMenuBar::EditMenu()
{
	if (!ImGui::BeginMenu("Edit"))
		return;

	ImGui::MenuItem("Undo");
	ImGui::Separator();
	ImGui::MenuItem("Cut");
	ImGui::MenuItem("Copy");
	ImGui::MenuItem("Paste");

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

	for (const auto& name : Layout::GetSystemLayouts())
	{
		std::string label = name + "###System_" + name;
		bool selected = (name == currentLayout);
	
		if (ImGui::MenuItem(label.c_str(), nullptr, selected))
		{
			currentLayout = name;
			Naui::Defer::Add(Layout::Load, name);
		}
	}
	
	ImGui::Separator();
	
	for (const auto& name : Layout::GetUserLayouts())
	{
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
		for (const auto& name : userLayouts)
		{
			if (ImGui::MenuItem(name.c_str()))
			{
				Layout::Delete(name);
				if (currentLayout == name)
					currentLayout = "Default";
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
			if (Layout::ExistsUser(name))
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
