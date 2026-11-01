**Because if you already have a Repo that does it with Rust, why not also do it with CPP? Well, this is actually a very interesting take!**

**So, this is a bit more straight forward, I believe. If you are on Windows, make sure you have MSYS2 installed with the UCRT64 Bash shell and git clone this repo.**

**In UCRT4 install: pacman -S mingw-w64-ucrt-x86_64-curl**

**After that, run: g++ -std=c++17 -O2 -I. -Iinclude -Ithird_party src/main.cpp src/api.cpp src/config.cpp src/downloader.cpp src/tracker.cpp  -o EDOProHdDownloader.exe -lcurl -lpthread**

**The output should be in the root folder. When you run the program, you will see an extensions folder made automatically. Select which option you wish to use and let the program do its thing!**

**Feel free to modify and tweak the app to your liking! This app is free to use and should be able to pick up new cards, as they get released (my Rust version is a bit more optimized on some aspects)**

**If you have any questions/issues, feel free to let me know in the Issues tab!**
