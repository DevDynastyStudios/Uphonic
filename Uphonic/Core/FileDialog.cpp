#include "FileDialog.h"
#include "ImGuiFileDialog.h"

void FileDialog::OpenFile(const char* title, const char* filters)
{
    IGFD::FileDialogConfig config;
	config.path = ".";
    ImGuiFileDialog::Instance()->OpenDialog(title, "Choose File", filters, config);
}

void FileDialog::OpenFolder(const char* title, const char* filters)
{
    IGFD::FileDialogConfig config;
    config.path = ".";
    ImGuiFileDialog::Instance()->OpenDialog(title, "Choose Folder", nullptr, config);
}

void FileDialog::Display(const char* title, const std::function<void(const std::filesystem::path& path)>& callback)
{
    if (ImGuiFileDialog::Instance()->Display(title))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
            callback(filePath);
        }
        ImGuiFileDialog::Instance()->Close();
    }
}