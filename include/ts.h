// ts.h
#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <optional>

namespace TS
{

    struct TypeError
    {
        size_t line;
        std::string message;
    };

    // --- Supported Value Types ---
    enum class ValueType
    {
        Number,
        String,
        Boolean,
        Null,
        Undefined,
        NaN,
    };

    // --- Value Representation ---
    struct Value
    {
        ValueType type;
        std::variant<double, std::string, bool> data;

        Value(); // Null by default
        Value(double num);
        Value(const std::string &str);
        Value(bool b);

        std::string toString() const;
        double toNumber() const;
        bool toBool() const;
    };

    // --- Variable Environment ---
    using Environment = std::unordered_map<std::string, Value>;

    // --- Environment Helpers ---
    bool setVar(Environment &env, const std::string &name, const Value &value);
    std::optional<Value> getVar(const Environment &env, const std::string &name);
    bool varExists(const Environment &env, const std::string &name);
    std::vector<TypeError> checkTypesInSource(const std::string &source);

} // namespace TS