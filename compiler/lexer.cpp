#include "lexer.h"

#include <iostream>
#include <string_view>
#include <charconv>
#include <optional>
#include <algorithm>
#include <cctype>

#include "isa_helper.h"

bool IsOperator(char symbol)
{
    return
        symbol == '-' ||
        symbol == '+';
}

bool IsLetter(char symbol)
{
    return (symbol >= 'a'  && symbol <= 'z') ||  (symbol >= 'A' && symbol <= 'Z');
}

bool IsSpace(char symbol)
{
    return symbol == ' ' || symbol == '\t' || symbol == '\r';
}

bool IsNewLine(char symbol)
{
    return symbol == '\n';
}

bool IsWhiteSpace(char symbol)
{
    return IsSpace(symbol) || IsNewLine(symbol);
}

bool IsNumber(char symbol, int base)
{
    if (base == 10)
    {
        return symbol >= '0' && symbol <= '9';
    }
    if (base == 2)
    {
        return symbol == '0' || symbol == '1';
    }
    if (base == 16)
    {
        return (symbol >= '0' && symbol <= 'z');
    }

    return false;
}

std::optional<TOKEN_TYPE> ParseNumber(std::string_view view)
{
    if (view.empty())
    {
        return { };
    }

    int current = 0;
    
    bool isHex = view.length() >= 3 && view[current] == '0' && (view[current + 1] == 'X' || view[current + 1] == 'x');
    bool isBin = view.length() >= 3 && view[current] == '0' && (view[current + 1] == 'B' || view[current + 1] == 'b');
    int base = isHex ? 16 : (isBin ? 2 : 10);

    if (isHex || isBin)
    {
        current += 2;
    }

    int digitCount = 0;
    bool hasDot = false;

    while (current < view.length())
    {
        if (view[current] == '.' && !hasDot)
        {
            hasDot = true;
            if (isHex || isBin)
            {
                return { };
            }
        }
        else if (IsNumber(view[current], base))
        {
            digitCount++;
        }
        else
        {
            return { };
        }
        current++;
    }

    if (digitCount == 0)
    {
        return { };
    }

    return hasDot ? TOKEN_TYPE::FLOAT : TOKEN_TYPE::INT;
}

bool IsIdentifier(std::string_view view)
{
    if (view.empty() || (view[0] != '_' && !IsLetter(view[0])))
    {
        return false;
    }

    std::string value = std::string(view);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::toupper(c); });

    if (Convert::opCode(value) != std::nullopt || Convert::directive(value) != std::nullopt)
    {
        return false;
    }

    std::string_view body = view.substr(1);
    bool hasAlnum = IsLetter(view[0]);
    for (char c : body)
    {
        if (IsLetter(c) || IsNumber(c, 10))
        {
            hasAlnum = true;
        }
        else if (c != '_')
        {
            return false;
        }
    }

    return hasAlnum;
}

Lexer::Lexer(ErrorCollector& errorCollector, std::string_view source) noexcept
    : m_source(source), m_errorCollector(errorCollector)
{
}

std::vector<Token> Lexer::tokenize()
{
    std::vector<Token> result;

    while (m_pos < m_source.length())
    {
        skipWhitespace();

        if (m_pos >= m_source.length())
        {
            break;
        }

        m_previous = m_pos;
        m_tokenStartLine = m_line;
        m_tokenStartColumn = m_column;

        if (auto token = readNext(); token.has_value())
        {
            result.push_back(std::move(*token));
        }
    }

    result.push_back(Token {
        .type = TOKEN_TYPE::END_OF_FILE,
        .value = std::monostate{},
        .line = m_line,
        .column = m_column
    });

    return result;
}

void Lexer::skipWhitespace()
{
    while (m_pos < m_source.length() && IsWhiteSpace(peek()))
    {
        advance();
    }
}

std::optional<Token> Lexer::readNext()
{
    if (peek() == '"')
    {
        return readString();
    }

    while (m_pos < m_source.length() && !IsWhiteSpace(peek()))
    {
        if (IsOperator(peek()))
        {
            if (m_pos == m_previous)
            {
                advance();
            }
            break;
        }
        advance();
    }

    std::string_view value = getCurrentSubstring();

    if (auto t = tryParseMark(value)) return t;
    if (auto t = tryParseDirective(value)) return t;
    if (auto t = tryParseOpCode(value)) return t;
    if (auto t = tryParseOperator(value)) return t;
    if (auto t = tryParseNumber(value)) return t;
    if (auto t = tryParseIdentifier(value)) return t;

    m_errorCollector.report(
        ErrorType::UNKNOWN_INSTRUCTION,
        m_tokenStartLine,
        m_tokenStartColumn,
        { std::string(value) }
    );
    return { };
}

std::optional<Token> Lexer::readString()
{
    if (peek() != '"')
    {
        return { };
    }

    advance();

    size_t contentStart = m_pos;

    while (m_pos < m_source.length() && peek() != '"' && !IsNewLine(peek()))
    {
        advance();
    }

    if (m_pos >= m_source.length() || IsNewLine(peek()))
    {
        m_errorCollector.report(
            ErrorType::UNEXPECTED_END_OF_STRING,
            m_tokenStartLine,
            m_tokenStartColumn
        );
        return { };
    }

    std::string content(m_source.substr(contentStart, m_pos - contentStart));
    advance();

    return Token {
        .type = TOKEN_TYPE::STRING,
        .value = std::move(content),
        .line = m_tokenStartLine,
        .column = m_tokenStartColumn
    };
}

std::string_view Lexer::getCurrentSubstring() const
{
    return m_source.substr(m_previous, m_pos - m_previous);
}

char Lexer::peek() const
{
    if (m_pos >= m_source.length())
    {
        return '\0';
    }
    return m_source[m_pos];
}

