#include "downloader.h"
#include <curl/curl.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <fstream>

namespace edopro {

// ID conversion (mirrors Go's idConversionMap)
static uint32_t convert_id(uint32_t card_id) {
    if (card_id == 904186) return 31533705;
    return card_id;
}

static size_t file_write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* file = static_cast<std::ofstream*>(userdata);
    file->write(ptr, size * nmemb);
    return size * nmemb;
}

// Simple counting semaphore
class Semaphore {
    std::mutex mtx_;
    std::condition_variable cv_;
    int count_;
public:
    explicit Semaphore(int count) : count_(count) {}
    void acquire() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this]{ return count_ > 0; });
        count_--;
    }
    void release() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            count_++;
        }
        cv_.notify_one();
    }
};

DownloadResult download_all(const Config& config, const std::vector<DownloadItem>& items) {
    size_t total = items.size();
    std::atomic<size_t> succeeded{0};
    std::atomic<size_t> failed{0};
    std::atomic<size_t> done{0};
    std::mutex progress_mutex;

    Semaphore sem(static_cast<int>(config.concurrency));

    auto worker = [&](const DownloadItem& item) {
        sem.acquire();

        uint32_t converted_id = convert_id(item.id);
        std::string url;
        if (item.is_field) {
            url = "https://images.ygoprodeck.com/images/cards_cropped/" + std::to_string(converted_id) + ".jpg";
        } else {
            url = "https://images.ygoprodeck.com/images/cards/" + std::to_string(converted_id) + ".jpg";
        }

        auto target = config.target_path(item.id, item.is_field);

        bool success = false;
        std::string error_msg;

        CURL* curl = curl_easy_init();
        if (curl) {
            std::ofstream file(target, std::ios::binary);
            if (file) {
                long http_code = 0;
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_USERAGENT, "edopro-hd-cpp/1.0");
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, file_write_callback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

                CURLcode res = curl_easy_perform(curl);
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

                if (res == CURLE_OK && http_code == 200) {
                    success = true;
                } else if (res == CURLE_OK) {
                    error_msg = "HTTP " + std::to_string(http_code);
                    std::filesystem::remove(target); // cleanup partial file
                } else {
                    error_msg = std::string(curl_easy_strerror(res));
                    std::filesystem::remove(target);
                }
            } else {
                error_msg = "Cannot open file for writing";
            }
            curl_easy_cleanup(curl);
        } else {
            error_msg = "Failed to init curl";
        }

        if (success) {
            succeeded.fetch_add(1);
        } else {
            failed.fetch_add(1);
            std::cerr << "Failed " << item.id << ": " << error_msg << "\n";
        }

        size_t n = done.fetch_add(1) + 1;
        if (n % 50 == 0 || n == total) {
            std::lock_guard<std::mutex> lock(progress_mutex);
            std::cout << "\rProgress: " << n << "/" << total
                      << " (" << (n * 100) / total << "%)" << std::flush;
        }

        sem.release();
    };

    // Launch worker threads
    std::vector<std::thread> threads;
    threads.reserve(items.size());
    for (const auto& item : items) {
        threads.emplace_back(worker, item);
    }
    for (auto& t : threads) {
        t.join();
    }

    std::cout << "\n";

    return {succeeded.load(), failed.load()};
}

} // namespace edopro   