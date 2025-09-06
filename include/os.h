#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace OS {

    // --- Basic I/O ---

    /**
     * Prints a message without a newline.
     * @param msg The string to print.
     */
    void print(const std::string& msg);

    /**
     * Prints a message followed by a newline.
     * @param msg The string to print.
     */
    void printLine(const std::string& msg);

    /**
     * Reads a line of input from the standard input stream.
     * @param out Reference to a string where the input will be stored.
     * @returns True if a line was successfully read, false on EOF or error.
     */
    bool readLine(std::string &out);

    // --- File I/O ---

    /**
     * Checks if a file exists at the given path.
     * @param path Path to the file.
     * @returns True if the file exists, false otherwise.
     */
    bool fileExists(const std::string& path);

    /**
     * Reads the entire contents of a file into a string.
     * @param path Path to the file.
     * @param outData Reference to a string where file contents will be stored.
     * @returns True if the file was read successfully, false otherwise.
     */
    bool readFile(const std::string& path, std::string& outData);

    /**
     * Writes a string to a file, overwriting if it exists.
     * @param path Path to the file.
     * @param data Data to write.
     * @returns True if the file was written successfully, false otherwise.
     */
    bool writeFile(const std::string& path, const std::string& data);

    /**
     * Lists all files in a directory.
     * @param directory Path to the directory.
     * @param outFiles Reference to a vector where file paths will be stored.
     * @returns True if the directory was read successfully, false otherwise.
     */
    bool listFiles(const std::string& directory, std::vector<std::string>& outFiles);

    // --- Timing ---

    /**
     * Gets the number of milliseconds since the program started.
     * @returns Milliseconds since program start.
     * @note Not All Platforms May Support this.
     */
    uint64_t getMillis();

    /**
     * Suspends execution for a given number of milliseconds.
     * @param ms Number of milliseconds to sleep.
     */
    void sleepMillis(uint64_t ms);

    // --- System Info ---

    /**
     * Gets the name of the current platform/OS.
     * @returns Platform name as a string (e.g., "Windows", "Linux", "macOS").
     */
    std::string getPlatformName();

    /**
     * Gets the current working directory.
     * @returns The current working directory path.
     * @note Not All Platforms May Support this.
     */
    std::string getWorkingDirectory();

    /**
     * Sets the current working directory.
     * @param path Path to set as the working directory.
     * @returns True if the working directory was changed successfully, false otherwise.
     * @note Not All Platforms May Support this.
     */
    bool setWorkingDirectory(const std::string& path);

} // namespace OS