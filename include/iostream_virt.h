#pragma once
#include "os.h"
#include <string>
#include <type_traits>

namespace std
{

    class ostream
    {
    public:
        // Generic template for most types
        template <typename T>
        ostream &operator<<(const T &value)
        {
            OS::print(toString(value));
            return *this;
        }

        // Overload for manipulators like endl
        using Manipulator = ostream &(*)(ostream &);
        ostream &operator<<(Manipulator manip)
        {
            return manip(*this);
        }

    private:
        // Convert to string for printing
        template <typename T>
        static std::string toString(const T &value)
        {
            if constexpr (std::is_same_v<T, std::string>)
            {
                return value;
            }
            else if constexpr (std::is_same_v<T, const char *>)
            {
                return std::string(value);
            }
            else if constexpr (std::is_arithmetic_v<T>)
            {
                return std::to_string(value);
            }
            else
            {
                // Fallback: try to_string if available
                return "[unsupported type]";
            }
        }
    };

    // Simple endl manipulator
    inline ostream &endl(ostream &os)
    {
        OS::printLine("");
        return os;
    }

    // Global instance like std::cout
    extern ostream cout;

    // -------- Input Stream --------
    class istream
    {
    public:
        template <typename T>
        istream &operator>>(T &value)
        {
            std::string line = OS::readLine();
            if constexpr (std::is_same_v<T, std::string>)
            {
                value = line;
            }
            else if constexpr (std::is_arithmetic_v<T>)
            {
                if (!line.empty())
                    value = static_cast<T>(std::stod(line));
            }
            return *this;
        }
    };

    extern istream cin;

    // -------- getline equivalent --------
    inline bool getline(istream &in, std::string &out)
    {
        return OS::readLine(out);
    }

} // namespace virtio