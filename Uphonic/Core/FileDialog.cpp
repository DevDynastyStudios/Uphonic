#include "FileDialog.h"
#include "Naui/FileSystem/File.h"
#include <imgui.h>
#include <string.h>
#include <algorithm>
#include <chrono>

static bool MatchesFilter(const std::filesystem::path& p, const std::string& filters)
{
	if (filters.empty())
		return true;

	auto ext = p.extension().string();
	return filters.find(ext) != std::string::npos;
}

void FileDialog::OpenFile(const char* key, const char* title, const char* filters)
{
	state.open = true;
	state.folderMode = false;
	state.key = key;
	state.title = title;
	state.filters = filters ? filters : "";
	state.currentDir = std::filesystem::current_path();
	state.selected.clear();
	state.searchText.clear();
	state.hasPendingDir = false;
}

void FileDialog::OpenFolder(const char* key, const char* filters)
{
	state.open = true;
	state.folderMode = true;
	state.key = key;
	state.title = "Choose Folder";
	state.filters = filters ? filters : "";
	state.currentDir = std::filesystem::current_path();
	state.selected.clear();
	state.searchText.clear();
	state.hasPendingDir = false;
}

void FileDialog::Display(const char* key, const std::function<void(const std::filesystem::path&)>& callback)
{
	if (!state.open || state.key != key)
		return;

	if (state.hasPendingDir)
	{
		if (std::filesystem::exists(state.pendingDir) && std::filesystem::is_directory(state.pendingDir))
		{
			state.currentDir = state.pendingDir;
			state.selected.clear();
		}
		state.hasPendingDir = false;
	}

	state.callback = callback;

	const int windowWidth = 800;
	const int windowHeight = windowWidth / 14 * 9;

	ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSizeConstraints(ImVec2(windowWidth / 3 * 2, windowHeight / 3 * 2), ImVec2(FLT_MAX, FLT_MAX));
	ImGui::SetNextWindowFocus();

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse;

	if (ImGui::Begin(state.title.c_str(), nullptr, flags))
	{
		float fullWidth = ImGui::GetContentRegionAvail().x;
		float breadcrumbWidth = fullWidth * 0.65f;
		float rowH = ImGui::GetFrameHeight();

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
		ImGui::BeginChild("BreadcrumbBox", ImVec2(breadcrumbWidth, rowH), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		float textH = ImGui::GetTextLineHeight();
		ImGui::SetCursorPosY((rowH - textH) * 0.5f);
		DrawBreadcrumb();
		ImGui::EndChild();
		ImGui::PopStyleColor();

		ImGui::SameLine();
		if (ImGui::Button("R", ImVec2(rowH, rowH)))
		{
			state.pendingDir = state.currentDir;
			state.hasPendingDir = true;
		}

		ImGui::SameLine();
		char searchBuf[256];
		strncpy(searchBuf, state.searchText.c_str(), sizeof(searchBuf));

		ImGui::PushItemWidth(-1);
		if (ImGui::InputTextWithHint("##SearchBar", "Search", searchBuf, sizeof(searchBuf)))
			state.searchText = searchBuf;
		ImGui::PopItemWidth();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		DrawFileTable();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("File Name:");
		ImGui::SameLine();

		std::string displayName = Naui::Directory::ToUTF8(state.folderMode ? state.selected.u8string() : state.selected.filename().u8string());
		char selectedBuf[512];
		if (displayName.size() < sizeof(selectedBuf))
		{
		    std::memcpy(selectedBuf, displayName.data(), displayName.size());
		    selectedBuf[displayName.size()] = '\0';
		}
		else
		    selectedBuf[0] = '\0';

		ImGui::PushItemWidth(-1);
		ImGui::InputText("##SelectedFile", selectedBuf, sizeof(selectedBuf), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);
		ImGui::PopItemWidth();
		
		ImGui::Dummy(ImVec2(0, 0));

		float buttonWidth = 80.0f;
		float spacing = ImGui::GetStyle().ItemSpacing.x;
		float totalWidth = buttonWidth * 2.0f + spacing;
		float avail = ImGui::GetContentRegionAvail().x;

		ImGui::SetCursorPosX(avail - totalWidth);

		bool canConfirm = state.folderMode ? std::filesystem::exists(state.currentDir) : (!state.selected.empty() && std::filesystem::exists(state.selected));
		if (!canConfirm) ImGui::BeginDisabled();

		if (ImGui::Button("Open", ImVec2(buttonWidth, 0)))
		{
			state.open = false;
			state.callback(state.folderMode ? state.currentDir : state.selected);
		}

		if (!canConfirm) ImGui::EndDisabled();

		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
			state.open = false;
	}

	ImGui::End();
}

void FileDialog::DrawBreadcrumb()
{
	float rowH = ImGui::GetFrameHeight();
	if (!state.editingPath)
	{
		std::filesystem::path accum;
		bool first = true;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		int idCounter = 0;
		for (auto& part : state.currentDir)
		{
			std::string partStr = part.string();
			if (partStr.empty() || partStr == "\\" || partStr == "/")
				continue;

			if (first)
			{
				accum = partStr + "\\";
				first = false;
			}
			else
			{
				accum /= partStr;
				ImGui::SameLine(0, 0);
				ImGui::TextUnformatted("/");
				ImGui::SameLine(0, 0);
			}

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);

			std::string buttonId = partStr + "###" + std::to_string(idCounter);
			if (ImGui::Button(buttonId.c_str()))
			{
				state.pendingDir = accum;
				state.hasPendingDir = true;
			}

			ImGui::PopStyleColor(3);
			idCounter++;
		}

		ImGui::PopStyleVar();

		float remaining = ImGui::GetContentRegionAvail().x;
		if (remaining > 0)
		{
			ImGui::SameLine();
			ImGui::InvisibleButton("##BreadcrumbEditTrigger", ImVec2(remaining, rowH));
			if (ImGui::IsItemClicked())
			{
				state.editingPath = true;
				state.editingJustActivated = true;
			}
		}
	}

	else
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 0));

		char pathBuf[512];
		strncpy(pathBuf, state.currentDir.string().c_str(), sizeof(pathBuf));

		if (state.editingJustActivated)
		{
			ImGui::SetKeyboardFocusHere();
			state.editingJustActivated = false;
		}

		bool submitted = ImGui::InputText("##PathEdit", pathBuf, sizeof(pathBuf), ImGuiInputTextFlags_EnterReturnsTrue);

		ImGui::PopStyleVar();

		if (submitted)
		{
			state.pendingDir = std::filesystem::path(pathBuf);
			state.hasPendingDir = true;
			state.editingPath = false;
		}
		else
		{
			if (!ImGui::IsItemActive() && !ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
			{
				state.editingPath = false;
			}
		}
	}
}

