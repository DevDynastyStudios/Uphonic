#include "MainMenuBar.h"
#include "Core/ProjectState.h"
#include "Core/ProjectManager.h"
#include "Core/FileDialog.h"
#include "Audio/AudioEngine.h"

double MainMenuBar::GetTimelineDuration()	// (Chimpchi): Move this into a more appropriate file
{
    ProjectState& state = ProjectState::GetInstance();
    double endBeat = 0.0;
    for (const auto& track : state.tracks)
    {
        for (const auto& block : track.blocks)
        {
            double blockEnd = 0.0;
            if (track.type == TrackType::Midi)
                blockEnd = block.midiBlock.startBeat + block.midiBlock.lengthBeats;
            else
                blockEnd = block.sampleBlock.startBeat + block.sampleBlock.lengthBeats;

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
        state.samples.push_back(AudioEngine::LoadSample(path.c_str()));
	});

	if(!ImGui::BeginMenu("File"))
		return;

	if (ImGui::MenuItem("New Project"))
		ProjectManager::NewProject();

    if (ImGui::MenuItem("Open"))
	{
		FileDialog::OpenFile("menubar_open_project", "Open Project", ".uph");
	}

	ImGui::Separator();

    if (ImGui::MenuItem("Save")) {}

    if (ImGui::MenuItem("Save As")) {}

    ImGui::Separator();

    if (ImGui::BeginMenu("Import"))
    {
        if (ImGui::MenuItem("Wave file"))
        {
		    FileDialog::OpenFile("menubar_import_wav", "Import Wave file", ".wav");
        }
        if (ImGui::MenuItem("MIDI")) {}
        ImGui::EndMenu();
    }

	if (ImGui::BeginMenu("Export"))
    {
        if (ImGui::MenuItem("Wave file..."))
			AudioEngine::ExportToWav("test.wav", 0, GetTimelineDuration());

        if (ImGui::MenuItem("Ogg file...", nullptr, nullptr, false)) {}
        if (ImGui::MenuItem("Mp3 file...", nullptr, nullptr, false)) {}
        if (ImGui::MenuItem("FLAC file...", nullptr, nullptr, false)) {}
        if (ImGui::MenuItem("M4A file...", nullptr, nullptr, false)) {}
        if (ImGui::MenuItem("MIDI file...", nullptr, nullptr, false)) {}
        ImGui::EndMenu();
    }

	ImGui::Separator();

    if (ImGui::MenuItem("Exit"))
        exit(0);

    ImGui::EndMenu();
}

void MainMenuBar::EditMenu()
{
	if(!ImGui::BeginMenu("Edit"))
		return;

    if (ImGui::MenuItem("Undo")) {}
    ImGui::Separator();
    if (ImGui::MenuItem("Cut")) {}
    if (ImGui::MenuItem("Copy")) {}
    if (ImGui::MenuItem("Paste")) {}

	ImGui::EndMenu();
}

void MainMenuBar::OptionsMenu()
{
	if(!ImGui::BeginMenu("Options"))
		return;

	if (ImGui::MenuItem("MIDI Settings")) {}
    if (ImGui::MenuItem("Audio Settings")) {}
    if (ImGui::MenuItem("General Settings")) {}

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

	// (Chimpchi): Add category system

    for (auto& [id, panelPtr] : Naui::GetAllPanels())
    {
        Naui::Panel& panel = *panelPtr;
        ImGui::PushID(id);
        if (ImGui::MenuItem(panel.GetTitle().c_str(), nullptr, panel.IsOpen()))
            panel.SetOpen(!panel.IsOpen());

        ImGui::PopID();
    }

    ImGui::EndMenu();
}

void MainMenuBar::LayoutMenu()
{
	if(!ImGui::BeginMenu("Layouts"))
		return;

	
	ImGui::EndMenu();
}

void MainMenuBar::HelpMenu()
{
	if(!ImGui::BeginMenu("Help"))
		return;

	if (ImGui::MenuItem("Tutorials")) {}
    if (ImGui::MenuItem("About")) {}

	ImGui::EndMenu();
}
