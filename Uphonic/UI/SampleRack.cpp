#include "SampleRack.h"
#include "Naui/FileSystem/File.h"
#include "Naui/FileSystem/FileDialog.h"
#include "Naui/Localization/Localization.h"
#include "Core/ProjectState.h"
#include "Core/ProjectManager.h"
#include "Audio/AudioEngine.h"
#include "Models/DataModel/Samples.h"
#include "Actions/Samples/RenameSampleAction.h"
#include <algorithm>
#include <cstring>
#include <imgui_internal.h>
#include <iostream>

static float SAMPLE_PANEL_MIN_WIDTH		= 120.0f;
static float SAMPLE_PANEL_MIN_HEIGHT	= 120.0f;

static const float IMPORT_BUTTON_WIDTH	= 90.0f;
static const float IMPORT_BUTTON_HEIGHT	= 26.0f;

static float SAMPLE_ITEM_MIN_WIDTH	= 60.0f;
static float SAMPLE_ITEM_MAX_WIDTH	= 120.0f;
static float SAMPLE_ITEM_HEIGHT		= 22.0f;
static float SAMPLE_ITEM_PADDING	= 8.0f;

static float SAMPLE_COLOR_STRIP_WIDTH	= 16.0f;

static float SAMPLE_BUTTON_ROUNDING	= 5.0f;
static float SAMPLE_BUTTON_OUTLINE	= 1.0f;

static ImVec4 SAMPLE_BG_COLOR		= ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
static ImVec4 SAMPLE_BG_HOVER_COLOR	= ImVec4(0.24f, 0.24f, 0.24f, 1.0f);
static ImVec4 SAMPLE_OUTLINE_COLOR	= ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
static ImVec4 SAMPLE_OUTLINE_ACTIVE	= ImVec4(1.00f, 1.00f, 1.00f, 1.0f);

#pragma region Facade Functions
SampleRack::SampleRack() : Naui::Panel(Naui::TR("sample_rack.title"))
{
	m_renamingIndex = -1;
	memset(m_renameBuffer, 0, sizeof(m_renameBuffer));
	SetMinSize(SAMPLE_PANEL_MIN_WIDTH, SAMPLE_PANEL_MIN_HEIGHT);
}

size_t SampleRack::GetSampleIndex(AudioSample& sample)
{
	return &sample - ProjectState::GetInstance().samples.data();
}

AudioSample& SampleRack::GetSampleAtIndex(size_t index)
{
	ProjectState& state = ProjectState::GetInstance();
	if (index >= state.samples.size())
		throw std::out_of_range("Sample index out of range");

	return state.samples[index];
}

bool SampleRack::RenameSample(size_t index, const std::string& newName)
{
	ProjectState& state = ProjectState::GetInstance();
	if(index >= state.samples.size())
		return false;

	AudioSample& sample = state.samples[index];
	sample.name = newName;
	return true;
}
#pragma endregion

bool SampleRack::DrawSampleItem(AudioSample& sample, float width, float height, bool& colorClicked)
{
	ImGui::PushID(&sample);
	ImVec2 pos  = ImGui::GetCursorScreenPos();
	ImVec2 size(width, height);
	ImRect bb(pos, pos + size);

	ImGui::InvisibleButton("##hitbox", size);

	bool hovered = ImGui::IsItemHovered();
	bool held	= ImGui::IsItemActive();
	bool pressed = ImGui::IsItemClicked();

	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImU32 bg = ImGui::ColorConvertFloat4ToU32(hovered ? SAMPLE_BG_HOVER_COLOR : SAMPLE_BG_COLOR);
	dl->AddRectFilled(bb.Min, bb.Max, bg, SAMPLE_BUTTON_ROUNDING);

	ImU32 outline = ImGui::ColorConvertFloat4ToU32(held ? SAMPLE_OUTLINE_ACTIVE : SAMPLE_OUTLINE_COLOR);
	dl->AddRect(bb.Min, bb.Max, outline, SAMPLE_BUTTON_ROUNDING, 0, SAMPLE_BUTTON_OUTLINE);

	ImRect strip(bb.Min, ImVec2(bb.Min.x + SAMPLE_COLOR_STRIP_WIDTH, bb.Max.y));
	dl->AddRectFilled(strip.Min, strip.Max, ImGui::ColorConvertFloat4ToU32(sample.color), SAMPLE_BUTTON_ROUNDING, ImDrawFlags_RoundCornersLeft);

	if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		ImVec2 mouse = ImGui::GetIO().MousePos;
		if (mouse.x >= strip.Min.x && mouse.x <= strip.Max.x && mouse.y >= strip.Min.y && mouse.y <= strip.Max.y)
		{
			colorClicked = true;
		}
	}

	float textX = bb.Min.x + SAMPLE_COLOR_STRIP_WIDTH + 6.0f;
	float textY = bb.Min.y + (height - ImGui::GetFontSize()) * 0.5f;
	ImVec2 textPos(textX, textY);
	ImVec2 textMax(bb.Max.x - 4.0f, bb.Max.y);
	float ellipsisMaxX = bb.Max.x - 4.0f;

	ImGui::RenderTextEllipsis(dl, textPos, textMax, ellipsisMaxX, sample.name.c_str(), nullptr, nullptr);
	ImGui::PopID();
	return pressed;
}

