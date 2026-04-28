#pragma once

#include <vector>
#include <variant>
#include <string>

#include "error.h"
#include "isa.h"

using TokenValue = std::variant<
    std::monostate,
    std::string,
    int,
    float,
    OP_CODE,
    DIRECTIVE,
    OPERATOR
>;

struct Token
{
    TOKEN_TYPE Type;
    TokenValue Value;
    std::size_t Line;
    std::size_t Column;

    bool hasInt() const { return std::holds_alternative<int>(Value); }
    bool hasFloat() const { return std::holds_alternative<float>(Value); }
    bool hasString() const { return std::holds_alternative<std::string>(Value); }
    bool hasOpCode() const { return std::holds_alternative<OP_CODE>(Value);}
    bool hasDirective() const { return std::holds_alternative<DIRECTIVE>(Value);}
    bool hasOperator() const { return std::holds_alternative<OPERATOR>(Value);}

    int getInt() const { return std::get<int>(Value); }
    float getFloat() const { return std::get<float>(Value); }
    const std::string& getString() const { return std::get<std::string>(Value); }
    OP_CODE getOpCode() const { return std::get<OP_CODE>(Value); }
    DIRECTIVE getDirective() const { return std::get<DIRECTIVE>(Value); }
    OPERATOR getOperator() const { return std::get<OPERATOR>(Value); }
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
