// interpreter.cpp
#include "interpreter.h"
#include "os.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <cctype>
#include <stack>
#include <queue>
#include "iostream_virt.h"

namespace Interpreter
{

    // Main evaluator: takes expression string + environment
    TS::Value evalSimpleExpression(
        const std::string &expr,
        TS::Environment &env,
        const std::unordered_map<std::string,
                                 std::function<TS::Value(const std::vector<TS::Value> &)>> &builtins)
    {
        // --- Tokenizer ---
        auto tokenize = [](const std::string &s)
        {
            std::vector<std::string> tokens;
            std::string cur;
            for (size_t i = 0; i < s.size(); ++i)
            {
                char c = s[i];
                if (std::isspace(c))
                    continue;

                // Number
                if (std::isdigit(c) || (c == '.' && i + 1 < s.size() && std::isdigit(s[i + 1])))
                {
                    cur.clear();
                    while (i < s.size() && (std::isdigit(s[i]) || s[i] == '.'))
                        cur.push_back(s[i++]);
                    --i;
                    tokens.push_back(cur);
                }
                // Identifier
                else if (std::isalpha(c) || c == '_' || c == '$')
                {
                    cur.clear();
                    while (i < s.size() && (std::isalnum(s[i]) || s[i] == '_' || s[i] == '.'))
                        cur.push_back(s[i++]);
                    --i;
                    tokens.push_back(cur);
                }
                // String literal
                else if (c == '"' || c == '\'')
                {
                    char quote = c;
                    cur.clear();
                    cur.push_back(c);
                    ++i;
                    while (i < s.size())
                    {
                        cur.push_back(s[i]);
                        if (s[i] == quote && s[i - 1] != '\\')
                            break;
                        ++i;
                    }
                    tokens.push_back(cur);
                }
                // Single char tokens
                else
                {
                    tokens.push_back(std::string(1, c));
                }
            }
            return tokens;
        };

        auto precedence = [](const std::string &op) -> int
        {
            // Highest precedence first
            if (op == "**")
                return 7; // Exponentiation
            if (op == "*" || op == "/" || op == "%")
                return 6; // Multiplicative
            if (op == "+" || op == "-")
                return 5; // Additive
            if (op == "<" || op == ">" || op == "<=" || op == ">=")
                return 4; // Relational
            if (op == "==" || op == "!=" || op == "===" || op == "!==")
                return 3; // Equality
            if (op == "&&")
                return 2; // Logical AND
            if (op == "||")
                return 1; // Logical OR

            return -1; // Unknown operator
        };

        auto isOperator = [](const std::string &tok)
        {
            return tok == "+" || tok == "-" || tok == "*" || tok == "/" || tok == "%" ||
                       tok == "==" || tok == "!=" || tok == "<" || tok == ">" || tok == "<=" || tok == ">=" ||
                       tok == "&&" || tok == "||" || tok == "**" || tok == "===" || tok == "!==",
                   "=", "+=", "-=", "*=", "/=", "%=";
        };

        auto tokens = tokenize(expr);

        // --- Shunting Yard with function call support ---
        std::vector<std::string> output;
        std::stack<std::string> ops;
        std::stack<int> argCount; // track args for functions

        for (size_t i = 0; i < tokens.size(); ++i)
        {
            const std::string &tok = tokens[i];

            // Literals / identifiers
            if (std::isdigit(tok[0]) || tok[0] == '"' || tok[0] == '\'' ||
                std::isalpha(tok[0]) || tok[0] == '_' || tok[0] == '$')
            {
                // Function call detection: identifier followed by '('
                if (i + 1 < tokens.size() && tokens[i + 1] == "(")
                {
                    ops.push(tok); // function name
                }
                else
                {
                    output.push_back(tok);
                }
            }
            else if (tok == ",")
            {
                // Pop until left paren
                while (!ops.empty() && ops.top() != "(")
                {
                    output.push_back(ops.top());
                    ops.pop();
                }
                if (!argCount.empty())
                    argCount.top()++;
            }
            else if (isOperator(tok))
            {
                while (!ops.empty() && isOperator(ops.top()) &&
                       precedence(ops.top()) >= precedence(tok))
                {
                    output.push_back(ops.top());
                    ops.pop();
                }
                ops.push(tok);
            }
            else if (tok == "(")
            {
                ops.push(tok);
                // If previous token was a function name, start arg count
                if (i > 0 && (std::isalpha(tokens[i - 1][0]) || tokens[i - 1][0] == '_' || tokens[i - 1][0] == '$'))
                {
                    argCount.push(1); // at least 1 arg unless empty
                }
            }
            else if (tok == ")")
            {
                while (!ops.empty() && ops.top() != "(")
                {
                    output.push_back(ops.top());
                    ops.pop();
                }
                if (!ops.empty() && ops.top() == "(")
                    ops.pop();

                // If top of ops is a function name, pop it to output
                if (!ops.empty() && !isOperator(ops.top()) && ops.top() != "(")
                {
                    std::string funcName = ops.top();
                    ops.pop();
                    // Append arg count marker
                    int argc = 0;
                    if (!argCount.empty())
                    {
                        argc = argCount.top();
                        argCount.pop();
                    }
                    output.push_back("#" + std::to_string(argc)); // arg count marker
                    output.push_back("@" + funcName);             // function marker
                }
            }
        }
        while (!ops.empty())
        {
            output.push_back(ops.top());
            ops.pop();
        }

        // --- Evaluate RPN ---
        std::stack<TS::Value> vals;

        auto applyOpVal = [&env](const std::string &op, TS::Value a, TS::Value b) -> TS::Value
        {
            if (op == "+")
            {
                if (a.type == TS::ValueType::String || b.type == TS::ValueType::String)
                    return TS::Value(a.toString() + b.toString());
                return TS::Value(a.toNumber() + b.toNumber());
            }
            if (op == "-")
                return TS::Value(a.toNumber() - b.toNumber());
            if (op == "*")
                return TS::Value(a.toNumber() * b.toNumber());
            if (op == "/")
                return TS::Value(b.toNumber() == 0 ? std::numeric_limits<double>::quiet_NaN()
                                                   : a.toNumber() / b.toNumber());
            if (op == "%")
                return TS::Value(std::fmod(a.toNumber(), b.toNumber()));

            // Comparisons
            if (op == "==")
                return TS::Value(a.data == b.data);
            if (op == "!=")
                return TS::Value(a.data != b.data);
            if (op == "===")
                return TS::Value(a.type == b.type && a.data == b.data);
            if (op == "!==")
                return TS::Value(a.type != b.type || a.data != b.data);
            if (op == "<")
                return TS::Value(a.toNumber() < b.toNumber());
            if (op == ">")
                return TS::Value(a.toNumber() > b.toNumber());
            if (op == "<=")
                return TS::Value(a.toNumber() <= b.toNumber());
            if (op == ">=")
                return TS::Value(a.toNumber() >= b.toNumber());

            // Logical
            if (op == "&&")
                return TS::Value(a.toBool() && b.toBool());
            if (op == "||")
                return TS::Value(a.toBool() || b.toBool());
            if (op == "**")
                return TS::Value(std::pow(a.toNumber(), b.toNumber()));

            return TS::Value();
        };

        for (size_t i = 0; i < output.size(); ++i)
        {
            const std::string &tok = output[i];

            if (isOperator(tok))
            {
                TS::Value b = vals.top();
                vals.pop();
                TS::Value a = vals.top();
                vals.pop();
                vals.push(applyOpVal(tok, a, b));
            }
            else if (!tok.empty() && tok[0] == '@')
            {
                // Function call
                std::string fname = tok.substr(1);
                int argc = 0;
                if (i > 0 && output[i - 1][0] == '#')
                {
                    argc = std::stoi(output[i - 1].substr(1));
                    // Remove arg count marker from stack processing
                }
                std::vector<TS::Value> args(argc);
                for (int j = argc - 1; j >= 0; --j)
                {
                    args[j] = vals.top();
                    vals.pop();
                }
                auto itB = builtins.find(fname);
                if (itB != builtins.end())
                {
                    vals.push(itB->second(args));
                }
                else
                {
                    vals.push(TS::Value()); // undefined
                }
            }
            else if (!tok.empty() && tok[0] == '#')
            {
                // arg count marker — skip, handled above
                continue;
            }
            else if ((tok.front() == '"' && tok.back() == '"') ||
                     (tok.front() == '\'' && tok.back() == '\''))
            {
                vals.push(TS::Value(tok.substr(1, tok.size() - 2)));
            }
            else if (tok == "true")
                vals.push(TS::Value(true));
            else if (tok == "false")
                vals.push(TS::Value(false));
            else if (tok == "undefined" || tok == "null")
                vals.push(TS::Value());
            else if (tok == "NaN")
                vals.push(TS::Value(std::numeric_limits<double>::quiet_NaN()));
            else
            {
                // Number or variable
                try
                {
                    double num = std::stod(tok);
                    vals.push(TS::Value(num));
                }
                catch (...)
                {
                    auto it = env.find(tok);
                    if (it != env.end())
                        vals.push(it->second);
                    else
                        vals.push(TS::Value());
                }
            }
        }

        if (!vals.empty())
            return vals.top();
        return TS::Value();
    }

