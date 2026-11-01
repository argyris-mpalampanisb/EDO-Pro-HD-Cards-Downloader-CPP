#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

namespace edopro {

struct DownloadItem {
    uint32_t id;
    bool is_field;
};

// Fetch ALL image IDs from the full card list
std::vector<uint32_t> get_all_card_ids();

// Fetch field spell image IDs
std::vector<uint32_t> get_all_field_ids();

// Search cards by name
std::vector<DownloadItem> search_cards(const std::string& name);

// Parse a .ydk deck file
std::vector<DownloadItem> parse_deck(const std::filesystem::path& path);

} // namespace edopro   