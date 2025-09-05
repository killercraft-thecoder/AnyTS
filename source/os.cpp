// os.cpp
#include "../include/os.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;

namespace OS {

    // --- Basic I/O ---
    void print(const std::string& msg) {
        std::cout << msg;
    }

    void printLine(const std::string& msg) {
        std::cout << msg << std::endl;
    }

    inline bool readLine(std::string &out) {
        if (std::getline(std::cin, out)) {
            return true; // successfully read a line
        }
        return false; // EOF or error
    }


    // --- File I/O ---
    inline bool fileExists(const std::string& path) {
        return fs::exists(path);
    }

    bool readFile(const std::string& path, std::string& outData) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return false;
        outData.assign((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
        return true;
    }

    bool writeFile(const std::string& path, const std::string& data) {
        std::ofstream file(path, std::ios::binary);
        if (!file) return false;
        file.write(data.data(), data.size());
        return true;
    }

    bool listFiles(const std::string& directory, std::vector<std::string>& outFiles) {
        try {
            for (const auto& entry : fs::directory_iterator(directory)) {
                outFiles.push_back(entry.path().string());
            }
            return true;
        } catch (...) {
            return false;
        }
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
    inline std::string getPlatformName() {
        std::string name;

        // OS detection
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

        // Architecture detection
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
        return fs::current_path().string();
    }

    bool setWorkingDirectory(const std::string& path) {
        try {
            fs::current_path(path);
            return true;
        } catch (...) {
            return false;
        }
    }

} // namespace OS