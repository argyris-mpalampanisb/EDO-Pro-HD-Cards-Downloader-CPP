#include "config.h"
#include <system_error>

namespace edopro {

Config::Config(std::filesystem::path edopro_path, size_t concurrency)
    : edopro_path(std::move(edopro_path)),
      concurrency(concurrency)
{
    pics_path = edopro_path / "expansions" / "pics";
    field_pics_path = pics_path / "field";
    tracker_file = edopro_path / "expansions" / ".hd_tracker.json";
}

void Config::setup_dirs() const {
    std::filesystem::create_directories(pics_path);
    std::filesystem::create_directories(field_pics_path);
}

void Config::wipe_pics() const {
    if (std::filesystem::exists(pics_path)) {
        std::filesystem::remove_all(pics_path);
    }
    std::filesystem::create_directories(field_pics_path);
}

std::filesystem::path Config::target_path(uint32_t id, bool is_field) const {
    const auto& base = is_field ? field_pics_path : pics_path;
    return base / (std::to_string(id) + ".jpg");
}

} // namespace edopro   