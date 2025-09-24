// ts.cpp
#include "ts.h"
#include "os.h"
#include <sstream>
#include <map>
#include <string>
#include <vector>

namespace TS
{

    // --- Value Constructors ---
    Value::Value() : type(ValueType::Null), data(false) {}
    Value::Value(NUMBER num) : type(ValueType::Number), data(num) {}
    Value::Value(const std::string &str) : type(ValueType::String), data(str) {}
    Value::Value(bool b) : type(ValueType::Boolean), data(b) {}
    Value::Value(_half b) : type(ValueType::Half), data(b) {}

    // --- Conversions ---
    std::string Value::toString() const
    {
        switch (type)
        {
        case ValueType::Number:
        {
            std::ostringstream oss;
            oss << std::get<NUMBER>(data);
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

#ifdef ADD_STD_HALF
        case ValueType::Half:
        {
            std::ostringstream oss;
            _half h = std::get<_half>(data); // extract the half value
            oss << static_cast<float>(h);    // convert to float and stream it
            return oss.str();
        }
#endif

        case ValueType::Null:
        default:
            return "null";
        }
    }

    NUMBER Value::toNumber() const
    {
        auto fastParseNUMBER = [](const std::string &s, bool &ok) -> NUMBER
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
            NUMBER val = std::strtod(str, &endptr);

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
            return std::get<NUMBER>(data);

        case ValueType::String:
        {
            const std::string &strVal = std::get<std::string>(data);
            bool ok = false;
            NUMBER parsed = fastParseNUMBER(strVal, ok);
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

#ifdef ADD_STD_HALF
        case ValueType::Half:
            return static_cast<NUMBER>(static_cast<float>(std::get<_half>(data)));
#endif
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
            return std::get<NUMBER>(data) != 0.0;
        case ValueType::String:
            return !std::get<std::string>(data).empty();
#ifdef ADD_STD_HALF
        case ValueType::Half:
            return std::get<_half>(data) != 0.0;
#endif
        case ValueType::Null:
        default:
            return false;
        }
    }
    inline bool Value::isTruthy() const
    {
        if (this->type != ValueType::Boolean)
        {
            // Raise Runtime Error
            OS::printLine("Runtime Error:caannot convert to bool.");
            return false;
        }
        return this->toBool() == true;
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
        return (it != env.end()) ? std::optional<Value>(it->second) : std::nullopt;
    }

    bool varExists(const Environment &env, const std::string &name)
    {
        return env.find(name) != env.end();
    }

    static inline void trim(std::string &s)
    {
        size_t start = 0;
        size_t end = s.size();

        // Skip leading whitespace
        while (start < end && std::isspace(static_cast<unsigned char>(s[start])))
            ++start;

        // Skip trailing whitespace
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
            --end;

        if (start > 0 || end < s.size())
            s.erase(end, s.size() - end), s.erase(0, start);
    }

#ifndef SKIP_TYPECHECK
    std::vector<TypeError> checkTypesInSource(const std::string &source)
    {
        std::vector<TypeError> errors;
        std::map<std::string, std::vector<std::string>> funcParamTypes;

        // Split into lines
        std::vector<std::string> lines;
        {
            size_t start = 0, end;
            while ((end = source.find('\n', start)) != std::string::npos)
            {
                lines.push_back(source.substr(start, end - start));
                start = end + 1;
            }
            lines.push_back(source.substr(start));
        }

        // Pass 1: collect function definitions
        for (size_t i = 0; i < lines.size(); ++i)
        {
            std::string line = lines[i];
            trim(line);
            if (line.rfind("function ", 0) == 0)
            {
                size_t nameStart = 9;
                size_t nameEnd = nameStart;
                while (nameEnd < line.size() && (isalnum((unsigned char)line[nameEnd]) || line[nameEnd] == '_'))
                    nameEnd++;
                std::string funcName = line.substr(nameStart, nameEnd - nameStart);

                size_t paramStart = line.find('(', nameEnd);
                size_t paramEnd = line.find(')', paramStart);
                if (paramStart != std::string::npos && paramEnd != std::string::npos)
                {
                    std::string params = line.substr(paramStart + 1, paramEnd - paramStart - 1);
                    std::vector<std::string> types;
                    size_t pos = 0;
                    while (pos < params.size())
                    {
                        // skip spaces
                        while (pos < params.size() && isspace((unsigned char)params[pos]))
                            pos++;
                        // skip param name
                        while (pos < params.size() && (isalnum((unsigned char)params[pos]) || params[pos] == '_'))
                            pos++;
                        // skip spaces
                        while (pos < params.size() && isspace((unsigned char)params[pos]))
                            pos++;
                        if (pos < params.size() && params[pos] == ':')
                        {
                            pos++;
                            while (pos < params.size() && isspace((unsigned char)params[pos]))
                                pos++;
                            size_t typeStart = pos;
                            while (pos < params.size() && (isalnum((unsigned char)params[pos]) || params[pos] == '_'))
                                pos++;
                            types.push_back(params.substr(typeStart, pos - typeStart));
                        }
                        // skip to next param
                        while (pos < params.size() && params[pos] != ',')
                            pos++;
                        if (pos < params.size() && params[pos] == ',')
                            pos++;
                    }
                    funcParamTypes[funcName] = types;
                }
            }
        }

        // Pass 2: check calls
        for (size_t i = 0; i < lines.size(); ++i)
        {
            std::string line = lines[i];
            trim(line);
            // find function name
            for (auto &entry : funcParamTypes)
            {
                const std::string &funcName = entry.first;
                size_t callPos = line.find(funcName + "(");
                if (callPos != std::string::npos)
                {
                    size_t argStart = callPos + funcName.size() + 1;
                    size_t argEnd = line.find(')', argStart);
                    if (argEnd != std::string::npos)
                    {
                        std::string args = line.substr(argStart, argEnd - argStart);
                        std::vector<std::string> argList;
                        size_t pos = 0;
                        while (pos < args.size())
                        {
                            size_t start = pos;
                            while (pos < args.size() && args[pos] != ',')
                                pos++;
                            std::string arg = args.substr(start, pos - start);
                            trim(arg);
                            argList.push_back(arg);
                            if (pos < args.size() && args[pos] == ',')
                                pos++;
                        }
                        // Compare types
                        auto &expectedTypes = entry.second;
                        for (size_t ai = 0; ai < argList.size() && ai < expectedTypes.size(); ++ai)
                        {
                            const std::string &expected = expectedTypes[ai];
                            const std::string &arg = argList[ai];
                            bool isString = arg.size() >= 2 && ((arg.front() == '"' && arg.back() == '"') || (arg.front() == '\'' && arg.back() == '\''));
                            bool isNumber = !arg.empty() && (isdigit((unsigned char)arg[0]) || arg[0] == '.');
                            bool isBool = (arg == "true" || arg == "false");

                            if (expected == "number" && !isNumber)
                            {
                                errors.push_back({i + 1, "Argument " + std::to_string(ai + 1) + " to " + funcName + " should be a number"});
                            }
                            if (expected == "string" && !isString)
                            {
                                errors.push_back({i + 1, "Argument " + std::to_string(ai + 1) + " to " + funcName + " should be a string"});
                            }
                            if (expected == "boolean" && !isBool)
                            {
                                errors.push_back({i + 1, "Argument " + std::to_string(ai + 1) + " to " + funcName + " should be a boolean"});
                            }
                        }
                    }
                }
            }
        }

        return errors;
    }
#endif

} // namespace TS