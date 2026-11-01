#include "tracker.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

namespace edopro {

Tracker::Tracker(const Config& config) : path_(config.tracker_file) {
    if (std::filesystem::exists(path_)) {
        try {
            std::ifstream file(path_);
            json j = json::parse(file);
            if (j.contains("cards")) {
                for (const auto& v : j["cards"]) cards_.insert(v.get<uint32_t>());
            }
            if (j.contains("fields")) {
                for (const auto& v : j["fields"]) fields_.insert(v.get<uint32_t>());
            }
        } catch (...) {
            // Start fresh if corrupt
        }
    }
}

bool Tracker::is_downloaded(const DownloadItem& item) const {
    if (item.is_field) return fields_.count(item.id) > 0;
    return cards_.count(item.id) > 0;
}

void Tracker::mark_downloaded(const DownloadItem& item) {
    if (item.is_field) fields_.insert(item.id);
    else cards_.insert(item.id);
}

void Tracker::save() const {
    json j;
    j["cards"] = json::array();
    for (auto id : cards_) j["cards"].push_back(id);
    j["fields"] = json::array();
    for (auto id : fields_) j["fields"].push_back(id);

    std::ofstream file(path_);
    file << j.dump(2) << "\n";
}

} // namespace edopro   