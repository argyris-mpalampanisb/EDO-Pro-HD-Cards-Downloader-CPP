#pragma once

#include <cstdint>
#include <unordered_set>
#include <filesystem>
#include "api.h"
#include "config.h"

namespace edopro {

class Tracker {
public:
    explicit Tracker(const Config& config);

    bool is_downloaded(const DownloadItem& item) const;
    void mark_downloaded(const DownloadItem& item);
    void save() const;

private:
    std::unordered_set<uint32_t> cards_;
    std::unordered_set<uint32_t> fields_;
    std::filesystem::path path_;
};

} // namespace edopro   