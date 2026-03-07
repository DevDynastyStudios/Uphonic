#include "Theme.h"
#include "FileSystem/File.h"

#include <nlohmann/json.hpp>

#include <string>
#include <fstream>
#include <unordered_map>

namespace Naui
{

static std::unordered_map<std::string, ImGuiCol> s_imguiColorMap = {
	{"Text", ImGuiCol_Text},
	{"TextDisabled", ImGuiCol_TextDisabled},
	{"WindowBg", ImGuiCol_WindowBg},
	{"ChildBg", ImGuiCol_ChildBg},
	{"PopupBg", ImGuiCol_PopupBg},
	{"Border", ImGuiCol_Border},
	{"BorderShadow", ImGuiCol_BorderShadow},
	{"FrameBg", ImGuiCol_FrameBg},
	{"FrameBgHovered", ImGuiCol_FrameBgHovered},
	{"FrameBgActive", ImGuiCol_FrameBgActive},
	{"TitleBg", ImGuiCol_TitleBg},
	{"TitleBgActive", ImGuiCol_TitleBgActive},
	{"TitleBgCollapsed", ImGuiCol_TitleBgCollapsed},
	{"MenuBarBg", ImGuiCol_MenuBarBg},
	{"ScrollbarBg", ImGuiCol_ScrollbarBg},
	{"ScrollbarGrab", ImGuiCol_ScrollbarGrab},
	{"ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered},
	{"ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive},
	{"CheckMark", ImGuiCol_CheckMark},
	{"SliderGrab", ImGuiCol_SliderGrab},
	{"SliderGrabActive", ImGuiCol_SliderGrabActive},
	{"Button", ImGuiCol_Button},
	{"ButtonHovered", ImGuiCol_ButtonHovered},
	{"ButtonActive", ImGuiCol_ButtonActive},
	{"Header", ImGuiCol_Header},
	{"HeaderHovered", ImGuiCol_HeaderHovered},
	{"HeaderActive", ImGuiCol_HeaderActive},
	{"Separator", ImGuiCol_Separator},
	{"SeparatorHovered", ImGuiCol_SeparatorHovered},
	{"SeparatorActive", ImGuiCol_SeparatorActive},
	{"ResizeGrip", ImGuiCol_ResizeGrip},
	{"ResizeGripHovered", ImGuiCol_ResizeGripHovered},
	{"ResizeGripActive", ImGuiCol_ResizeGripActive},
	{"Tab", ImGuiCol_Tab},
	{"TabHovered", ImGuiCol_TabHovered},
	{"TabSelected", ImGuiCol_TabSelected},
	{"TabSelectedOverline", ImGuiCol_TabSelectedOverline},
	{"TabDimmed", ImGuiCol_TabDimmed},
	{"TabDimmedSelected", ImGuiCol_TabDimmedSelected},
	{"TabDimmedSelectedOverline", ImGuiCol_TabDimmedSelectedOverline},
	{"DockingPreview", ImGuiCol_DockingPreview},
	{"DockingEmptyBg", ImGuiCol_DockingEmptyBg},
	{"PlotLines", ImGuiCol_PlotLines},
	{"PlotLinesHovered", ImGuiCol_PlotLinesHovered},
	{"PlotHistogram", ImGuiCol_PlotHistogram},
	{"PlotHistogramHovered", ImGuiCol_PlotHistogramHovered},
	{"TableHeaderBg", ImGuiCol_TableHeaderBg},
	{"TableBorderStrong", ImGuiCol_TableBorderStrong},
	{"TableBorderLight", ImGuiCol_TableBorderLight},
	{"TableRowBg", ImGuiCol_TableRowBg},
	{"TableRowBgAlt", ImGuiCol_TableRowBgAlt},
	{"TextLink", ImGuiCol_TextLink},
	{"TextSelectedBg", ImGuiCol_TextSelectedBg},
	{"TreeLines", ImGuiCol_TreeLines},
	{"DragDropTarget", ImGuiCol_DragDropTarget},
	{"DragDropTargetBg", ImGuiCol_DragDropTargetBg},
	{"NavCursor", ImGuiCol_NavCursor},
	{"NavWindowingHighlight", ImGuiCol_NavWindowingHighlight},
	{"NavWindowingDimBg", ImGuiCol_NavWindowingDimBg},
	{"ModalWindowDimBg", ImGuiCol_ModalWindowDimBg}
};

static std::unordered_map<std::string, ImColor> s_customColors;
static std::unordered_map<std::string, ImVec2>  s_customVec2s;
static std::unordered_map<std::string, float>   s_customFloats;

static ImVec4 ColorFromHex(const std::string& hex)
{
	auto hexval = [](char c) -> int {
		if (c >= '0' && c <= '9') return c - '0';
		if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
		if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
		return 0;
	};
	auto byte = [&](const char* p) -> int {
		return (hexval(p[0]) << 4) | hexval(p[1]);
	};

	int r = 255, g = 255, b = 255, a = 255;
	if (hex.size() >= 7)
	{
		r = byte(hex.c_str() + 1);
		g = byte(hex.c_str() + 3);
		b = byte(hex.c_str() + 5);
		if (hex.size() >= 9)
			a = byte(hex.c_str() + 7);
	}
	constexpr float inv = 1.0f / 255.0f;
	return ImVec4(r * inv, g * inv, b * inv, a * inv);
}

static ImVec2 ReadVec2(const nlohmann::json& v)
{
	return ImVec2(v[0].get<float>(), v[1].get<float>());
}

static void ApplyColors(const nlohmann::json& colors)
{
	ImVec4* c = ImGui::GetStyle().Colors;

	for (auto& [key, value] : colors.items())
	{
		if (!value.is_string())
			continue;

		const std::string hex = value.get<std::string>();
		auto it = s_imguiColorMap.find(key);
		if (it != s_imguiColorMap.end())
			c[it->second] = ColorFromHex(hex);
		else
			s_customColors[key] = ColorFromHex(hex);
	}
}

static void ApplyStyle(const nlohmann::json& style)
{
	ImGuiStyle& s = ImGui::GetStyle();

	for (auto& [key, value] : style.items())
	{
		if (key == "Font")
			continue;

		if (value.is_array() && value.size() == 2)
		{
			const ImVec2 v = ReadVec2(value);
			if		(key == "WindowPadding")	s.WindowPadding		= v;
			else if (key == "FramePadding")		s.FramePadding		= v;
			else if (key == "ItemSpacing")		s.ItemSpacing		= v;
			else if (key == "ItemInnerSpacing")	s.ItemInnerSpacing	= v;
			else if (key == "WindowTitleAlign")	s.WindowTitleAlign	= v;
			else s_customVec2s[key] = v;
		}
		else if (value.is_number())
		{
			if	  (key == "IndentSpacing")		s.IndentSpacing		= value;
			else if (key == "WindowRounding")		s.WindowRounding	= value;
			else if (key == "ChildRounding")		s.ChildRounding		= value;
			else if (key == "FrameRounding")		s.FrameRounding		= value;
			else if (key == "PopupRounding")		s.PopupRounding		= value;
			else if (key == "ScrollbarRounding")	s.ScrollbarRounding	= value;
			else if (key == "GrabRounding")			s.GrabRounding		= value;
			else if (key == "TabRounding")			s.TabRounding		= value;
			else if (key == "ScrollbarSize")		s.ScrollbarSize		= value;
			else if (key == "GrabMinSize")			s.GrabMinSize		= value;
			else if (key == "WindowBorderSize")		s.WindowBorderSize	= value;
			else if (key == "FrameBorderSize")		s.FrameBorderSize	= value;
			else if (key == "PopupBorderSize")		s.PopupBorderSize	= value;
			else s_customFloats[key] = value.get<float>();
		}
		else if (value.is_boolean())
		{
			if (key == "AntiAliasedLines") s.AntiAliasedLines = value.get<bool>();
			else if (key == "AntiAliasedFill")  s.AntiAliasedFill  = value.get<bool>();
		}
	}
}

static void ApplyFont(const nlohmann::json& font)
{
	if (!font.contains("File") || !font["File"].is_string())
		return;

	const std::filesystem::path fontPath = Naui::Directory::BinDirectory() / font["File"].get<std::string>();
	if (!std::filesystem::exists(fontPath))
		return;

	const float size = font.value("Size", 16.0f);
	const int   oversampleH = font.value("OversampleH", 3);
	const int   oversampleV = font.value("OversampleV", 3);
	const float rasterizerMultiply = font.value("RasterizerMultiply", 1.0f);

	ImFontConfig cfg;
	cfg.OversampleH		= oversampleH;
	cfg.OversampleV		= oversampleV;
	cfg.RasterizerMultiply = rasterizerMultiply;

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Clear();
	io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), size, &cfg);

}

void Theme::Load(const char* path)
{
	std::ifstream file(path);
	if (!file.is_open())
		return;

	nlohmann::json j;
	try
	{
		file >> j;
	}
	catch (...) 
	{
		return;
	}

	s_customColors.clear();
	s_customVec2s.clear();
	s_customFloats.clear();

	const bool isCombined = j.contains("Style") || j.contains("Colors");

	if (isCombined)
	{
		if (j.contains("Colors") && j["Colors"].is_object())
			ApplyColors(j["Colors"]);

		if (j.contains("Style") && j["Style"].is_object())
		{
			ApplyStyle(j["Style"]);

			if (j["Style"].contains("Font") && j["Style"]["Font"].is_object())
				ApplyFont(j["Style"]["Font"]);
		}
	}
	else
	{
		ApplyColors(j);
		ApplyStyle(j);
	}

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::GetStyle().WindowRounding = 0.0f;
		ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
}

void Theme::LoadDefault()
{
	const std::filesystem::path path = Naui::Directory::BinDirectory() / "Themes" / "Default.json";
	Load(path.string().c_str());
}

ImColor Theme::GetColor(const char* name) { return s_customColors[name]; }
ImVec2  Theme::GetVec2(const char* name)  { return s_customVec2s[name];  }
float   Theme::GetFloat(const char* name) { return s_customFloats[name]; }

}