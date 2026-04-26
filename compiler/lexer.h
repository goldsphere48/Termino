#pragma once
#include "opcode.h"

#include <vector>
#include <string>
#include <variant>

#include "error.h"

enum class TOKEN_TYPE
{
    INT,
    FLOAT,
    INSTRUCTION,
};

using TokenValue = std::variant<std::monostate, std::string, int, float, OP_CODE>;

struct Token
{
    TOKEN_TYPE Type;
    TokenValue Value;

    bool hasInt() const { return std::holds_alternative<int>(Value); }
    bool hasFloat() const { return std::holds_alternative<float>(Value); }
    bool hasString() const { return std::holds_alternative<std::string>(Value); }
    bool hasOpCode() const { return std::holds_alternative<OP_CODE>(Value);}

    int getInt() const { return std::get<int>(Value); }
    float getFloat() const { return std::get<float>(Value); }
    std::string getString() const { return std::get<std::string>(Value); }
    OP_CODE getOpCode() const { return std::get<OP_CODE>(Value); }
};

class Lexer
{
public:
    explicit Lexer(ErrorCollector& errorCollector) noexcept;
    
    std::vector<Token> tokenize(const std::string& source) const;
    
private:
    ErrorCollector& m_errorCollector;
};

void PrintToken(const Token& token);
