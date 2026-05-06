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
    explicit Lexer(ErrorCollector& errorCollector, std::string_view source) noexcept;

    std::vector<Token> tokenize();

private:
    void skipWhitespace();
    std::optional<Token> readString();
    std::optional<Token> readNext();

    std::optional<Token> tryParseMark(std::string_view value) const;
    std::optional<Token> tryParseDirective(std::string_view value) const;
    std::optional<Token> tryParseOpCode(std::string_view value) const;
    std::optional<Token> tryParseOperator(std::string_view value) const;
    std::optional<Token> tryParseNumber(std::string_view value) const;
    std::optional<Token> tryParseIdentifier(std::string_view value) const;

    std::string getUpperCase(std::string_view value) const;
    std::string_view getCurrentSubstring() const;
    char peek() const;
    char advance();

private:
    size_t m_pos = 0;
    size_t m_previous = 0;
    size_t m_tokenStartLine = 1;
    size_t m_tokenStartColumn = 1;
    size_t m_line = 1;
    size_t m_column = 1;
    
    std::string_view m_source;
    ErrorCollector& m_errorCollector;
};

void PrintToken(const Token& token);
