#pragma once
#include <filesystem>
#include <vector>
#include <optional>

class Recovery
{
public:
	static void Recover(const std::filesystem::path& projectDir);
	static void Discard(const std::filesystem::path& projectDir);
	static std::optional<std::filesystem::path> TryGetLastSession();
};