void FileDialog::DrawFileTable()
{
	std::vector<std::filesystem::directory_entry> entries;
	entries.reserve(256);
	std::error_code ec;
	for (auto& e : std::filesystem::directory_iterator(state.currentDir, ec))
		entries.push_back(e);

	if (ec)
	{
		state.currentDir = std::filesystem::current_path();
		entries.clear();
		for (auto& e : std::filesystem::directory_iterator(state.currentDir))
			entries.push_back(e);
	}

	entries.erase(
		std::remove_if(entries.begin(), entries.end(),
			[&](const auto& entry)
			{
				const bool isDir = entry.is_directory();
				const std::string name = Naui::Directory::ToUTF8(entry.path().filename());

				if (!state.searchText.empty() &&
					name.find(state.searchText) == std::string::npos)
					return true;

				if (!isDir && !MatchesFilter(entry.path(), state.filters))
					return true;

				return false;
			}),
		entries.end()
	);

	float bottomHeight =
		ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y * 3 +
		ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y +
		ImGui::GetStyle().WindowPadding.y;

	float maxChildHeight =
		ImGui::GetWindowContentRegionMax().y -
		ImGui::GetCursorPosY() -
		bottomHeight;

	const float maxBounds = 50.0f;
	if (maxChildHeight < maxBounds)
		maxChildHeight = maxBounds;

	ImGui::BeginChild("FileList", ImVec2(0, maxChildHeight), true);

	if (ImGui::BeginTable("FileTable", 4,
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_BordersInnerV))
	{
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableSetupColumn("Created", ImGuiTableColumnFlags_WidthFixed, 140.0f);
		ImGui::TableSetupColumn("Modified", ImGuiTableColumnFlags_WidthFixed, 140.0f);
		ImGui::TableHeadersRow();

		for (auto& entry : entries)
		{
			const bool isDir = entry.is_directory();
			const auto path = entry.path();
			const std::string name = Naui::Directory::ToUTF8(path.filename());

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			std::string label = isDir ? "[DIR] " + name : name;
			bool selected = (state.selected == path);

			if (ImGui::Selectable(label.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
			{
				if (isDir)
				{
					state.pendingDir = path;
					state.hasPendingDir = true;
				}
				else
					state.selected = path;
			}

			ImGui::TableSetColumnIndex(1);	// Size
			if (isDir)
				ImGui::TextUnformatted("-");
			else
			{
				uint64_t size = std::filesystem::file_size(path);
				std::string pretty = FormatBytes(size);
				ImGui::TextUnformatted(pretty.c_str());
			}

			ImGui::TableSetColumnIndex(2);	// Created
			auto created = std::filesystem::last_write_time(path);
			PrintTime(created);

			ImGui::TableSetColumnIndex(3);	// Modified
			auto modified = std::filesystem::last_write_time(path);
			PrintTime(modified);
		}

		ImGui::EndTable();
	}

	ImGui::EndChild();
}

void FileDialog::PrintTime(const std::filesystem::file_time_type& ft)
{
	using namespace std::chrono;

	auto sctp = clock_cast<system_clock>(ft);
	std::time_t tt = system_clock::to_time_t(sctp);
	std::tm* tm = std::localtime(&tt);

	ImGui::Text("%04d-%02d-%02d %02d:%02d",
		tm->tm_year + 1900,
		tm->tm_mon + 1,
		tm->tm_mday,
		tm->tm_hour,
		tm->tm_min);
}


std::string FileDialog::FormatBytes(uint64_t bytes)
{
	static const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
	static const size_t unitCount = sizeof(units) / sizeof(units[0]);
	double size = static_cast<double>(bytes);
	size_t unitIndex = 0;

	while (size >= 1024.0 && unitIndex < unitCount - 1)
	{
		size /= 1024.0;
		unitIndex++;
	}

	char buf[64];
	if (unitIndex == 0)
		snprintf(buf, sizeof(buf), "%llu %s", (unsigned long long)bytes, units[unitIndex]);
	else
		snprintf(buf, sizeof(buf), "%.1f %s", size, units[unitIndex]);

	return std::string(buf);
}