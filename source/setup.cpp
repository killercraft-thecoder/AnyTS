#include "setup.h"
#include "interpreter.h"
#include "os.h"
#include <fstream>
#include <sstream>

namespace {
    Interpreter::Context ctx;
}

namespace Setup {

    void initialize() {
        Interpreter::init(ctx);
    }

    bool runFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            OS::printLine("Error: Could not open file: " + filename);
            return false;
        }

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }

        Interpreter::executeScript(lines, ctx);
        return true;
    }

    bool runString(const std::string& code) {
        std::istringstream iss(code);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(iss, line)) {
            lines.push_back(line);
        }

        Interpreter::executeScript(lines, ctx);
        return true;
    }

} // namespace Setup