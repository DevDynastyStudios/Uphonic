#pragma once

struct ApplicationSettings;

class PluginTab
{
public:
	void Draw(ApplicationSettings& draft);

private:
	int  m_editingPathIndex = -1;
	char m_pathBuffer[512]  = {};
};