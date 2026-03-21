#pragma warning(push)
#pragma warning(disable : 4251)
#pragma once
#include "Base.h"
#include "Key.h"

#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <cstdint>

namespace Naui {

using ShortcutChord = std::vector<Key>;

enum class ShortcutConflictMode
{
	UniqueChords,
	AllowShared,
};

struct ShortcutConflict
{
	std::string winnerID;
	std::string loserID;
	ShortcutChord chord;
};

struct NAUI_API ShortcutEntry
{
	std::string id;
	ShortcutChord chord;
	int priority = 0;
	std::optional<float> hardWeight;
	uint64_t lastTriggered = 0;
	bool ignoreMouseInput = true;	// Mouse buttons held simultaneously are ignored
	bool sideAgnostic = false;		// LeftCtrl and RightCtrl treated as equivalent, etc.
	std::function<void()> callback;
};

class NAUI_API ShortcutTable
{
public:
	static constexpr float FOCUS_WEIGHT = 1e9f;

	explicit ShortcutTable(ShortcutConflictMode mode = ShortcutConflictMode::AllowShared);

	bool Register(const std::string& id, ShortcutChord chord, int priority, std::function<void()> callback);
	bool Remap(const std::string& id, ShortcutChord newChord);

	void SetHardWeight(const std::string& id, float weight);
	void ClearHardWeight(const std::string& id);
	void SetAllHardWeights(float weight);
	void ClearAllHardWeights();

	// When true, any mouse buttons held alongside the chord are ignored.
	void SetIgnoreMouseInput(const std::string& id, bool ignore);

	// When true, LeftCtrl and RightCtrl (and other modifier pairs) are treated
	// as equivalent — either side fires the shortcut.
	void SetSideAgnostic(const std::string& id, bool agnostic);

	const ShortcutChord* GetChord(const std::string& id) const;

	std::vector<ShortcutEntry*> FindByChord(size_t chordHash);
	void RecordTrigger(const std::string& id, uint64_t frameCounter);

	ShortcutConflictMode GetMode() const { return m_mode; }
	size_t EntryCount() const { return m_entries.size(); }

	std::vector<ShortcutEntry*> AllEntries()
	{
		std::vector<ShortcutEntry*> out;
		out.reserve(m_entries.size());
		for (auto& [id, entry] : m_entries)
		{
			out.push_back(&entry);
		}

		return out;
	}

	static std::string ChordToString(const ShortcutChord& chord);
	static ShortcutChord Normalize(ShortcutChord chord);
	static size_t Hash(const ShortcutChord& chord);
	static float ComputeInfluence(const ShortcutEntry& entry);

private:
	ShortcutConflictMode m_mode;

	std::unordered_map<std::string, ShortcutEntry> m_entries;
	std::unordered_map<size_t, std::vector<std::string>> m_chordIndex;
};

}
#pragma warning(pop)