char Lexer::advance()
{
    if (m_pos >= m_source.length())
    {
        return '\0';
    }

    char c = m_source[m_pos++];
    if (IsNewLine(c))
    {
        m_line++;
        m_column = 1;
    }
    else
    {
        m_column++;
    }
    return c;
}

std::string Lexer::getUpperCase(std::string_view value) const
{
    std::string result;
    result.resize(value.length());
    std::transform(value.begin(), value.end(), result.begin(), [](unsigned char c) { return std::toupper(c); });
    return result;
}

std::optional<Token> Lexer::tryParseMark(std::string_view value) const
{
    if (value.empty() || value.back() != ':')
    {
        return { };
    }

    std::string label(value.substr(0, value.length() - 1));
    if (label.empty() || !IsIdentifier(label))
    {
        m_errorCollector.report(
            ErrorType::INVALID_LABEL,
            m_tokenStartLine,
            m_tokenStartColumn,
            { label }
        );
        return { };
    }

    return Token {
        .type = TOKEN_TYPE::LABEL_DEF,
        .value = label,
        .line = m_tokenStartLine,
        .column = m_tokenStartColumn
    };
}

std::optional<Token> Lexer::tryParseDirective(std::string_view value) const
{
    if (value.length() < 2 || value[0] != '.' || !IsLetter(value[1]))
    {
        return { };
    }

    std::string directiveStr = getUpperCase(value.substr(1));
    if (auto directive = Convert::directive(directiveStr); directive != std::nullopt)
    {
        return Token {
            .type = TOKEN_TYPE::DIRECTIVE,
            .value = directive.value(),
            .line = m_tokenStartLine,
            .column = m_tokenStartColumn
        };
    }

    m_errorCollector.report(
        ErrorType::UNKNOWN_DIRECTIVE,
        m_tokenStartLine,
        m_tokenStartColumn,
        { directiveStr }
    );
    return { };
}

std::optional<Token> Lexer::tryParseOpCode(std::string_view value) const
{
    std::string upper = getUpperCase(value);
    if (auto opCode = Convert::opCode(upper); opCode != std::nullopt)
    {
        return Token {
            .type = TOKEN_TYPE::INSTRUCTION,
            .value = opCode.value(),
            .line = m_tokenStartLine,
            .column = m_tokenStartColumn
        };
    }

    return { };
}

std::optional<Token> Lexer::tryParseOperator(std::string_view value) const
{
    if (auto oper = Convert::oper(value); oper != std::nullopt)
    {
        return Token {
            .type = TOKEN_TYPE::OPERATOR,
            .value = oper.value(),
            .line = m_tokenStartLine,
            .column = m_tokenStartColumn
        };
    }

    return { };
}

std::optional<Token> Lexer::tryParseNumber(std::string_view value) const
{
    auto type = ParseNumber(value);
    if (type == std::nullopt)
    {
        return { };
    }

    if (type == TOKEN_TYPE::INT)
    {
        bool isHex = value.length() >= 2 && value[0] == '0' && (value[1] == 'X' || value[1] == 'x');
        bool isBin = value.length() >= 2 && value[0] == '0' && (value[1] == 'B' || value[1] == 'b');

        int shift = (isHex || isBin) ? 2 : 0;
        int base = isHex ? 16 : (isBin ? 2 : 10);
        int intValue = 0;

        auto[ptr, ec] = std::from_chars(
            value.data() + shift,
            value.data() + value.length(),
            intValue,
            base
        );

        if (ec == std::errc())
        {
            return Token {
                .type = TOKEN_TYPE::INT,
                .value = intValue,
                .line = m_tokenStartLine,
                .column = m_tokenStartColumn
            };
        }

        if (ec == std::errc::result_out_of_range)
        {
            m_errorCollector.report(
                ErrorType::INTEGER_LITERAL_OVERFLOW,
                m_tokenStartLine,
                m_tokenStartColumn,
                { std::string(value) }
            );
        }
        return { };
    }

    if (type == TOKEN_TYPE::FLOAT)
    {
        float floatValue = 0;
        auto[ptr, ec] = std::from_chars(
            value.data(),
            value.data() + value.length(),
            floatValue
        );

        if (ec == std::errc())
        {
            return Token {
                .type = TOKEN_TYPE::FLOAT,
                .value = floatValue,
                .line = m_tokenStartLine,
                .column = m_tokenStartColumn
            };
        }

        if (ec == std::errc::result_out_of_range)
        {
            m_errorCollector.report(
                ErrorType::FLOAT_LITERAL_OVERFLOW,
                m_tokenStartLine,
                m_tokenStartColumn,
                { std::string(value) }
            );
        }
    }

    return { };
}

std::optional<Token> Lexer::tryParseIdentifier(std::string_view value) const
{
    if (IsIdentifier(value))
    {
        return Token {
            .type = TOKEN_TYPE::IDENTIFIER,
            .value = std::string(value),
            .line = m_tokenStartLine,
            .column = m_tokenStartColumn
        };
    }

    return { };
}

void PrintToken(const Token& token)
{
    std::cout << "{ TOKEN_TYPE = "
              << Stringify::tokenType(token.type)
              << ", Value = ";

    if (token.hasInt())
    {
        std::cout << token.getInt();
    }

    if (token.hasFloat())
    {
        std::cout << token.getFloat();
    }

    if (token.hasString())
    {
        std::cout << token.getString();
    }

    if (token.hasOpCode())
    {
        std::cout << Stringify::opCode(token.getOpCode());
    }

    if (token.hasDirective())
    {
        std::cout << Stringify::directive(token.getDirective());
    }

    if (token.hasOperator())
    {
        std::cout << Stringify::oper(token.getOperator());
    }

    std::cout << " }" << std::endl;
}
