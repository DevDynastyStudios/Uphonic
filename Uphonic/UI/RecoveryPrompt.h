#pragma once
#include "Naui.h"

class RecoveryPrompt : public Naui::Panel
{
public:
	RecoveryPrompt();

protected:
	void OnRender() override;
};