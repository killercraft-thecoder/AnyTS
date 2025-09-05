// ts.cpp
#include "ts.h"
#include <sstream>

namespace TS
{

    // --- Value Constructors ---
    Value::Value() : type(ValueType::Null), data(false) {}
    Value::Value(double num) : type(ValueType::Number), data(num) {}
    Value::Value(const std::string &str) : type(ValueType::String), data(str) {}
    Value::Value(bool b) : type(ValueType::Boolean), data(b) {}

    // --- Conversions ---
    std::string Value::toString() const
    {
        switch (type)
        {
        case ValueType::Number:
        {
            std::ostringstream oss;
            oss << std::get<double>(data);
            return oss.str();
        }
        case ValueType::String:
            return std::get<std::string>(data);
        case ValueType::Boolean:
            return std::get<bool>(data) ? "true" : "false";
        case ValueType::NaN:
            return "NaN";

        case ValueType::Undefined:
            return "undefined";

        case ValueType::Null:
        default:
            return "null";
        }
    }

    double Value::toNumber() const
    {
        auto fastParseDouble = [](const std::string &s, bool &ok) -> double
        {
            const char *str = s.c_str();
            char *endptr = nullptr;
            ok = false;

            // Skip leading whitespace
            while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')
            {
                ++str;
            }

            if (*str == '\0')
                return 0.0;

            errno = 0;
            double val = std::strtod(str, &endptr);

            if (endptr != str && errno == 0)
            {
                ok = true;
                return val;
            }
            return 0.0;
        };

        switch (type)
        {
        case ValueType::Number:
            return std::get<double>(data);

        case ValueType::String:
        {
            const std::string &strVal = std::get<std::string>(data);
            bool ok = false;
            double parsed = fastParseDouble(strVal, ok);
            if (ok)
                return parsed;

            // Fallback to std::stod if needed
            try
            {
                return std::stod(strVal);
            }
            catch (...)
            {
                return 0.0;
            }
        }

        case ValueType::Boolean:
            return std::get<bool>(data) ? 1.0 : 0.0;

        case ValueType::Null:
        default:
            return 0.0;
        }
    }

    bool Value::toBool() const
    {
        switch (type)
        {
        case ValueType::Boolean:
            return std::get<bool>(data);
        case ValueType::Number:
            return std::get<double>(data) != 0.0;
        case ValueType::String:
            return !std::get<std::string>(data).empty();
        case ValueType::Null:
        default:
            return false;
        }
    }

    // --- Environment Helpers ---
    bool setVar(Environment &env, const std::string &name, const Value &value)
    {
        env[name] = value;
        return true;
    }

    std::optional<Value> getVar(const Environment &env, const std::string &name)
    {
        auto it = env.find(name);
        if (it != env.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    bool varExists(const Environment &env, const std::string &name)
    {
        return env.find(name) != env.end();
    }

} // namespace TS