#include <CLI11.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "api.h"
#include "config.h"
#include "downloader.h"
#include "tracker.h"
#include <optional>

using namespace edopro;

static std::vector<DownloadItem> collect_all() {
    std::vector<DownloadItem> items;
    auto card_ids = get_all_card_ids();
    for (auto id : card_ids) items.push_back({id, false});
    auto field_ids = get_all_field_ids();
    for (auto id : field_ids) items.push_back({id, true});
    return items;
}

static void run_download(const Config& config, Tracker& tracker,
                         std::vector<DownloadItem>& items, bool force) {
    auto to_download = [&]() {
        std::vector<DownloadItem> result;
        for (const auto& item : items) {
            if (force || !tracker.is_downloaded(item))
                result.push_back(item);
        }
        return result;
    }();

    if (to_download.empty()) {
        std::cout << "Everything is up to date!\n";
        return;
    }

    std::cout << "Found " << to_download.size() << " new images to download.\n";
    auto result = download_all(config, to_download);

    for (const auto& item : to_download) {
        tracker.mark_downloaded(item);
    }
    tracker.save();

    std::cout << "\nDone: " << result.succeeded << " succeeded, "
              << result.failed << " failed.\n";
}

static void interactive_mode(const Config& config, Tracker& tracker, bool force) {
    std::cout << "=== EDOPro HD Downloader (interactive) ===\n";
    std::cout << "Commands: /allcards, /allfields, /deck <name>, /ids <123,456>, /clean, /quit\n";

    std::string input;
    while (true) {
        std::cout << "\n> " << std::flush;
        if (!std::getline(std::cin, input)) break;

        // Trim
        auto start = input.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        auto end = input.find_last_not_of(" \t\r\n");
        input = input.substr(start, end - start + 1);

        if (input.empty()) continue;
        if (input == "/quit" || input == "/exit") break;

        if (input == "/clean") {
            std::cout << "Wiping " << config.pics_path << " ...\n";
            config.wipe_pics();
            std::cout << "Done. Starting fresh download.\n\n";

            auto items = collect_all();
            std::cout << "Found " << items.size() << " images to download.\n";
            auto result = download_all(config, items);
            for (const auto& item : items) tracker.mark_downloaded(item);
            tracker.save();
            std::cout << "Done: " << result.succeeded << " succeeded, "
                      << result.failed << " failed.\n";
            continue;
        }

        std::vector<DownloadItem> items;

        if (input == "/allcards") {
            auto ids = get_all_card_ids();
            for (auto id : ids) items.push_back({id, false});
        } else if (input == "/allfields") {
            auto ids = get_all_field_ids();
            for (auto id : ids) items.push_back({id, true});
        } else if (input.rfind("/deck ", 0) == 0) {
            std::string name = input.substr(6);
            auto path = config.edopro_path / "expansions" / "decks" / (name + ".ydk");
            if (!std::filesystem::exists(path)) {
                std::cerr << "Deck not found: " << path << "\n";
                continue;
            }
            items = parse_deck(path);
        } else if (input.rfind("/ids ", 0) == 0) {
            std::string ids_str = input.substr(5);
            size_t pos = 0;
            while (pos < ids_str.size()) {
                auto comma = ids_str.find(',', pos);
                std::string token = ids_str.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
                // trim
                auto ts = token.find_first_not_of(" \t");
                auto te = token.find_last_not_of(" \t");
                if (ts != std::string::npos) token = token.substr(ts, te - ts + 1);
                else token = "";
                try {
                    uint32_t id = std::stoul(token);
                    items.push_back({id, false});
                } catch (...) {}
                if (comma == std::string::npos) break;
                pos = comma + 1;
            }
        } else {
            // Search by name
            try {
                items = search_cards(input);
            } catch (const std::exception& e) {
                std::cerr << "Card or command not found: " << e.what() << "\n";
                continue;
            }
        }

        if (items.empty()) {
            std::cout << "No cards found.\n";
            continue;
        }

        run_download(config, tracker, items, force);
    }
    std::cout << "Bye!\n";
}

int main(int argc, char** argv) {
    CLI::App app{"HD card image downloader for EDOPro"};

    std::filesystem::path edopro_path = ".";
    bool all_cards = false;
    bool all_fields = false;
    bool force = false;
    bool clean = false;
    size_t concurrency = 100;
    std::optional<std::filesystem::path> deck;
    std::optional<std::string> ids;

    app.add_option("-e,--edopro-path", edopro_path, "Path to EDOPro root folder");
    app.add_flag("-a,--all-cards", all_cards, "Download all cards");
    app.add_flag("--all-fields", all_fields, "Download all field spell images");
    app.add_flag("-f,--force", force, "Force re-download even if already cached");
    app.add_flag("--clean", clean, "Clean re-download: wipe pics folder and download everything");
    app.add_option("-c,--concurrency", concurrency, "Number of concurrent downloads")->default_val(100);
    app.add_option("-d,--deck", deck, "Deck file to download");
    app.add_option("-i,--ids", ids, "Specific card IDs (comma-separated)");

    CLI11_PARSE(app, argc, argv);

    Config config(edopro_path, concurrency);

    if (clean) {
        std::cout << "Wiping " << config.pics_path << " ...\n";
        config.wipe_pics();
        std::cout << "Done. Starting fresh download.\n\n";

        Tracker tracker(config);
        auto items = collect_all();
        std::cout << "Found " << items.size() << " images to download.\n";

        auto result = download_all(config, items);
        for (const auto& item : items) tracker.mark_downloaded(item);
        tracker.save();

        std::cout << "\nDone: " << result.succeeded << " succeeded, "
                  << result.failed << " failed.\n";
        return 0;
    }

    config.setup_dirs();
    Tracker tracker(config);

    if (all_cards || all_fields || deck.has_value() || ids.has_value()) {
        std::vector<DownloadItem> items;

        if (all_cards || all_fields) {
            items = collect_all();
        } else {
            if (deck) {
                auto deck_items = parse_deck(*deck);
                items.insert(items.end(), deck_items.begin(), deck_items.end());
            }
            if (ids) {
                std::string s = *ids;
                size_t pos = 0;
                while (pos < s.size()) {
                    auto comma = s.find(',', pos);
                    std::string token = s.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
                    auto ts = token.find_first_not_of(" \t");
                    auto te = token.find_last_not_of(" \t");
                    if (ts != std::string::npos) token = token.substr(ts, te - ts + 1);
                    else token = "";
                    try {
                        uint32_t id = std::stoul(token);
                        items.push_back({id, false});
                    } catch (...) {}
                    if (comma == std::string::npos) break;
                    pos = comma + 1;
                }
            }
        }

        run_download(config, tracker, items, force);
    } else {
        interactive_mode(config, tracker, force);
    }

    return 0;
}   