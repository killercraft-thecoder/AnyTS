#pragma once
#include <string>

namespace Setup {

    // Initialize the runtime (register builtins, etc.)
    void initialize();

    // Run a script from a file path
    bool runFile(const std::string& filename);

    // Run a script from a string (optional helper)
    bool runString(const std::string& code);

} // namespace Setup