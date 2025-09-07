#include "../include/os.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <vector>

// Detect filesystem support
#if __has_include(<filesystem>)
    #include <filesystem>
    namespace fs = std::filesystem;
    #define ANYTS_HAS_FS 1
#else
    #define ANYTS_HAS_FS 0
#endif

// Detect fstream support
#if __has_include(<fstream>)
    #include <fstream>
    #define ANYTS_HAS_FSTREAM 1
#else
    #define ANYTS_HAS_FSTREAM 0
#endif

// Fallback includes for file ops
#include <cstdio>
#include <sys/stat.h>

#if defined(_WIN32)
    #include <windows.h>
    #include <direct.h>
#else
    #include <dirent.h>
    #include <unistd.h>
#endif

namespace OS {

    // --- Basic I/O ---
    void print(const std::string& msg) {
        std::cout << msg;
    }

    void printLine(const std::string& msg) {
        std::cout << msg << std::endl;
    }

    bool readLine(std::string &out) {
        return static_cast<bool>(std::getline(std::cin, out));
    }

    // --- File I/O ---
    bool fileExists(const std::string& path) {
    #if ANYTS_HAS_FS
        return fs::exists(path);
    #else
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0);
    #endif
    }

    bool readFile(const std::string& path, std::string& outData) {
    #if ANYTS_HAS_FSTREAM
        std::ifstream file(path, std::ios::binary);
        if (!file) return false;
        outData.assign((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
        return true;
    #else
        FILE* f = std::fopen(path.c_str(), "rb");
        if (!f) return false;
        std::fseek(f, 0, SEEK_END);
        long size = std::ftell(f);
        std::rewind(f);
        outData.resize(size);
        std::fread(&outData[0], 1, size, f);
        std::fclose(f);
        return true;
    #endif
    }

    bool writeFile(const std::string& path, const std::string& data) {
    #if ANYTS_HAS_FSTREAM
        std::ofstream file(path, std::ios::binary);
        if (!file) return false;
        file.write(data.data(), data.size());
        return true;
    #else
        FILE* f = std::fopen(path.c_str(), "wb");
        if (!f) return false;
        std::fwrite(data.data(), 1, data.size(), f);
        std::fclose(f);
        return true;
    #endif
    }

    bool listFiles(const std::string& directory, std::vector<std::string>& outFiles) {
    #if ANYTS_HAS_FS
        try {
            for (const auto& entry : fs::directory_iterator(directory)) {
                outFiles.push_back(entry.path().string());
            }
            return true;
        } catch (...) {
            return false;
        }
    #else
    #if defined(_WIN32)
        WIN32_FIND_DATA findFileData;
        HANDLE hFind = FindFirstFile((directory + "\\*").c_str(), &findFileData);
        if (hFind == INVALID_HANDLE_VALUE) return false;
        do {
            outFiles.push_back(findFileData.cFileName);
        } while (FindNextFile(hFind, &findFileData) != 0);
        FindClose(hFind);
        return true;
    #else
        DIR* dir = opendir(directory.c_str());
        if (!dir) return false;
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            outFiles.push_back(entry->d_name);
        }
        closedir(dir);
        return true;
    #endif
    #endif
    }

    // --- Timing ---
    uint64_t getMillis() {
        using namespace std::chrono;
        static auto start = steady_clock::now();
        auto now = steady_clock::now();
        return duration_cast<milliseconds>(now - start).count();
    }

    void sleepMillis(uint64_t ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    // --- System Info ---
    std::string getPlatformName() {
        std::string name;
        #if defined(_WIN32)
            name = "Windows";
        #elif defined(__APPLE__)
            #include "TargetConditionals.h"
            #if TARGET_OS_IPHONE
                name = "iOS";
            #else
                name = "macOS";
            #endif
        #elif defined(__ANDROID__)
            name = "Android";
        #elif defined(__linux__)
            name = "Linux";
        #elif defined(__FreeBSD__)
            name = "FreeBSD";
        #elif defined(__unix__)
            name = "Unix";
        #else
            name = "UnknownOS";
        #endif

        #if defined(__x86_64__) || defined(_M_X64)
            name += " x86_64";
        #elif defined(__i386__) || defined(_M_IX86)
            name += " x86";
        #elif defined(__aarch64__) || defined(_M_ARM64)
            name += " ARM64";
        #elif defined(__arm__) || defined(_M_ARM)
            name += " ARM";
        #elif defined(__riscv)
            name += " RISC-V";
        #endif

        return name;
    }

    std::string getWorkingDirectory() {
    #if ANYTS_HAS_FS
        return fs::current_path().string();
    #elif defined(_WIN32)
        char buffer[MAX_PATH];
        _getcwd(buffer, MAX_PATH);
        return std::string(buffer);
    #else
        char buffer[PATH_MAX];
        getcwd(buffer, sizeof(buffer));
        return std::string(buffer);
    #endif
    }

    bool setWorkingDirectory(const std::string& path) {
    #if ANYTS_HAS_FS
        try {
            fs::current_path(path);
            return true;
        } catch (...) {
            return false;
        }
    #elif defined(_WIN32)
        return _chdir(path.c_str()) == 0;
    #else
        return chdir(path.c_str()) == 0;
    #endif
    }

} // namespace OS