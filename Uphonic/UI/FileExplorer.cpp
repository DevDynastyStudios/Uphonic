#include "FileExplorer.h"
#include "../Core/ProjectState.h"
#include "../Audio/AudioEngine.h"
#include <algorithm>
#include <cstring>
#include <cctype>

#ifndef ICON_FA_FOLDER
#define ICON_FA_FOLDER "[DIR]"
#define ICON_FA_FILE "[FILE]"
#define ICON_FA_FILE_AUDIO "[AUD]"
#define ICON_FA_MUSIC "[MID]"
#endif

FileExplorer::FileExplorer() : Naui::Panel("File Explorer")
{
    m_currentPath = fs::current_path();
    strcpy(m_pathInput, m_currentPath.string().c_str());
    m_historyIndex = 0;
    m_selectedIndex = -1;
    memset(m_searchFilter, 0, sizeof(m_searchFilter));
    RefreshDirectory();
}

void FileExplorer::OnRender()
{
    RenderToolbar();
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::BeginChild("##FileList", ImVec2(0, -160), ImGuiChildFlags_ResizeY);
    RenderFileList();
    ImGui::EndChild();
    
    RenderInfoPanel();
}

void FileExplorer::RenderToolbar()
{
    ImGui::BeginDisabled(m_historyIndex == 0);
    if (ImGui::ArrowButton("##Back", ImGuiDir_Left))
    {
        NavigateBack();
    }
    ImGui::EndDisabled();
    
    ImGui::SameLine();
    ImGui::BeginDisabled(m_historyIndex >= m_history.size() - 1);
    if (ImGui::ArrowButton("##Forward", ImGuiDir_Right))
    {
        NavigateForward();
    }
    ImGui::EndDisabled();
    
    ImGui::SameLine();
    ImGui::BeginDisabled(!m_currentPath.has_parent_path());
    if (ImGui::ArrowButton("##Up", ImGuiDir_Up))
    {
        NavigateUp();
    }
    ImGui::EndDisabled();
    
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    
    ImGui::SetNextItemWidth(-80.0f);
    if (ImGui::InputTextWithHint("##Path", "Path...", m_pathInput, sizeof(m_pathInput), 
                        ImGuiInputTextFlags_EnterReturnsTrue))
    {
        NavigateToPath(m_pathInput);
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Refresh", ImVec2(70, 0)))
    {
        RefreshDirectory();
    }
    
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputTextWithHint("##Search", "Search files...", m_searchFilter, sizeof(m_searchFilter)))
    {
        RefreshDirectory();
    }
}

void FileExplorer::RenderFileList()
{
    if (ImGui::BeginTable("##Files", 3, 
        ImGuiTableFlags_Resizable | 
        ImGuiTableFlags_RowBg | 
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_BordersInnerV))
    {
        
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.6f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
        
        ImGuiListClipper clipper;
        clipper.Begin(m_files.size());
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
            {
                const auto& file = m_files[i];
                
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                
                const char* icon = file.isDirectory ? ICON_FA_FOLDER : ICON_FA_FILE;
                if (!file.isDirectory && IsAudioFile(file.extension))
                {
                    icon = ICON_FA_FILE_AUDIO;
                }
                if (!file.isDirectory && (file.extension == ".mid" || file.extension == ".midi"))
                {
                    icon = ICON_FA_MUSIC;
                }
                
                ImGui::PushID(i);
                bool selected = i == m_selectedIndex;
                
                if (ImGui::Selectable(("##row" + std::to_string(i)).c_str(), 
                                     selected, 
                                     ImGuiSelectableFlags_SpanAllColumns | 
                                     ImGuiSelectableFlags_AllowDoubleClick))
                {
                    m_selectedIndex = i;
                    
                    if (ImGui::IsMouseDoubleClicked(0))
                    {
                        if (file.isDirectory)
                        {
                            NavigateToPath(file.fullPath.string());
                        }
                        else
                        {
                            OnFileSelected(file.fullPath);
                        }
                    }
                }
                
                ImGui::SameLine();
                ImGui::Text("%s  %s", icon, file.name.c_str());
                
                ImGui::TableNextColumn();
                if (file.isDirectory)
                {
                    ImGui::TextDisabled("Folder");
                }
                else
                {
                    ImGui::Text("%s", file.extension.empty() ? "-" : file.extension.c_str());
                }
                
                ImGui::TableNextColumn();
                if (!file.isDirectory)
                {
                    ImGui::Text("%s", FormatFileSize(file.size).c_str());
                }
                else
                {
                    ImGui::TextDisabled("-");
                }
                
                ImGui::PopID();
            }
        }
        
        ImGui::EndTable();
    }
}

