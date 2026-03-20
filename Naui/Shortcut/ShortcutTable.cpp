#include "ShortcutTable.h"
#include <imgui.h>
#include <algorithm>
#include <string>

namespace Naui {

ShortcutChord ShortcutTable::Normalize(ShortcutChord chord)
{
	std::sort(chord.begin(), chord.end());
	chord.erase(std::unique(chord.begin(), chord.end()), chord.end());
	return chord;
}

size_t ShortcutTable::Hash(const ShortcutChord& chord)
{
	size_t hash = 14695981039346656037ULL;
	for (ImGuiKey key : chord)
	{
		hash ^= static_cast<size_t>(key);
		hash *= 1099511628211ULL;
	}
	return hash;
}

float ShortcutTable::ComputeInfluence(const ShortcutEntry& entry)
{
	if (entry.hardWeight.has_value())
		return entry.hardWeight.value();

	return static_cast<float>(entry.lastTriggered);
}

ShortcutTable::ShortcutTable(ShortcutConflictMode mode) : m_mode(mode){}

bool ShortcutTable::Register(const std::string& id, ShortcutChord chord, int priority, std::function<void()> callback)
{
	if (m_entries.count(id))
	{
		return false;
	}

	chord = Normalize(chord);
	if (chord.empty())
	{
		return false;
	}

	const size_t hash = Hash(chord);
	if (m_mode == ShortcutConflictMode::UniqueChords && m_chordIndex.count(hash))
		return false;

	m_entries[id] = { id, chord, priority, std::nullopt, 0, false, false, std::move(callback) };
	m_chordIndex[hash].push_back(id);
	return true;
}

bool ShortcutTable::Remap(const std::string& id, ShortcutChord newChord)
{
	auto entryIt = m_entries.find(id);
	if (entryIt == m_entries.end())
		return false;

	newChord = Normalize(newChord);
	if (newChord.empty())
		return false;

	const size_t newHash = Hash(newChord);
	const size_t oldHash = Hash(entryIt->second.chord);
	if (newHash != oldHash)
	{
		if (m_mode == ShortcutConflictMode::UniqueChords && m_chordIndex.count(newHash))
			return false;

		auto& oldList = m_chordIndex[oldHash];
		oldList.erase(std::remove(oldList.begin(), oldList.end(), id), oldList.end());
		if (oldList.empty())
			m_chordIndex.erase(oldHash);

		m_chordIndex[newHash].push_back(id);
	}

	entryIt->second.chord = newChord;
	return true;
}

void ShortcutTable::SetIgnoreMouseInput(const std::string& id, bool ignore)
{
	auto it = m_entries.find(id);
	if (it != m_entries.end())
		it->second.ignoreMouseInput = ignore;
}

// Collapses Right-side modifier keys to their Left equivalent.
// Agnostic chords are stored under Left keys so PollImpl's collapsed chord path matches them regardless of which side was pressed.
static ImGuiKey CollapseToLeft(ImGuiKey k)
{
	switch (k)
	{
		case ImGuiKey_RightCtrl: return ImGuiKey_LeftCtrl;
		case ImGuiKey_RightShift: return ImGuiKey_LeftShift;
		case ImGuiKey_RightAlt: return ImGuiKey_LeftAlt;
		case ImGuiKey_RightSuper: return ImGuiKey_LeftSuper;
		default: return k;
	}
}

void ShortcutTable::SetSideAgnostic(const std::string& id, bool agnostic)
{
	auto it = m_entries.find(id);
	if (it == m_entries.end())
		return;

	it->second.sideAgnostic = agnostic;
	ShortcutChord reindexed;
	for (ImGuiKey k : it->second.chord)
	{
		reindexed.push_back(agnostic ? CollapseToLeft(k) : k);
	}

	reindexed = Normalize(reindexed);
	const size_t oldHash = Hash(it->second.chord);
	const size_t newHash = Hash(reindexed);
	if (newHash != oldHash)
	{
		auto& oldList = m_chordIndex[oldHash];
		oldList.erase(std::remove(oldList.begin(), oldList.end(), id), oldList.end());
		if (oldList.empty())
			m_chordIndex.erase(oldHash);

		it->second.chord = reindexed;
		m_chordIndex[newHash].push_back(id);
	}
}

void ShortcutTable::SetHardWeight(const std::string& id, float weight)
{
	auto it = m_entries.find(id);
	if (it != m_entries.end())
		it->second.hardWeight = weight;
}

void ShortcutTable::ClearHardWeight(const std::string& id)
{
	auto it = m_entries.find(id);
	if (it != m_entries.end())
		it->second.hardWeight = std::nullopt;
}

void ShortcutTable::SetAllHardWeights(float weight)
{
	for (auto& [id, entry] : m_entries)
		entry.hardWeight = weight;
}

void ShortcutTable::ClearAllHardWeights()
{
	for (auto& [id, entry] : m_entries)
	{
		entry.hardWeight = std::nullopt;
	}
}

const ShortcutChord* ShortcutTable::GetChord(const std::string& id) const
{
	auto it = m_entries.find(id);
	if (it == m_entries.end())
		return nullptr;

	return &it->second.chord;
}

std::vector<ShortcutEntry*> ShortcutTable::FindByChord(size_t chordHash)
{
	std::vector<ShortcutEntry*> results;
	auto indexIt = m_chordIndex.find(chordHash);
	if (indexIt == m_chordIndex.end())
		return results;

	for (const std::string& id : indexIt->second)
	{
		auto entryIt = m_entries.find(id);
		if (entryIt != m_entries.end())
			results.push_back(&entryIt->second);
	}

	return results;
}

void ShortcutTable::RecordTrigger(const std::string& id, uint64_t frameCounter)
{
	auto it = m_entries.find(id);
	if (it != m_entries.end())
		it->second.lastTriggered = frameCounter;
}

std::string ShortcutTable::ChordToString(const ShortcutChord& chord)
{
	auto DisplayName = [](ImGuiKey key) -> std::string
	{
		switch (key)
		{
			case ImGuiKey_LeftCtrl:
			case ImGuiKey_RightCtrl:	return "Ctrl";
			case ImGuiKey_LeftShift:
			case ImGuiKey_RightShift:   return "Shift";
			case ImGuiKey_LeftAlt:
			case ImGuiKey_RightAlt:	 return "Alt";
			case ImGuiKey_LeftSuper:
			case ImGuiKey_RightSuper:   return "Super";
			case ImGuiKey_Delete:	   return "Delete";
			case ImGuiKey_Backspace:	return "Backspace";
			case ImGuiKey_Tab:		  return "Tab";
			case ImGuiKey_Escape:	   return "Esc";
			case ImGuiKey_Enter:		return "Enter";
			case ImGuiKey_Space:		return "Space";
			case ImGuiKey_UpArrow:	  return "Up";
			case ImGuiKey_DownArrow:	return "Down";
			case ImGuiKey_LeftArrow:	return "Left";
			case ImGuiKey_RightArrow:   return "Right";
			case ImGuiKey_PageUp:	   return "PgUp";
			case ImGuiKey_PageDown:	 return "PgDn";
			case ImGuiKey_Home:		 return "Home";
			case ImGuiKey_End:		  return "End";
			case ImGuiKey_Insert:	   return "Insert";
			default:
			{
				const char* name = ImGui::GetKeyName(key);
				return name ? name : "?";
			}
		}
	};

	auto IsModifier = [](ImGuiKey key) -> bool
	{
		return key == ImGuiKey_LeftCtrl		|| key == ImGuiKey_RightCtrl	||
				key == ImGuiKey_LeftShift	|| key == ImGuiKey_RightShift	||
				key == ImGuiKey_LeftAlt		|| key == ImGuiKey_RightAlt		||
				key == ImGuiKey_LeftSuper	|| key == ImGuiKey_RightSuper;
	};

	ShortcutChord sorted = chord;
	std::stable_sort(sorted.begin(), sorted.end(), [&](ImGuiKey a, ImGuiKey b)
	{
		return IsModifier(a) && !IsModifier(b);
	});

	std::string result;
	std::string prev;
	for (ImGuiKey key : sorted)
	{
		std::string name = DisplayName(key);
		if (name == prev)
			continue;
		if (!result.empty())
			result += " + ";

		result += name;
		prev = name;
	}

	return result;
}


}