#include "AppShortcuts.h"
#include "Core/ProjectManager.h"
#include "UI/Modals/Settings/SettingsPanel.h"

#include "Naui/FileSystem/FileDialog.h"
#include "Naui/Actions/ActionManager.h"
#include "Actions/Project/SaveProjectAction.h"
#include <iostream>

static constexpr int PRIORITY_GLOBAL = 10;
static constexpr int PRIORITY_PANEL  = 20;

namespace AppShortcuts {

void RegisterGlobal()
{
	Naui::ShortcutTable& g = Naui::Shortcut::Global();

	g.Register("app.save",		{ ImGuiKey_LeftCtrl, ImGuiKey_S },		PRIORITY_GLOBAL, []
	{
		ProjectState& state = ProjectState::GetInstance();
		state.actionManager.ExecuteWithoutHistory<SaveProjectAction>(state, false);
	});

	g.Register("app.save_as",	{ ImGuiKey_LeftCtrl, ImGuiKey_LeftShift, ImGuiKey_S },		PRIORITY_GLOBAL, []
	{
		ProjectState& state = ProjectState::GetInstance();
		state.actionManager.ExecuteWithoutHistory<SaveProjectAction>(state, true);
	});

	g.Register("app.open",		{ ImGuiKey_LeftCtrl, ImGuiKey_O },		PRIORITY_GLOBAL, []{ FileDialog::OpenFile("open_project", "Open Project", ".uph"); });
	g.Register("app.undo",		{ ImGuiKey_LeftCtrl, ImGuiKey_Z },		PRIORITY_GLOBAL, []{ ProjectState::GetInstance().actionManager.Undo(); });
	g.Register("app.redo",		{ ImGuiKey_LeftCtrl, ImGuiKey_Y },		PRIORITY_GLOBAL, []{ ProjectState::GetInstance().actionManager.Redo(); });
	g.Register("app.delete",	{ ImGuiKey_Delete },					PRIORITY_GLOBAL, []{ /* global delete fallback */ });
	g.Register("app.settings",	{ ImGuiKey_F10 },						PRIORITY_GLOBAL, []{ Naui::TriggerModal<SettingsPanel>(); });
}

}