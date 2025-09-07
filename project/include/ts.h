// ts.h
#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <optional>
#include <cmath>
#include <cstdint>

namespace TS
{

    /**
     * @struct
     * @short Type Error
     */
    struct TypeError
    {
        size_t line;         // Line Error Occured on
        std::string message; // Error Message
    };

    /**
     * @struct
     * @short Runtime Error
     */
    struct RuntimeError
    {
        size_t line;         // Line Error Occured on
        std::string file;    // File Error Happened in.
        std::string message; // Error Message
    };

    /**
     * @struct
     * @short Error
     */
    struct Error
    {
        size_t line;         // Line Error Occured on
        std::string message; // Error Message
    };

    // --- Supported Value Types ---
    enum class ValueType
    {
        Number,    // Number (double)
        String,    // String (std::string)
        Boolean,   // boolean (bool)
        Null,      // NULL
        Undefined, // Undefined
        NaN,       // NaN
        Half       // half (fp16)
    };
    
    #ifdef ADD_STD_HALF
    #define _half half
    #else
    #define _half float
    #endif

    // --- Value Representation ---
    struct Value
    {
        ValueType type;                               // Current Type
        std::variant<double, std::string, bool,_half> data; // Current Value

        Value();                       // Null by default
        explicit Value(double num);             // Create a TS::Value with a number.
        explicit Value(const std::string &str); // Create a TS::Value with a string.
        explicit Value(bool b);                 // Create a TS::Value with a boolean.
        explicit Value(_half b);                 // Create a TS::Value with a half.

        inline size_t size() const
        {
            size_t total = sizeof(*this); // shallow size (tag + variant)

            switch (type)
            {
            case ValueType::String:
            {
                const auto &s = std::get<std::string>(data);

                // Detect if string is using heap storage
                // Detect SSO (Short String Optimization) threshold based on compiler/STL
#if defined(_MSC_VER)
                // MSVC's std::string typically uses 22-byte SSO on 64-bit
                constexpr size_t SSO_THRESHOLD = 22;

#elif defined(__GLIBCXX__)
                // libstdc++ (used by GCC) typically uses 15-byte SSO on 64-bit
                constexpr size_t SSO_THRESHOLD = 15;

#elif defined(_LIBCPP_VERSION)
                // libc++ (used by Clang) also tends to use 15-byte SSO
                constexpr size_t SSO_THRESHOLD = 15;

#else
                // Unknown compiler/STL — fallback to conservative estimate
                constexpr size_t SSO_THRESHOLD = 20;
#endif

                if (s.size() > SSO_THRESHOLD)
                {
                    // capacity() is the allocated heap buffer size
                    total += s.capacity() + 1; // +1 for null terminator
                }
                break;
            }

            case ValueType::Number:
            case ValueType::Boolean:
            case ValueType::Null:
            case ValueType::Undefined:
            case ValueType::NaN:
                // No extra heap allocation for these
                break;
            }
            return total;
        }

        std::string toString() const;
        double toNumber() const;
        bool toBool() const;
        bool isTruthy() const;

        // Implicit conversion to bool (safe)
        operator bool() const { return toBool(); }

        // Explicit conversion to double
        explicit operator double() const { return toNumber(); }

        explicit operator float() const { return static_cast<float>(toNumber()); }

        explicit operator int32_t() const { return static_cast<int32_t>(std::round(toNumber())); }

        explicit operator int64_t() const { return static_cast<int64_t>(std::round(toNumber())); }

        explicit operator int16_t() const { return static_cast<int16_t>(std::round(toNumber())); }

        // Explicit conversion to std::string
        explicit operator std::string() const
        {
            return toString();
        }
    };

    // --- Variable Environment ---
    using Environment = std::unordered_map<std::string, Value>;

    // --- Environment Helpers ---
    /**
     * @fn
     * @short Set Varible.
     */
    bool setVar(Environment &env, const std::string &name, const Value &value);
    /**
     * @fn
     * @short Get Varible.
     */
    std::optional<Value> getVar(const Environment &env, const std::string &name);
    /**
     * @fn
     * @short Var Exists?
     * @returns wheter the var exists in the current enviroment.
     */
    bool varExists(const Environment &env, const std::string &name);
    /**
     * @fn
     * @short Check For type Errors.
     */
    std::vector<TypeError> checkTypesInSource(const std::string &source);

} // namespace TS