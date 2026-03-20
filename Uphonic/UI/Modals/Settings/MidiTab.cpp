#include "MidiTab.h"
#include "Models/EditorModel/ApplicationSettings.h"
#include <imgui.h>

void MidiTab::Draw(ApplicationSettings& draft)
{
	MIDISettings& m = draft.midiSettings;
	ImGui::SeparatorText("Recording");
	{
		ImGui::Checkbox("MIDI Thru", &m.midiThruEnabled);
		ImGui::Checkbox("Record Overdub", &m.recordOverdub);
		ImGui::Checkbox("Record Quantize", &m.recordQuantize);

		if (m.recordQuantize)
		{
			int grid = (int)m.recordQuantizeGrid;
			if (ImGui::DragInt("Quantize Grid (1/N)", &grid, 1, 1, 64))
				m.recordQuantizeGrid = (uint32_t)grid;
		}
	}

	ImGui::SeparatorText("Metronome");
	{
		ImGui::Checkbox("Metronome", &m.metronomeEnabled);
		ImGui::Checkbox("Count In", &m.countInEnabled);

		if (m.countInEnabled)
		{
			int bars = (int)m.countInBars;
			if (ImGui::DragInt("Count-in Bars", &bars, 1, 1, 8))
				m.countInBars = (uint32_t)bars;
		}
	}

	ImGui::SeparatorText("Defaults");
	{
		int velocity = (int)m.defaultVelocity;
		if (ImGui::DragInt("Default Velocity", &velocity, 1, 1, 127))
			m.defaultVelocity = (uint8_t)velocity;

		ImGui::DragFloat("Default Note Length (beats)", &m.defaultNoteLength, 0.0625f, 0.125f, 16.0f, "%.3f");
		ImGui::DragFloat("Default Beat Width", &m.defaultBeatWidth, 1.0f, 10.0f, 200.0f, "%.1f");
		ImGui::DragFloat("Default Note Height (row)", &m.defaultBeatHeight, 1.0f, 8.0f, 64.0f, "%.1f");
		ImGui::DragFloat("Note Block Height", &m.noteHeight, 1.0f, 4.0f, 32.0f, "%.1f");
	}

	ImGui::SeparatorText("Grid");
	{
		int grid = (int)m.gridDivision;
		if (ImGui::DragInt("Grid Division (1/N)", &grid, 1, 1, 64))
			m.gridDivision = (uint32_t)grid;

		ImGui::DragFloat("Swing", &m.gridSwing, 0.01f, 0.0f, 1.0f, "%.2f");
	}

	ImGui::SeparatorText("Display");
	{
		ImGui::Checkbox("Show Grid Lines", &m.showGridLines);
		ImGui::Checkbox("Show Note Labels", &m.showNoteLabels);
		ImGui::Checkbox("Colour Notes by Channel", &m.colorNotesByChannel);
	}

	ImGui::SeparatorText("Channel");
	{
		int channel = (int)m.defaultChannel;
		if (ImGui::DragInt("Default Channel", &channel, 1, 1, 16))
			m.defaultChannel = (uint8_t)channel;

		ImGui::Checkbox("Filter by Channel", &m.filterByChannel);
	}
}