#pragma once

#include "Naui.h"

class MainMenuBar
{
public:
	static void FileMenu();
	static void EditMenu();
	static void OptionsMenu();
	static void ViewMenu();
	static void LayoutMenu();
	static void HelpMenu();
	static void ShowSaveAsPopup();
	static void ShowOverridePopup();
	static void RenderPopups();

private:
	static double GetTimelineDuration();
};