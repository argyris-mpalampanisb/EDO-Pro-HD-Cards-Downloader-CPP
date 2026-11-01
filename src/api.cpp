#include "api.h"
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace edopro {

static const char* API_BASE = "https://db.ygoprodeck.com/api/v7/cardinfo.php";

// --- libcurl helpers ---

static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* str = static_cast<std::string*>(userdata);
    str->append(ptr, size * nmemb);
    return size * nmemb;
}

static std::string http_get(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Failed to init curl");

    std::string response;
    std::string user_agent = "edopro-hd-cpp/1.0";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        throw std::runtime_error(std::string("CURL error: ") + curl_easy_strerror(res));
    if (http_code != 200)
        throw std::runtime_error("HTTP " + std::to_string(http_code));

    return response;
}

static std::string url_encode(const std::string& str) {
    std::ostringstream escaped;
    for (unsigned char c : str) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::hex << std::uppercase
                    << (c >> 4) << (c & 0xF) << std::dec;
        }
    }
    return escaped.str();
}

// --- Extract image IDs from API response ---

static std::vector<uint32_t> extract_ids(const std::string& response) {
    auto j = json::parse(response);
    std::vector<uint32_t> ids;
    if (j.contains("data")) {
        for (const auto& card : j["data"]) {
            if (card.contains("card_images")) {
                for (const auto& img : card["card_images"]) {
                    uint32_t id = img.value("id", 0);
                    if (id != 0) ids.push_back(id);
                }
            }
        }
    }
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

// --- Public API ---

std::vector<uint32_t> get_all_card_ids() {
    std::string response = http_get(API_BASE);
    return extract_ids(response);
}

std::vector<uint32_t> get_all_field_ids() {
    std::string url = std::string(API_BASE) + "?type=spell card&race=field";
    std::string response = http_get(url);
    return extract_ids(response);
}

std::vector<DownloadItem> search_cards(const std::string& name) {
    std::string url = std::string(API_BASE) + "?fname=" + url_encode(name);
    std::string response = http_get(url);
    auto ids = extract_ids(response);
    std::vector<DownloadItem> items;
    items.reserve(ids.size());
    for (auto id : ids) {
        items.push_back({id, false});
    }
    return items;
}

std::vector<DownloadItem> parse_deck(const fs::path& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Failed to read deck file: " + path.string());

    std::vector<DownloadItem> items;
    std::string line;
    while (std::getline(file, line)) {
        // Trim
        auto start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        auto end = line.find_last_not_of(" \t\r\n");
        line = line.substr(start, end - start + 1);

        if (line.empty() || line[0] == '#') continue;

        // First whitespace-delimited token
        auto space_pos = line.find_first_of(" \t");
        std::string id_str = (space_pos != std::string::npos)
            ? line.substr(0, space_pos) : line;

        try {
            uint32_t id = std::stoul(id_str);
            if (id != 0) {
                items.push_back({id, false});
            }
        } catch (...) {}
    }
    return items;
}

} // namespace edopro   