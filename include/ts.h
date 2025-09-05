// ts.h
#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <optional>

namespace TS
{

    /** 
     * @struct
     * @short Type Error
    */
    struct TypeError
    {
        size_t line; // Line Error Occured on
        std::string message;  // Error Message
    };

    /** 
     * @struct
     * @short Runtime Error
    */
    struct RuntimeError {
        size_t line; // Line Error Occured on
        std::string file; // File Error Happened in.
        std::string message; // Error Message
    };

    /** 
     * @struct
     * @short Error
    */
    struct Error {
        size_t line; // Line Error Occured on
        std::string message;  // Error Message
    };

    // --- Supported Value Types ---
    enum class ValueType
    {
        Number, // Number (double)
        String, // String (std::string)
        Boolean, // boolean (bool)
        Null, // NULL
        Undefined, // Undefined
        NaN, // NaN
    };

    // --- Value Representation ---
    struct Value
    {
        ValueType type; // Current Type
        std::variant<double, std::string, bool> data; // Current Value
        
        Value(); // Null by default
        Value(double num); // Create a TS::Value with a number.
        Value(const std::string &str); // Create a TS::Value with a string.
        Value(bool b); // Create a TS::Value with a boolean.

        std::string toString() const;
        double toNumber() const;
        bool toBool() const;
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