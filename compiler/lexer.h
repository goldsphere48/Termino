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
    TOKEN_TYPE type;
    TokenValue value;
    std::size_t line;
    std::size_t column;

    bool hasInt() const { return std::holds_alternative<int>(value); }
    bool hasFloat() const { return std::holds_alternative<float>(value); }
    bool hasString() const { return std::holds_alternative<std::string>(value); }
    bool hasOpCode() const { return std::holds_alternative<OP_CODE>(value);}
    bool hasDirective() const { return std::holds_alternative<DIRECTIVE>(value);}
    bool hasOperator() const { return std::holds_alternative<OPERATOR>(value);}

    int getInt() const { return std::get<int>(value); }
    float getFloat() const { return std::get<float>(value); }
    const std::string& getString() const { return std::get<std::string>(value); }
    OP_CODE getOpCode() const { return std::get<OP_CODE>(value); }
    DIRECTIVE getDirective() const { return std::get<DIRECTIVE>(value); }
    OPERATOR getOperator() const { return std::get<OPERATOR>(value); }
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