    static std::string trim(const std::string &s)
    {
        auto start = s.find_first_not_of(" \t\r\n");
        auto end = s.find_last_not_of(" \t\r\n");
        if (start == std::string::npos)
            return "";
        return s.substr(start, end - start + 1);
    }

    void init(Context &ctx)
    {
        // Built-in console.log
        ctx.builtins["console.log"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            for (size_t i = 0; i < args.size(); ++i)
            {
                OS::print(args[i].toString());
                if (i < args.size() - 1)
                    OS::print(" ");
            }
            OS::printLine("");
            return TS::Value(); // undefined
        };

        // Built-in constants
        TS::setVar(ctx.variables, "NaN", TS::Value(std::numeric_limits<double>::quiet_NaN()));
        TS::setVar(ctx.variables, "undefined", TS::Value()); // null/undefined equivalent
        TS::setVar(ctx.variables, "Math.PI", TS::Value(M_PI));
        TS::setVar(ctx.variables, "Math.E", TS::Value(M_E));

        // Math functions
        ctx.builtins["Math.sqrt"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.empty())
                return TS::Value(std::numeric_limits<double>::quiet_NaN());
            return TS::Value(std::sqrt(args[0].toNumber()));
        };

        ctx.builtins["Math.sin"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.empty())
                return TS::Value(std::numeric_limits<double>::quiet_NaN());
            return TS::Value(std::sin(args[0].toNumber()));
        };

        ctx.builtins["Math.cos"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.empty())
                return TS::Value(std::numeric_limits<double>::quiet_NaN());
            return TS::Value(std::cos(args[0].toNumber()));
        };

        ctx.builtins["Math.tan"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.empty())
                return TS::Value(std::numeric_limits<double>::quiet_NaN());
            return TS::Value(std::tan(args[0].toNumber()));
        };

        ctx.builtins["Math.pow"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.size() < 2)
                return TS::Value(std::numeric_limits<double>::quiet_NaN());
            return TS::Value(std::pow(args[0].toNumber(), args[1].toNumber()));
        };

        ctx.builtins["Math.random"] = [](const std::vector<TS::Value> &) -> TS::Value
        {
            return TS::Value(static_cast<double>(std::rand()) / RAND_MAX);
        };

        ctx.builtins["Math.abs"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.empty())
                return TS::Value(std::numeric_limits<double>::quiet_NaN());
            return TS::Value(std::fabs(args[0].toNumber()));
        };

        ctx.builtins["Math.floor"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.empty())
                return TS::Value(0.0);
            return TS::Value(std::floor(args[0].toNumber()));
        };

        ctx.builtins["Math.round"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.empty())
                return TS::Value(0.0);
            return TS::Value(std::round(args[0].toNumber()));
        };

        ctx.builtins["Math.ceil"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.empty())
                return TS::Value(0.0);
            return TS::Value(std::ceil(args[0].toNumber()));
        };

        ctx.builtins["Math.trunc"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.empty())
                return TS::Value(0.0);
            return TS::Value(std::trunc(args[0].toNumber()));
        };

        ctx.builtins["Math.exp"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.empty())
                return TS::Value(0.0);
            return TS::Value(std::exp(args[0].toNumber()));
        };

        ctx.builtins["Math.log"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.empty())
                return TS::Value(0.0);
            return TS::Value(std::log(args[0].toNumber()));
        };

        ctx.builtins["Math.atan"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.empty())
                return TS::Value(0.0);
            return TS::Value(std::atan(args[0].toNumber()));
        };

        ctx.builtins["Math.asin"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.empty())
                return TS::Value(0.0);
            return TS::Value(std::atan(args[0].toNumber()));
        };

        ctx.builtins["Math.acos"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.empty())
                return TS::Value(0.0);
            return TS::Value(std::atan(args[0].toNumber()));
        };

        ctx.builtins["Math.atan2"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.empty())
                return TS::Value(0.0);
            return TS::Value(std::atan2(args[0].toNumber(), args[1].toNumber()));
        };

        ctx.builtins["Math.max"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.empty())
                return TS::Value(-INFINITY);
            double m = args[0].toNumber();
            for (size_t i = 1; i < args.size(); ++i)
            {
                m = std::max(m, args[i].toNumber());
            }
            return TS::Value(m);
        };

        ctx.builtins["Math.min"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.empty())
                return TS::Value(-INFINITY);
            double m = args[0].toNumber();
            for (size_t i = 1; i < args.size(); ++i)
            {
                m = std::min(m, args[i].toNumber());
            }
            return TS::Value(m);
        };

        ctx.builtins["sizeof"] = [](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.empty())
                return TS::Value(0.0);

            // Deep size using your Value::size()
            size_t sz = args[0].size();
            return TS::Value(static_cast<double>(sz));
        };

        ctx.builtins["assert"] = [&ctx](const std::vector<TS::Value> &args) -> TS::Value
        {
            if (args.empty())
            {
                throw std::runtime_error("assert() called with no arguments");
            }

            bool condition = evalSimpleExpression(args[0].toString(), ctx.variables, ctx.builtins).toBool();
            if (!condition)
            {
                std::string msg = "Assertion failed";
                if (args.size() > 1)
                {
                    msg += ": " + args[1].toString();
                }
                throw std::runtime_error(msg);
            }

            return TS::Value(true); // return true if assertion passes
        };
    }

    void executeLine(const std::string &rawLine, Context &ctx)
    {
        std::string line = trim(rawLine);
        if (line.empty() || line[0] == '/' && line[1] == '/')
            return; // skip comments

        // Handle variable declaration: let x = 10; or let x:any = 10;
        if (line.rfind("let ", 0) == 0)
        {
            line = line.substr(4);
            auto eqPos = line.find('=');
            if (eqPos == std::string::npos)
            {
                OS::printLine("SyntaxError: Missing '=' in let statement");
                return;
            }

            std::string varName = trim(line.substr(0, eqPos));
            if (varName.empty())
            {
                OS::printLine("SyntaxError: Missing variable name");
                return;
            }

            // Strip optional type annotation
            if (auto colonPos = varName.find(':'); colonPos != std::string::npos)
            {
                varName = trim(varName.substr(0, colonPos));
            }

            std::string expr = trim(line.substr(eqPos + 1));
            if (!expr.empty() && expr.back() == ';')
                expr.pop_back();

            try
            {
                // Merge builtins + user functions into one callable map
                std::unordered_map<std::string, std::function<TS::Value(const std::vector<TS::Value> &)>> callables(ctx.builtins);

                // Wrap user functions so they look like builtins
                for (auto &uf : ctx.userFunctions)
                {
                    callables[uf.first] = [&ctx, name = uf.first](const std::vector<TS::Value> &args) -> TS::Value
                    {
                        auto &def = ctx.userFunctions.at(name);

                        // Simple arg count check
                        if (args.size() != def.params.size())
                        {
                            OS::printLine("Error: Function '" + name + "' expects " +
                                          std::to_string(def.params.size()) + " args, got " +
                                          std::to_string(args.size()));
                            return TS::Value();
                        }

                        // Local scope
                        Interpreter::Context localCtx = ctx;
                        for (size_t i = 0; i < def.params.size(); ++i)
                        {
                            TS::setVar(localCtx.variables, def.params[i], args[i]);
                        }

                        // Execute body
                        for (auto &bodyLine : def.bodyLines)
                        {
                            std::string trimmed = trim(bodyLine);
                            if (trimmed.rfind("return", 0) == 0)
                            {
                                // Strip "return" and optional semicolon
                                std::string retExpr = trim(trimmed.substr(6));
                                if (!retExpr.empty() && retExpr.back() == ';')
                                    retExpr.pop_back();

                                // Evaluate and return immediately
                                return evalSimpleExpression(retExpr, localCtx.variables, /* merged callables */ ctx.builtins);
                            }

                            Interpreter::executeLine(bodyLine, localCtx);
                        }
                        return TS::Value(); // no return yet
                    };
                }

                // Now evaluate with both builtins and user functions
                TS::Value val = evalSimpleExpression(expr, ctx.variables, callables);
                TS::setVar(ctx.variables, varName, val);
            }
            catch (const std::exception &e)
            {
                OS::printLine(std::string("Error evaluating expression: ") + e.what());
            }
            return;
        }

        // Handle function definition: function name(param1, param2) { ... }
        if (line.rfind("function ", 0) == 0)
        {
            std::string header = line;

            // Extract function name and params
            auto nameStart = header.find(' ') + 1;
            auto parenOpen = header.find('(', nameStart);
            auto parenClose = header.find(')', parenOpen);

            std::string funcName = trim(header.substr(nameStart, parenOpen - nameStart));
            std::string paramsStr = header.substr(parenOpen + 1, parenClose - parenOpen - 1);

            FunctionDef def;

            // Parse parameters with optional type annotations
            std::istringstream pss(paramsStr);
            std::string param;
            while (std::getline(pss, param, ','))
            {
                param = trim(param);
                if (param.empty())
                    continue;

                std::string paramName = param;
                std::string paramType = "any"; // default

                auto colonPos = param.find(':');
                if (colonPos != std::string::npos)
                {
                    paramName = trim(param.substr(0, colonPos));
                    paramType = trim(param.substr(colonPos + 1));
                }

                def.params.push_back(paramName);
                def.paramTypes.push_back(paramType);
            }

            // --- Multi-line body capture ---
            std::string bodyLine;
            int braceDepth = 0;

            // If the opening brace is on the same line
            auto braceOpen = header.find('{', parenClose);
            if (braceOpen != std::string::npos)
            {
                braceDepth = 1;
                std::string afterBrace = trim(header.substr(braceOpen + 1));
                if (!afterBrace.empty())
                    def.bodyLines.push_back(afterBrace);
            }

            // Read subsequent lines until matching closing brace
            while (braceDepth > 0 && std::getline(std::cin, bodyLine))
            {
                std::string trimmed = trim(bodyLine);
                if (trimmed.find('{') != std::string::npos)
                    braceDepth++;
                if (trimmed.find('}') != std::string::npos)
                {
                    braceDepth--;
                    if (braceDepth <= 0)
                        break;
                }
                if (!trimmed.empty() && braceDepth > 0)
                {
                    def.bodyLines.push_back(trimmed);
                }
            }

            ctx.userFunctions[funcName] = def;
            return;
        }

        if (line.rfind("if ", 0) == 0)
        {
            // Merge builtins + user functions into one callable map
            std::unordered_map<std::string, std::function<TS::Value(const std::vector<TS::Value> &)>> callables(ctx.builtins);

            // Wrap user functions so they look like builtins
            for (auto &uf : ctx.userFunctions)
            {
                callables[uf.first] = [&ctx, name = uf.first](const std::vector<TS::Value> &args) -> TS::Value
                {
                    auto &def = ctx.userFunctions.at(name);

                    // Simple arg count check
                    if (args.size() != def.params.size())
                    {
                        OS::printLine("Error: Function '" + name + "' expects " +
                                      std::to_string(def.params.size()) + " args, got " +
                                      std::to_string(args.size()));
                        return TS::Value();
                    }
                    // Local scope
                    Interpreter::Context localCtx = ctx;
                    for (size_t i = 0; i < def.params.size(); ++i)
                    {
                        TS::setVar(localCtx.variables, def.params[i], args[i]);
                    }

                    // Execute body
                    for (auto &bodyLine : def.bodyLines)
                    {
                        std::string trimmed = trim(bodyLine);
                        if (trimmed.rfind("return", 0) == 0)
                        {
                            // Strip "return" and optional semicolon
                            std::string retExpr = trim(trimmed.substr(6));
                            if (!retExpr.empty() && retExpr.back() == ';')
                                retExpr.pop_back();

                            // Evaluate and return immediately
                            return evalSimpleExpression(retExpr, localCtx.variables, /* merged callables */ ctx.builtins);
                        }

                        Interpreter::executeLine(bodyLine, localCtx);
                    }
                    return TS::Value(); // no return
                };
            }

            // Extract condition between parentheses
            auto condStart = line.find('(');
            auto condEnd = line.find(')', condStart);
            if (condStart == std::string::npos || condEnd == std::string::npos)
            {
                OS::printLine("SyntaxError: malformed if statement");
                return;
            }
            std::string condExpr = trim(line.substr(condStart + 1, condEnd - condStart - 1));
            TS::Value condVal = evalSimpleExpression(condExpr, ctx.variables, callables);

            bool condTrue = condVal.toBool();

            // Find block start
            auto braceStart = line.find('{', condEnd);
            if (braceStart == std::string::npos)
            {
                OS::printLine("SyntaxError: if without block");
                return;
            }

            // Collect block lines until matching }
            int depth = 1;
            std::vector<std::string> blockLines;
            std::string blockLine;
            while (depth > 0 && std::getline(std::cin, blockLine))
            {
                if (blockLine.find('{') != std::string::npos)
                    depth++;
                if (blockLine.find('}') != std::string::npos)
                {
                    depth--;
                    if (depth == 0)
                        break;
                }
                if (depth > 0)
                    blockLines.push_back(blockLine);
            }

            if (condTrue)
            {
                for (auto &bl : blockLines)
                    executeLine(bl, ctx);
            }
            else
            {
                // Peek for else
                std::string elseLine;
                if (std::getline(std::cin, elseLine))
                {
                    elseLine = trim(elseLine);
                    if (elseLine.rfind("else", 0) == 0)
                    {
                        // Expect another block
                        std::vector<std::string> elseBlock;
                        int edepth = 0;
                        std::string el;
                        while (std::getline(std::cin, el))
                        {
                            if (el.find('{') != std::string::npos)
                                edepth++;
                            if (el.find('}') != std::string::npos)
                            {
                                edepth--;
                                if (edepth == 0)
                                    break;
                            }
                            if (edepth > 0)
                                elseBlock.push_back(el);
                        }
                        for (auto &bl : elseBlock)
                            executeLine(bl, ctx);
                    }
                }
            }
            return;
        }

        if (line.rfind("class ", 0) == 0)
        {
            // Extract class name
            auto nameStart = 6;
            auto nameEnd = line.find('{', nameStart);
            std::string className = trim(line.substr(nameStart, nameEnd - nameStart));

            // Read class body until matching '}'
            int depth = 0;
            std::vector<std::string> bodyLines;
            std::string bodyLine;

            if (line.find('{') != std::string::npos)
                depth++;
            while (depth > 0 && std::getline(std::cin, bodyLine))
            {
                if (bodyLine.find('{') != std::string::npos)
                    depth++;
                if (bodyLine.find('}') != std::string::npos)
                {
                    depth--;
                    if (depth == 0)
                        break;
                }
                if (depth > 0)
                    bodyLines.push_back(trim(bodyLine));
            }

            // Process each body line
            for (auto &bl : bodyLines)
            {
                if (bl.rfind("static ", 0) == 0)
                {
                    std::string rest = trim(bl.substr(7));

                    // Method: static name(params) { ... }
                    if (rest.find('(') != std::string::npos)
                    {
                        // Treat like a function definition with name "ClassName.method"
                        std::string funcName = className + "." + trim(rest.substr(0, rest.find('(')));
                        std::string header = line;

                        // Extract function name and params
                        auto nameStart = header.find(' ') + 1;
                        auto parenOpen = header.find('(', nameStart);
                        auto parenClose = header.find(')', parenOpen);

                        std::string paramsStr = header.substr(parenOpen + 1, parenClose - parenOpen - 1);

                        FunctionDef def;

                        // Parse parameters with optional type annotations
                        std::istringstream pss(paramsStr);
                        std::string param;
                        while (std::getline(pss, param, ','))
                        {
                            param = trim(param);
                            if (param.empty())
                                continue;

                            std::string paramName = param;
                            std::string paramType = "any"; // default

                            auto colonPos = param.find(':');
                            if (colonPos != std::string::npos)
                            {
                                paramName = trim(param.substr(0, colonPos));
                                paramType = trim(param.substr(colonPos + 1));
                            }

                            def.params.push_back(paramName);
                            def.paramTypes.push_back(paramType);
                        }

                        // --- Multi-line body capture ---
                        std::string bodyLine;
                        int braceDepth = 0;

                        // If the opening brace is on the same line
                        auto braceOpen = header.find('{', parenClose);
                        if (braceOpen != std::string::npos)
                        {
                            braceDepth = 1;
                            std::string afterBrace = trim(header.substr(braceOpen + 1));
                            if (!afterBrace.empty())
                                def.bodyLines.push_back(afterBrace);
                        }

                        // Read subsequent lines until matching closing brace
                        while (braceDepth > 0 && std::getline(std::cin, bodyLine))
                        {
                            std::string trimmed = trim(bodyLine);
                            if (trimmed.find('{') != std::string::npos)
                                braceDepth++;
                            if (trimmed.find('}') != std::string::npos)
                            {
                                braceDepth--;
                                if (braceDepth <= 0)
                                    break;
                            }
                            if (!trimmed.empty() && braceDepth > 0)
                            {
                                def.bodyLines.push_back(trimmed);
                            }
                        }

                        ctx.userFunctions[funcName] = def;
                    }
                    // Property: static name = value;
                    else if (rest.find('=') != std::string::npos)
                    {
                        auto eqPos = rest.find('=');
                        std::string propName = className + "." + trim(rest.substr(0, eqPos));
                        std::string expr = trim(rest.substr(eqPos + 1));
                        if (!expr.empty() && expr.back() == ';')
                            expr.pop_back();
                        // Merge builtins + user functions into one callable map
                        std::unordered_map<std::string, std::function<TS::Value(const std::vector<TS::Value> &)>> callables(ctx.builtins);

                        // Wrap user functions so they look like builtins
                        for (auto &uf : ctx.userFunctions)
                        {
                            callables[uf.first] = [&ctx, name = uf.first](const std::vector<TS::Value> &args) -> TS::Value
                            {
                                auto &def = ctx.userFunctions.at(name);

                                // Simple arg count check
                                if (args.size() != def.params.size())
                                {
                                    OS::printLine("Error: Function '" + name + "' expects " +
                                                  std::to_string(def.params.size()) + " args, got " +
                                                  std::to_string(args.size()));
                                    return TS::Value();
                                }
                                // Local scope
                                Interpreter::Context localCtx = ctx;
                                for (size_t i = 0; i < def.params.size(); ++i)
                                {
                                    TS::setVar(localCtx.variables, def.params[i], args[i]);
                                }

                                // Execute body
                                for (auto &bodyLine : def.bodyLines)
                                {
                                    std::string trimmed = trim(bodyLine);
                                    if (trimmed.rfind("return", 0) == 0)
                                    {
                                        // Strip "return" and optional semicolon
                                        std::string retExpr = trim(trimmed.substr(6));
                                        if (!retExpr.empty() && retExpr.back() == ';')
                                            retExpr.pop_back();

                                        // Evaluate and return immediately
                                        return evalSimpleExpression(retExpr, localCtx.variables, /* merged callables */ ctx.builtins);
                                    }

                                    Interpreter::executeLine(bodyLine, localCtx);
                                }
                                return TS::Value(); // no return yet
                            };
                        }
                        TS::Value val = evalSimpleExpression(expr, ctx.variables, callables);
                        TS::setVar(ctx.variables, propName, val);
                    }
                }
            }
            return;
        }

        // --- Handle function calls: name(arg1, arg2, ...) ---
        auto parenOpen = line.find('(');
        auto parenClose = line.rfind(')'); // use rfind to get the last closing parenthesis

        if (parenOpen != std::string::npos && parenClose != std::string::npos && parenClose > parenOpen)
        {
            std::string funcName = trim(line.substr(0, parenOpen));
            std::string argsStr = line.substr(parenOpen + 1, parenClose - parenOpen - 1);

            std::vector<TS::Value> args;
            std::string currentArg;
            bool inString = false;
            char stringChar = '\0';
            int parenDepth = 0;

            // --- Robust argument splitting ---
            for (size_t i = 0; i < argsStr.size(); ++i)
            {
                char c = argsStr[i];

                if ((c == '"' || c == '\'') && !inString)
                {
                    inString = true;
                    stringChar = c;
                    currentArg.push_back(c);
                }
                else if (inString && c == stringChar)
                {
                    inString = false;
                    currentArg.push_back(c);
                }
                else if (!inString && c == '(')
                {
                    parenDepth++;
                    currentArg.push_back(c);
                }
                else if (!inString && c == ')')
                {
                    parenDepth--;
                    currentArg.push_back(c);
                }
                else if (!inString && parenDepth == 0 && c == ',')
                {
                    // End of argument
                    std::string argTrimmed = trim(currentArg);
                    if (!argTrimmed.empty())
                    {
                        // Parse argument value
                        if (argTrimmed == "true")
                            args.push_back(TS::Value(true));
                        else if (argTrimmed == "false")
                            args.push_back(TS::Value(false));
                        else if (argTrimmed == "null")
                            args.push_back(TS::Value());
                        else
                        {
                            try
                            {
                                double num = std::stod(argTrimmed);
                                args.push_back(TS::Value(num));
                            }
                            catch (...)
                            {
                                if (argTrimmed.size() >= 2 &&
                                    ((argTrimmed.front() == '"' && argTrimmed.back() == '"') ||
                                     (argTrimmed.front() == '\'' && argTrimmed.back() == '\'')))
                                {
                                    args.push_back(TS::Value(argTrimmed.substr(1, argTrimmed.size() - 2)));
                                }
                                else
                                {
                                    auto val = TS::getVar(ctx.variables, argTrimmed);
                                    if (val)
                                        args.push_back(*val);
                                    else
                                        args.push_back(TS::Value());
                                }
                            }
                        }
                    }
                    currentArg.clear();
                }
                else
                {
                    currentArg.push_back(c);
                }
            }

            // Last argument (if any)
            if (!currentArg.empty())
            {
                std::string argTrimmed = trim(currentArg);
                if (!argTrimmed.empty())
                {
                    if (argTrimmed == "true")
                        args.push_back(TS::Value(true));
                    else if (argTrimmed == "false")
                        args.push_back(TS::Value(false));
                    else if (argTrimmed == "null")
                        args.push_back(TS::Value());
                    else
                    {
                        try
                        {
                            double num = std::stod(argTrimmed);
                            args.push_back(TS::Value(num));
                        }
                        catch (...)
                        {
                            if (argTrimmed.size() >= 2 &&
                                ((argTrimmed.front() == '"' && argTrimmed.back() == '"') ||
                                 (argTrimmed.front() == '\'' && argTrimmed.back() == '\'')))
                            {
                                args.push_back(TS::Value(argTrimmed.substr(1, argTrimmed.size() - 2)));
                            }
                            else
                            {
                                auto val = TS::getVar(ctx.variables, argTrimmed);
                                if (val)
                                    args.push_back(*val);
                                else
                                    args.push_back(TS::Value());
                            }
                        }
                    }
                }
            }

            // --- Built-in function? ---
            if (ctx.builtins.find(funcName) != ctx.builtins.end())
            {
                ctx.builtins[funcName](args);
                return;
            }

            // --- User-defined function? ---
            if (ctx.userFunctions.find(funcName) != ctx.userFunctions.end())
            {
                auto &def = ctx.userFunctions[funcName];

                // Type checking
                if (args.size() != def.params.size())
                {
                    OS::printLine("Error: Function '" + funcName + "' expects " +
                                  std::to_string(def.params.size()) + " arguments, got " +
                                  std::to_string(args.size()));
                    return;
                }

                for (size_t i = 0; i < def.params.size(); ++i)
                {
                    const std::string &expectedType = def.paramTypes[i];
                    if (expectedType != "any")
                    {
                        bool typeOk = false;
                        if (expectedType == "number" && args[i].type == TS::ValueType::Number)
                            typeOk = true;
                        if (expectedType == "string" && args[i].type == TS::ValueType::String)
                            typeOk = true;
                        if (expectedType == "boolean" && args[i].type == TS::ValueType::Boolean)
                            typeOk = true;
                        if (!typeOk)
                        {
                            OS::printLine("TypeError: Argument '" + def.params[i] + "' expected " +
                                          expectedType + ", got " + args[i].toString());
                            return;
                        }
                    }
                }

                // Local scope
                Context localCtx = ctx;
                for (size_t i = 0; i < def.params.size(); ++i)
                {
                    TS::setVar(localCtx.variables, def.params[i], args[i]);
                }

                // Execute body
                for (auto &bodyLine : def.bodyLines)
                {
                    executeLine(bodyLine, localCtx);
                }
                return;
            }

            OS::printLine("Error: Unknown function '" + funcName + "'");
            return;
        }

        OS::printLine("Error: Unrecognized statement: " + line);
    }

    void executeScript(const std::vector<std::string> &lines, Context &ctx)
    {
        for (auto &line : lines)
        {
            executeLine(line, ctx);
        }
    }

} // namespace Interpreter