#include "Debug.h"
#include <imgui.h>
#include <string>
#include <vector>
#include <cstdarg>

namespace Naui
{
static std::vector<std::string> s_errors;
static bool s_modalOpen = false;

void Debug::Error(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), format, args);
	s_errors.push_back(buffer);
	va_end(args);
	
	// Signal that we need to open a modal
	if (!s_modalOpen && !s_errors.empty())
	{
		s_modalOpen = true;
	}
}

void Debug::Render(void)
{
	// Open the modal popup when we have errors
	if (s_modalOpen && !s_errors.empty())
	{
		ImGui::OpenPopup("Error");
		s_modalOpen = false;
	}

	// Center the modal on screen
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	// Display modal
	if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (!s_errors.empty())
		{
			ImGui::Text("%s", s_errors.front().c_str());
			ImGui::Separator();
			
			if (ImGui::Button("OK", ImVec2(120, 0)))
			{
				s_errors.erase(s_errors.begin());
				ImGui::CloseCurrentPopup();
				
				// If there are more errors, open the next modal
				if (!s_errors.empty())
				{
					s_modalOpen = true;
				}
			}
			
			ImGui::SetItemDefaultFocus();
		}
		
		ImGui::EndPopup();
	}
}
}