void SampleRack::OnRender()
{
	FileDialog::Display("sample_audio_import", [this](const std::filesystem::path& path)
	{
    	std::cout << Naui::Directory::PathToUTF8(path) << "\n";
		ProjectManager::ImportSample(path);
	});

	ProjectState& state = ProjectState::GetInstance();
	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton("##import_hitbox", ImVec2(IMPORT_BUTTON_WIDTH, IMPORT_BUTTON_HEIGHT));

	bool hovered = ImGui::IsItemHovered();
	bool pressed = ImGui::IsItemClicked();

	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImRect bb(pos, pos + ImVec2(IMPORT_BUTTON_WIDTH, IMPORT_BUTTON_HEIGHT));

	ImGuiStyle& style = ImGui::GetStyle();
	ImU32 bg = ImGui::IsItemActive() ? ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_ButtonActive]) : ImGui::IsItemHovered() ? ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_ButtonHovered]) : ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Button]);
	dl->AddRectFilled(bb.Min, bb.Max, bg, 4.0f);

	const char* label = Naui::TR("sample_rack.import");
	ImVec2 ts = ImGui::CalcTextSize(label);
	float tx = bb.Min.x + (IMPORT_BUTTON_WIDTH - ts.x) * 0.5f;
	float ty = bb.Min.y + (IMPORT_BUTTON_HEIGHT - ts.y) * 0.5f;

	ImGui::SetCursorScreenPos(ImVec2(tx, ty));
	ImGui::TextUnformatted(label);

	if (pressed)
	{
		FileDialog::OpenFile("sample_audio_import", "Import Audio", "*.wav;*.mp3;*.flac;*.ogg");
	}

	ImGui::Separator();

	if (state.samples.empty())
	{
		ImVec2 avail = ImGui::GetContentRegionAvail();
		ImGui::BeginChild("empty", avail, true);

		const char* msg = Naui::TR("sample_rack.import_text");
		float wrap = avail.x * 0.75f;

		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrap);
		ImVec2 ts = ImGui::CalcTextSize(msg, nullptr, false, wrap);

		ImGui::SetCursorPosX((avail.x - ts.x) * 0.5f);
		ImGui::SetCursorPosY((avail.y - ts.y) * 0.5f);
		ImGui::TextUnformatted(msg);

		ImGui::PopTextWrapPos();
		ImGui::EndChild();
		return;
	}

	float panelWidth = ImGui::GetContentRegionAvail().x;
	panelWidth = std::max<float>(panelWidth, SAMPLE_PANEL_MIN_WIDTH);

	int itemsPerRow = (int)((panelWidth + SAMPLE_ITEM_PADDING) / (SAMPLE_ITEM_MAX_WIDTH + SAMPLE_ITEM_PADDING));
	itemsPerRow = std::max<int>(1, itemsPerRow);

	float maxCellWidth = (panelWidth - SAMPLE_ITEM_PADDING * (itemsPerRow - 1)) / itemsPerRow;
	maxCellWidth = std::max<float>(maxCellWidth, SAMPLE_ITEM_MIN_WIDTH);

	for (int i = 0; i < (int)state.samples.size(); i++)
	{
		AudioSample& sample = state.samples[i];
		ImGui::PushID(i);

		int col = i % itemsPerRow;
		if (col > 0)
			ImGui::SameLine(0.0f, SAMPLE_ITEM_PADDING);

		float textWidth = ImGui::CalcTextSize(sample.name.c_str()).x;
		float naturalWidth = SAMPLE_COLOR_STRIP_WIDTH + 6.0f + textWidth + 4.0f;
		float finalWidth = ImClamp(naturalWidth, SAMPLE_ITEM_MIN_WIDTH, maxCellWidth);

		if (m_renamingIndex == i)
		{
			float renameTextWidth = ImGui::CalcTextSize(m_renameBuffer).x;
			float naturalWidth = SAMPLE_COLOR_STRIP_WIDTH + 6.0f + renameTextWidth + 2.0f;
			float renameWidth = ImClamp(naturalWidth, SAMPLE_ITEM_MIN_WIDTH, maxCellWidth);
			bool exceeded = naturalWidth > maxCellWidth;
		
			ImVec2 pos  = ImGui::GetCursorScreenPos();
			ImVec2 size(renameWidth, SAMPLE_ITEM_HEIGHT);
			ImRect bb(pos, pos + size);
			ImGuiID id = ImGui::GetID("##rename_item");
			ImGui::ItemAdd(bb, id);
		
			ImDrawList* dl = ImGui::GetWindowDrawList();
			dl->AddRectFilled(bb.Min, bb.Max, ImGui::ColorConvertFloat4ToU32(SAMPLE_BG_COLOR), SAMPLE_BUTTON_ROUNDING);
			dl->AddRect(bb.Min, bb.Max, ImGui::ColorConvertFloat4ToU32(SAMPLE_OUTLINE_COLOR), SAMPLE_BUTTON_ROUNDING, 0, SAMPLE_BUTTON_OUTLINE);
			ImRect strip(bb.Min, ImVec2(bb.Min.x + SAMPLE_COLOR_STRIP_WIDTH, bb.Max.y));
			dl->AddRectFilled(strip.Min, strip.Max, ImGui::ColorConvertFloat4ToU32(sample.color), SAMPLE_BUTTON_ROUNDING, ImDrawFlags_RoundCornersLeft);
			
			float textX = pos.x + SAMPLE_COLOR_STRIP_WIDTH + 6.0f;
			float textWidth = renameWidth - SAMPLE_COLOR_STRIP_WIDTH - 10.0f;
			
			ImGui::SetCursorScreenPos(ImVec2(textX, pos.y));
			
			float padY = (SAMPLE_ITEM_HEIGHT - ImGui::GetFontSize()) * 0.5f;
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, padY));
			ImGui::SetNextItemWidth(textWidth);
			
			if (m_justStartedRenaming)
			{
				ImGui::SetKeyboardFocusHere();
				m_justStartedRenaming = false;
			}

			ImGuiInputTextFlags flags = exceeded ? ImGuiInputTextFlags_EnterReturnsTrue : (ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_NoHorizontalScroll);
			bool enter = ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer), flags);
			ImGui::PopStyleVar();
			bool clickedOutside = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsItemHovered() && !ImGui::IsMouseHoveringRect(bb.Min, bb.Max);
		
			if (enter)
			{
				if(sample.name != m_renameBuffer)
					state.actionManager.Execute<RenameSampleAction>(SampleRack::GetSampleAtIndex(i), m_renameBuffer);

				m_renamingIndex = -1;
			}
			else if (clickedOutside)
			{
				m_renamingIndex = -1;
			}
		
			ImGui::Dummy(size);
			ImGui::PopID();
			continue;
		}

		bool colorClicked = false;
		bool pressed = DrawSampleItem(sample, finalWidth, SAMPLE_ITEM_HEIGHT, colorClicked);
		
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			m_renamingIndex = i;
			strncpy(m_renameBuffer, sample.name.c_str(), sizeof(m_renameBuffer) - 1);
			m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
			m_justStartedRenaming = true;
		}

		if (ImGui::IsItemHovered() && ImGui::IsKeyPressed(ImGuiKey_F2))
		{
			m_renamingIndex = i;
			strncpy(m_renameBuffer, sample.name.c_str(), sizeof(m_renameBuffer) - 1);
			m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
			m_justStartedRenaming = true;
		}

		if (colorClicked)
			ImGui::OpenPopup("ColorPicker");

		if (ImGui::BeginPopup("ColorPicker"))
		{
			ImGui::ColorPicker4("##picker", (float*)&sample.color);
			ImGui::EndPopup();
		}

		if (ImGui::BeginDragDropSource())
		{
			uint16_t idx = (uint16_t)i;
			ImGui::SetDragDropPayload("SAMPLE_INDEX", &idx, sizeof(uint16_t));
			ImGui::Text("%s", sample.name.c_str());
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Rename"))
			{
				m_renamingIndex = i;
				strncpy(m_renameBuffer, sample.name.c_str(), sizeof(m_renameBuffer) - 1);
				m_renameBuffer[sizeof(m_renameBuffer) - 1] = '\0';
				m_justStartedRenaming = true;
			}

			if (ImGui::MenuItem("Delete"))
			{
				DeleteSample(i);
				ImGui::EndPopup();
				ImGui::PopID();
				break;
			}

			ImGui::Separator();
			ImGui::TextDisabled("Sample Rate: %d Hz", sample.originalSampleRate);
			ImGui::TextDisabled("Channels: %s", sample.channelType == SampleChannelType::Mono ? "Mono" : "Stereo");
			ImGui::TextDisabled("Frames: %lu", sample.frameCount);
			ImGui::EndPopup();
		}

		ImGui::PopID();
	}
}

void SampleRack::DeleteSample(uint16_t index)
{
	ProjectState& state = ProjectState::GetInstance();
	if (index >= state.samples.size()) 
		return;

	ProjectManager::DeleteSample((size_t)index);

	for (auto& track : state.tracks)
	{
		if (track.type == TrackType::Audio)
		{
			track.blocks.erase(std::remove_if(track.blocks.begin(), track.blocks.end(), [index](const TimelineBlock& block) {return block.sampleBlock.sampleIndex == index;}), track.blocks.end());
			for (auto& block : track.blocks)
			{
				if (block.sampleBlock.sampleIndex > index)
					block.sampleBlock.sampleIndex--;
			}
		}
	}
}
