#pragma once

#include <vector>
#include <cstdint>
#include "api.h"
#include "config.h"

namespace edopro {

struct DownloadResult {
    size_t succeeded;
    size_t failed;
};

// Download all items concurrently
DownloadResult download_all(const Config& config, const std::vector<DownloadItem>& items);

} // namespace edopro   