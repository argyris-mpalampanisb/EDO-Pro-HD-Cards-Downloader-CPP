#pragma once

#include <string>
#include <filesystem>
#include <stdexcept>

namespace edopro {

class Config {
public:
    std::filesystem::path edopro_path;
    std::filesystem::path pics_path;
    std::filesystem::path field_pics_path;
    std::filesystem::path tracker_file;
    size_t concurrency;

    Config(std::filesystem::path edopro_path, size_t concurrency);

    void setup_dirs() const;
    void wipe_pics() const;
    std::filesystem::path target_path(uint32_t id, bool is_field) const;
};

} // namespace edopro   