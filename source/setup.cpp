#include "setup.h"
#include "interpreter.h"
#include "os.h"
#include "ts.h"
#include "iostream_virt.h"
#include <fstream>
#include <sstream>

namespace
{
    Interpreter::Context ctx;
}

namespace std
{
    std::string joinStrings(const std::vector<std::string> &parts, const std::string &sep = "")
    {
        std::string result;
        for (size_t i = 0; i < parts.size(); ++i)
        {
            result += parts[i];
            if (i + 1 < parts.size())
                result += sep;
        }
        return result;
    }
}

namespace Setup
{

    void initialize()
    {
        Interpreter::init(ctx);
    }

    bool runFile(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file)
        {
            OS::printLine("Error: Could not open file: " + filename);
            return false;
        }

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line))
        {
            lines.push_back(line);
        }

        // Run the pre-execution type check
        auto errors = TS::checkTypesInSource(std::joinStrings(lines));

        if (!errors.empty())
        {
            for (auto &err : errors)
            {
                std::cout << "Line " << err.line << ": " << err.message << "\n";
                
            }
            return false;
        }
        Interpreter::executeScript(lines, ctx);
        return true;
    }

    bool runString(const std::string &code)
    {
        std::istringstream iss(code);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(iss, line))
        {
            lines.push_back(line);
        }

        Interpreter::executeScript(lines, ctx);
        return true;
    }

} // namespace Setup