#pragma once
#include "Base.h"
#include "ShortcutTable.h"
#include <filesystem>
#include <functional>
#include <span>

namespace Naui {

class NAUI_API Shortcut
{
public:
	// The global table — always polled regardless of what tables are passed in.
	static ShortcutTable& Global();

	// Called when two entries are completely tied (same chord, priority, and influence).
	// First registered will be picked.
	using ConflictCallback = std::function<void(const ShortcutConflict&)>;
	static void SetConflictCallback(ConflictCallback cb);

	// Returns the display string for an action in the global table, e.g. "Ctrl + S".
	// Returns an empty string if the id is not found.
	static std::string GetChordString(const std::string& id);

	// Returns the raw chord for an action in the global table, or nullptr if not found.
	static const ShortcutChord* GetChord(const std::string& id);

	static bool Save(std::filesystem::path path = {}, std::initializer_list<ShortcutTable*> tables = {});
	static bool Save(std::filesystem::path path, std::span<ShortcutTable*> tables);

	static bool Load(std::filesystem::path path = {}, std::initializer_list<ShortcutTable*> tables = {});
	static bool Load(std::filesystem::path path, std::span<ShortcutTable*> tables);

	static void Poll(std::initializer_list<ShortcutTable*> tables = {});
	static void Poll(std::span<ShortcutTable*> tables);
};

}