void FileExplorer::RenderInfoPanel()
{
    ImGui::Separator();
    ImGui::BeginChild("##InfoPanel", ImVec2(0, 0), false);
    
    if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_files.size())
    {
        const auto& file = m_files[m_selectedIndex];
        
        ImGui::Spacing();
        ImGui::Indent(10.0f);
        
        ImGui::Text("Name: %s", file.name.c_str());
        
        ImGui::TextWrapped("Path: %s", file.fullPath.string().c_str());
        
        if (!file.isDirectory)
        {
            ImGui::Text("Size: %s", FormatFileSize(file.size).c_str());
            
            if (!file.extension.empty())
            {
                ImGui::Text("Type: %s", file.extension.c_str());
                
                if (IsAudioFile(file.extension))
                {
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Audio File");
                }
                else if (file.extension == ".mid" || file.extension == ".midi")
                {
                    ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.4f, 1.0f), "MIDI File");
                }
            }
        }
        else
        {
            ImGui::TextDisabled("Folder");
        }
        
        ImGui::Unindent(10.0f);
    }
    else
    {
        ImGui::Spacing();
        ImGui::Indent(10.0f);
        ImGui::TextDisabled("No file selected");
        ImGui::Unindent(10.0f);
    }
    
    ImGui::EndChild();
}

void FileExplorer::RefreshDirectory()
{
    m_files.clear();
    m_selectedIndex = -1;
    
    try
    {
        for (const auto& entry : fs::directory_iterator(m_currentPath))
        {
            FileItem item;
            item.fullPath = entry.path();
            item.name = entry.path().filename().string();
            item.isDirectory = entry.is_directory();
            item.extension = item.isDirectory ? "" : entry.path().extension().string();
            
            try
            {
                item.size = item.isDirectory ? 0 : fs::file_size(entry.path());
            }
            catch (...)
            {
                item.size = 0;
            }
            
            if (!PassesFilter(item)) continue;
            
            m_files.push_back(item);
        }
        
        std::sort(m_files.begin(), m_files.end(), [](const FileItem& a, const FileItem& b) {
            if (a.isDirectory != b.isDirectory) return a.isDirectory;
            return a.name < b.name;
        });
        
    }
    catch (const fs::filesystem_error& e)
    {
    }
    
    strcpy(m_pathInput, m_currentPath.string().c_str());
}

bool FileExplorer::PassesFilter(const FileItem& item)
{
    if (strlen(m_searchFilter) > 0)
    {
        std::string nameLower = item.name;
        std::string filterLower = m_searchFilter;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
        std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);
        if (nameLower.find(filterLower) == std::string::npos)
        {
            return false;
        }
    }
    
    return true;
}

bool FileExplorer::IsAudioFile(const std::string& ext)
{
    static const std::vector<std::string> audioExts = {
        ".wav", ".mp3", ".flac", ".ogg", ".aif", ".aiff", 
        ".m4a", ".wma", ".aac", ".opus"
    };
    return std::find(audioExts.begin(), audioExts.end(), ext) != audioExts.end();
}

std::string FileExplorer::FormatFileSize(uintmax_t size)
{
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit = 0;
    double s = (double)size;
    
    while (s >= 1024 && unit < 3)
    {
        s /= 1024;
        unit++;
    }
    
    char buf[64];
    snprintf(buf, sizeof(buf), "%.1f %s", s, units[unit]);
    return buf;
}

void FileExplorer::NavigateToPath(const std::string& path)
{
    try
    {
        fs::path newPath(path);
        if (fs::exists(newPath) && fs::is_directory(newPath))
        {
            if (m_historyIndex < m_history.size())
            {
                m_history.erase(m_history.begin() + m_historyIndex + 1, m_history.end());
            }
            m_history.push_back(m_currentPath);
            m_historyIndex = m_history.size();
            
            m_currentPath = newPath;
            RefreshDirectory();
        }
    }
    catch (const fs::filesystem_error& e)
    {
    }
}

void FileExplorer::NavigateUp()
{
    if (m_currentPath.has_parent_path())
    {
        NavigateToPath(m_currentPath.parent_path().string());
    }
}

void FileExplorer::NavigateBack()
{
    if (m_historyIndex > 0)
    {
        m_historyIndex--;
        m_currentPath = m_history[m_historyIndex];
        RefreshDirectory();
    }
}

void FileExplorer::NavigateForward()
{
    if (m_historyIndex < m_history.size() - 1)
    {
        m_historyIndex++;
        m_currentPath = m_history[m_historyIndex];
        RefreshDirectory();
    }
}

void FileExplorer::OnFileSelected(const fs::path& filePath)
{
    ProjectState& state = ProjectState::GetInstance();
    std::string ext = filePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (IsAudioFile(ext))
    {
        state.samples.push_back(AudioEngine::LoadSample(filePath.string().c_str()));
    }
}

