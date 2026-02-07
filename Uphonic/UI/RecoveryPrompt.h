#pragma once
#include "Naui.h"
#include <filesystem>
#include <optional>

class RecoveryPrompt : public Naui::Panel
{
public:
	RecoveryPrompt();
	std::optional<std::filesystem::path> recoverPath;

protected:
	void OnRender() override;

};