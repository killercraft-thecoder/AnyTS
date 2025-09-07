#pragma once

#include "ts.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>

namespace Interpreter
{
    /**
     * Function signature for built-in or user-defined functions.
     * Functions take a vector of TS::Value arguments and return a TS::Value.
     */
    using Function = std::function<TS::Value(const std::vector<TS::Value> &)>;

// Registers a built-in function in ctx.builtins with a given name
// Usage: __BUILTIN("Math.sign") { /* body */ }
#define __BUILTIN(NAME) ctx.builtins[NAME] = [](const std::vector<TS::Value> &args) -> TS::Value

#define __BUILTIN2(NAME) ctx.builtins[NAME] = [&ctx](const std::vector<TS::Value> &args) -> TS::Value

// Quick-eval macro for expressions in the current context , lacks user functions to be used.also ho
#define QEVAL(EXPR) evalSimpleExpression((EXPR), ctx.variables, ctx.builtins)

// any type
#define any std::any

    /**
     * Represents a user-defined function definition.
     */
    struct FunctionDef
    {
        /**
         * Parameter names for the function.
         */
        std::vector<std::string> params;

        /**
         * Declared parameter types (e.g., "number", "string", "any").
         * Matches the order of `params`.
         */
        std::vector<std::string> paramTypes;

        /**
         * The body of the function, stored as a list of source code lines.
         */
        std::vector<std::string> bodyLines;
    };

    /**
     * Holds the current execution context for the interpreter.
     */
    struct Context
    {
        /**
         * Variable environment mapping variable names to values.
         */
        TS::Environment variables;

        /**
         * Map of built-in functions available in the current context.
         */
        std::unordered_map<std::string, Function> builtins;

        /**
         * Map of user-defined functions available in the current context.
         */
        std::unordered_map<std::string, FunctionDef> userFunctions;
    };

    /**
     * Evaluates a simple expression string and returns its value.
     * Supports literals, variables, operators, and function calls.
     *
     * @param expr The expression to evaluate.
     * @param env The variable environment to use for lookups.
     * @param builtins Map of callable built-in functions.
     * @returns The evaluated TS::Value result.
     */
    TS::Value evalSimpleExpression(
        const std::string &expr,
        TS::Environment &env,
        const std::unordered_map<std::string, std::function<TS::Value(const std::vector<TS::Value> &)>> &builtins);

    /**
     * Initializes the interpreter context.
     * Registers built-in functions and prepares the environment.
     *
     * @param ctx The context to initialize.
     */
    void init(Context &ctx);

    /**
     * Executes a single line of code in the given context.
     *
     * @param line The source code line to execute.
     * @param ctx The execution context.
     */
    void executeLine(const std::string &line, Context &ctx);

    /**
     * Executes multiple lines of code (a script) in the given context.
     *
     * @param lines The list of source code lines to execute.
     * @param ctx The execution context.
     */
    void executeScript(const std::vector<std::string> &lines, Context &ctx);

} // namespace Interpreter