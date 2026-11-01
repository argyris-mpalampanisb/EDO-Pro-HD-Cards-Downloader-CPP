# 1. Get dependencies (header-only, just download the single .hpp files)
mkdir -p third_party/nlohmann
wget -O third_party/nlohmann/json.hpp \
  https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
wget -O third_party/CLI11.hpp \
  https://github.com/CLIUtils/CLI11/releases/download/v2.4.2/CLI11.hpp

# 2. Install libcurl dev headers (if not present)
#    Ubuntu/Debian: sudo apt install libcurl4-openssl-dev
#    macOS:         brew install curl
#    Windows:       use vcpkg or prebuilt from curl.se

# 3. Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 4. Run
./edopro-hd --help
./edopro-hd -e /path/to/edopro --all-cards
./edopro-hd -e /path/to/edopro -d mydeck
./edopro-hd -e /path/to/edopro          # interactive mode   