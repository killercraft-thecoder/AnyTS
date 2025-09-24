#pragma once
#include "os.h"
#include <string>
#include <type_traits>

namespace std
{

    class ostream_2
    {
    public:
        // Generic template for most types
        template <typename T>
        ostream_2 &operator<<(const T &value)
        {
            OS::print(toString(value));
            return *this;
        }

        // Overload for manipulators like endl
        using Manipulator = ostream_2 &(*)(ostream_2 &);
        ostream_2 &operator<<(Manipulator manip)
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
    inline ostream_2 &endl(ostream_2 &os)
    {
        OS::printLine("");
        return os;
    }

    // Global instance like std::cout
    extern ostream_2 cout;

    // -------- Input Stream --------
    class istream_2
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

    extern istream_2 cin;

    // -------- getline equivalent --------
    inline bool getline(istream_2 &in, std::string &out)
    {
        return OS::readLine(out);
    }

} // namespace virtio