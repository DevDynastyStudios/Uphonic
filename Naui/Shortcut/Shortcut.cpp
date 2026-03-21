#include "Shortcut.h"
#include "FileSystem/File.h"
#include "Vendor/nlohmann/json.hpp"
#include <imgui.h>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <string>
#include <span>

namespace Naui {

static ShortcutTable s_global;
static uint64_t s_frameCounter = 0;
static Shortcut::ConflictCallback s_conflictCallback;

ShortcutTable& Shortcut::Global()
{
	return s_global;
}

void Shortcut::SetConflictCallback(ConflictCallback cb)
{
	s_conflictCallback = std::move(cb);
}

const ShortcutChord* Shortcut::GetChord(const std::string& id)
{
	return s_global.GetChord(id);
}

std::string Shortcut::GetChordString(const std::string& id)
{
	const ShortcutChord* chord = s_global.GetChord(id);
	if (!chord)
		return {};

	return ShortcutTable::ChordToString(*chord);
}

static std::pair<ShortcutEntry*, bool> Resolve(std::vector<ShortcutEntry*>& candidates)
{
	if (candidates.empty())
		return { nullptr, false };

	if (candidates.size() == 1)
		return { candidates[0], false };

	std::stable_sort(candidates.begin(), candidates.end(), [](const ShortcutEntry* a, const ShortcutEntry* b)
	{
		if (a->priority != b->priority)
			return a->priority > b->priority;
			
		return ShortcutTable::ComputeInfluence(*a) > ShortcutTable::ComputeInfluence(*b);
	});

	const bool hadTie = candidates[0]->priority == candidates[1]->priority && ShortcutTable::ComputeInfluence(*candidates[0]) == ShortcutTable::ComputeInfluence(*candidates[1]);
	return { candidates[0], hadTie };
}

static void PollImpl(std::vector<ShortcutTable*>& tables)
{
	++s_frameCounter;
	auto CollapseToLeft = [](Key k) -> Key
	{
		switch (k)
		{
			case ImGuiKey_RightCtrl: return ImGuiKey_LeftCtrl;
			case ImGuiKey_RightShift: return ImGuiKey_LeftShift;
			case ImGuiKey_RightAlt: return ImGuiKey_LeftAlt;
			case ImGuiKey_RightSuper: return ImGuiKey_LeftSuper;
			default: return k;
		}
	};

	auto IsMouse = [](Key k) -> bool
	{
		return k >= ImGuiKey_MouseLeft && k <= ImGuiKey_MouseWheelY;
	};

	ShortcutChord specificKeys;
	ShortcutChord agnosticKeys;
	bool anyJustPressed = false;

	for (ImGuiKey key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key = static_cast<ImGuiKey>(key + 1))
	{
		if (key >= ImGuiKey_MouseX1 && key <= ImGuiKey_MouseWheelY)
			continue;

		if (key >= ImGuiKey_GamepadStart && key <= ImGuiKey_GamepadRStickDown)
			continue;

		const char* keyName = ImGui::GetKeyName(key);
		if (keyName && keyName[0] == 'M' && keyName[1] == 'o' && keyName[2] == 'd')
			continue;

		if (!ImGui::IsKeyDown(key))
			continue;

		if (ImGui::IsKeyPressed(key, false))
			anyJustPressed = true;

		specificKeys.push_back(key);
		agnosticKeys.push_back(CollapseToLeft(key));
	}

	if (!anyJustPressed || specificKeys.empty())
		return;

	specificKeys = ShortcutTable::Normalize(specificKeys);
	agnosticKeys = ShortcutTable::Normalize(agnosticKeys);

	ShortcutChord specificNoMouse;
	ShortcutChord agnosticNoMouse;
	for (Key k : specificKeys)
	{
		if (!IsMouse(k))
			specificNoMouse.push_back(k);
	}

	for (Key k : agnosticKeys)
	{
		if (!IsMouse(k))
			agnosticNoMouse.push_back(k);
	}

	specificNoMouse = ShortcutTable::Normalize(specificNoMouse);
	agnosticNoMouse = ShortcutTable::Normalize(agnosticNoMouse);

	const size_t hashSpecific = ShortcutTable::Hash(specificKeys);
	const size_t hashAgnostic = ShortcutTable::Hash(agnosticKeys);
	const size_t hashSpecificNoMouse = ShortcutTable::Hash(specificNoMouse);
	const size_t hashAgnosticNoMouse = ShortcutTable::Hash(agnosticNoMouse);
	std::vector<ShortcutEntry*> candidates;

	auto Gather = [&](ShortcutTable* table)
	{
		if (!table) return;

		for (ShortcutEntry* e : table->FindByChord(hashSpecific))
		{
			if (!e->sideAgnostic && !e->ignoreMouseInput)
				candidates.push_back(e);
		}

		for (ShortcutEntry* e : table->FindByChord(hashAgnostic))
		{
			if (e->sideAgnostic && !e->ignoreMouseInput)
				candidates.push_back(e);
		}

		if (hashSpecificNoMouse != hashSpecific)
		{
			for (ShortcutEntry* e : table->FindByChord(hashSpecificNoMouse))
			{
				if (!e->sideAgnostic && e->ignoreMouseInput)
					candidates.push_back(e);
			}
		}

		if (hashAgnosticNoMouse != hashAgnostic)
		{
			for (ShortcutEntry* e : table->FindByChord(hashAgnosticNoMouse))
			{
				if (e->sideAgnostic && e->ignoreMouseInput)
					candidates.push_back(e);
			}
		}
	};

	Gather(&Shortcut::Global());
	for (ShortcutTable* t : tables)
	{
		Gather(t);
	}

	if (candidates.empty())
		return;

	auto [winner, hadTie] = Resolve(candidates);
	if (!winner || !winner->callback)
		return;

	if (hadTie && s_conflictCallback)
		s_conflictCallback({ candidates[0]->id, candidates[1]->id, specificKeys });

	winner->callback();
	auto RecordIn = [&](ShortcutTable* table)
	{
		if (!table)
			return;

		const size_t hashes[4] = { hashSpecific, hashAgnostic, hashSpecificNoMouse, hashAgnosticNoMouse };
		for (size_t h : hashes)
		{
			for (ShortcutEntry* e : table->FindByChord(h))
			{
				if (e == winner)
				{
					table->RecordTrigger(winner->id, s_frameCounter);
					return;
				}
			}
		}
	};

	RecordIn(&Shortcut::Global());
	for (ShortcutTable* t : tables)
	{
		RecordIn(t);
	}
}

void Shortcut::Poll(std::initializer_list<ShortcutTable*> tables)
{
	std::vector<ShortcutTable*> vec(tables);
	PollImpl(vec);
}

void Shortcut::Poll(std::span<ShortcutTable*> tables)
{
	std::vector<ShortcutTable*> vec(tables.begin(), tables.end());
	PollImpl(vec);
}

static std::filesystem::path ResolveShortcutPath(std::filesystem::path path)
{
	if (path.empty())
		return Naui::Directory::BinDirectory() / "shortcuts.json";

	if (!path.has_parent_path() || path.parent_path() == path.root_path())
		return Naui::Directory::BinDirectory() / path;

	return path;
}

static void WriteTable(nlohmann::json& j, const std::string& scope, ShortcutTable* table)
{
	if (!table)
		return;

	nlohmann::json entries = nlohmann::json::object();
	for (ShortcutEntry* e : table->AllEntries())
	{
		nlohmann::json keys = nlohmann::json::array();
		for (Key k : e->chord)
		{
			keys.push_back(ImGui::GetKeyName(static_cast<ImGuiKey>(k)));
		}

		entries[e->id] = keys;
	}

	if (!entries.empty())
		j[scope] = entries;
}

static void ReadTable(const nlohmann::json& j, const std::string& scope, ShortcutTable* table)
{
	if (!table)
		return;

	auto scopeIt = j.find(scope);
	if (scopeIt == j.end())
		return;

	for (auto& [id, keyArray] : scopeIt->items())
	{
		ShortcutChord chord;
		for (const auto& keyName : keyArray)
		{
			const std::string name = keyName.get<std::string>();
			for (ImGuiKey k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; k = static_cast<ImGuiKey>(k + 1))
			{
				const char* n = ImGui::GetKeyName(k);
				if (n && name == n)
				{
					chord.push_back(k);
					break;
				}
			}
		}

		if (!chord.empty())
			table->Remap(id, chord);
	}
}

static bool SaveImpl(std::filesystem::path path, std::vector<ShortcutTable*>& tables)
{
	path = ResolveShortcutPath(path);
	nlohmann::json j;
	WriteTable(j, "global", &Shortcut::Global());
	for (size_t i = 0; i < tables.size(); ++i)
	{
		WriteTable(j, "panel_" + std::to_string(i), tables[i]);
	}

	std::ofstream file(path);
	if (!file.is_open())
		return false;

	file << j.dump(4);
	return file.good();
}

static bool LoadImpl(std::filesystem::path path, std::vector<ShortcutTable*>& tables)
{
	path = ResolveShortcutPath(path);
	std::ifstream file(path);
	if (!file.is_open())
		return false;

	nlohmann::json j;
	try
	{
		j = nlohmann::json::parse(file);
	}
	catch (...)
	{
		return false;
	}

	ReadTable(j, "global", &Shortcut::Global());
	for (size_t i = 0; i < tables.size(); ++i)
	{
		ReadTable(j, "panel_" + std::to_string(i), tables[i]);
	}

	return true;
}

bool Shortcut::Save(std::filesystem::path path, std::initializer_list<ShortcutTable*> tables)
{
	std::vector<ShortcutTable*> vec(tables);
	return SaveImpl(path, vec);
}

bool Shortcut::Save(std::filesystem::path path, std::span<ShortcutTable*> tables)
{
	std::vector<ShortcutTable*> vec(tables.begin(), tables.end());
	return SaveImpl(path, vec);
}

bool Shortcut::Load(std::filesystem::path path, std::initializer_list<ShortcutTable*> tables)
{
	std::vector<ShortcutTable*> vec(tables);
	return LoadImpl(path, vec);
}

bool Shortcut::Load(std::filesystem::path path, std::span<ShortcutTable*> tables)
{
	std::vector<ShortcutTable*> vec(tables.begin(), tables.end());
	return LoadImpl(path, vec);
}

}