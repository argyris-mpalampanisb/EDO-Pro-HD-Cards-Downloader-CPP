cmake_minimum_required(VERSION 3.16)
project(edopro-hd-cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Header-only deps: place in third_party/
#   - nlohmann/json.hpp  (https://github.com/nlohmann/json/releases)
#   - CLI11.hpp          (https://github.com/CLIUtils/CLI11/releases)

add_executable(edopro-hd
    src/main.cpp
    src/api.cpp
    src/config.cpp
    src/downloader.cpp
    src/tracker.cpp
)

target_include_directories(edopro-hd PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(edopro-hd PRIVATE
    CURL::libcurl
    Threads::Threads
)   