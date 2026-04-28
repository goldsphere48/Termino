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
        symbol == '+' ||
        symbol == '*' ||
        symbol == '/' ||
        symbol == '%' ||
        symbol == '^' ||
        symbol == '&' ||
        symbol == '|' ||
        symbol == '~';
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

bool IsNumber(char symbol)
{
    return symbol >= '0' && symbol <= '9';
}

std::optional<TOKEN_TYPE> ParseNumber(std::string_view view)
{
    if (view.empty())
    {
        return { };
    }

    int current = 0;
    bool isHexOrBin = view.length() >= 3 && (
        view[current] == '0' && view[current + 1] == 'X' ||
        view[current] == '0' && view[current + 1] == 'B'
    );
    
    if (isHexOrBin)
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
            if (isHexOrBin)
            {
                return { };
            }
        }
        else if (IsNumber(view[current]))
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

    std::string upper = std::string(view);
    std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c) { return std::toupper(c); });
    
    if (Convert::opCode(upper) != std::nullopt || Convert::directive(upper) != std::nullopt)
    {
        return false;
    }

    std::string_view body = view.substr(1);
    bool hasAlnum = IsLetter(view[0]);
    for (char c : body)
    {
        if (IsLetter(c) || IsNumber(c))
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

Lexer::Lexer(ErrorCollector& errorCollector) noexcept
    : m_errorCollector(errorCollector)
{
    
}

std::vector<Token> Lexer::tokenize(const std::string& source) const
{
    std::vector<Token> result; 
    if (source.empty())
    {
        return result;
    }

    std::size_t current = 0;
    std::size_t line = 1;
    std::size_t column = 1;
    
    while (current < source.length())
    {
        while (current < source.length() && IsWhiteSpace(source[current]))
        {
            if (IsNewLine(source[current]))
            {
                column = 1;
                line++;
            }
            else
            {
                column++;
            }
            current++;
        }

        if (current >= source.length())
        {
            break; 
        }

        std::size_t tokenStartColumn = column;
        std::size_t tokenStartLine = line;
        std::size_t previous = current;

        while (current < source.length() && !IsWhiteSpace(source[current]))
        {
            if (IsOperator(source[current]))
            {
                if (current == previous)
                {
                    current++;
                    column++;
                }
                
                break;
            }

            
            current++;
            column++;
        }

        std::string value = source.substr(previous, current - previous);
        std::string origin = value;
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::toupper(c); });

        if (value.back() == ':')
        {
            std::string label = origin.substr(0, origin.length() - 1);
            if (label.length() > 0 && IsIdentifier(label))
            {
                result.push_back(
                    Token {
                        .Type = TOKEN_TYPE::LABEL_DEF,
                        .Value = label,
                        .Line = tokenStartLine,
                        .Column = tokenStartColumn
                    }
                );
            }
            else
            {
                m_errorCollector.report(ErrorType::INVALID_LABEL, tokenStartLine, tokenStartColumn, { label });
            }
        }
        else if (value.length() > 1 && value[0] == '.' && IsLetter(value[1])) {
            std::string directiveStr = value.substr(1);
            if (auto directive = Convert::directive(directiveStr); directive != std::nullopt)
            {
                result.push_back(
                    Token {
                        .Type = TOKEN_TYPE::DIRECTIVE,
                        .Value = directive.value(),
                        .Line = tokenStartLine,
                        .Column = tokenStartColumn
                    }
                );
            }
            else
            {
                m_errorCollector.report(ErrorType::UNKNOWN_DIRECTIVE, tokenStartLine, tokenStartColumn, { directiveStr });
            }
        }
        else if (auto keyword = Convert::opCode(value); keyword != std::nullopt)
        {
            result.push_back(
                Token {
                    .Type = TOKEN_TYPE::INSTRUCTION,
                    .Value = keyword.value(),
                    .Line = tokenStartLine,
                    .Column = tokenStartColumn
                }
            );
        }
        else if (auto oper = Convert::oper(value); oper != std::nullopt)
        {
            result.push_back(
                Token {
                    .Type = TOKEN_TYPE::OPERATOR,
                    .Value = oper.value(),
                    .Line = tokenStartLine,
                    .Column = tokenStartColumn
                }
            );
        }
        else if (auto type = ParseNumber(value); type != std::nullopt)
        {
            if (type == TOKEN_TYPE::INT)
            {
                bool isHex = value[0] == '0' && value[1] == 'X';
                bool isBin = value[0] == '0' && value[1] == 'B';
                
                int shift = isHex || isBin ? 2 : 0;
                int base = isHex ? 16 : (isBin ? 2 : 10);
                int intValue = 0;
                
                auto[ptr, ec] = std::from_chars(value.data() + shift, value.data() + value.length(), intValue, base);
                
                if (ec == std::errc())
                {   
                    result.push_back(
                        Token {
                            .Type = TOKEN_TYPE::INT,
                            .Value = intValue,
                            .Line = tokenStartLine,
                            .Column = tokenStartColumn
                        }
                    );
                }
                else
                {
                    if (ec == std::errc::result_out_of_range)
                    {
                        m_errorCollector.report(ErrorType::INTEGER_LITERAL_OVERFLOW, tokenStartLine, tokenStartColumn, { value });
                    }
                }
            }
                
            if (type == TOKEN_TYPE::FLOAT)
            {
                float floatValue = 0;
                auto[ptr, ec] = std::from_chars(value.data(), value.data() + value.length(), floatValue);
                if (ec == std::errc())
                {
                    result.push_back(
                        Token {
                            .Type = TOKEN_TYPE::FLOAT,
                            .Value = floatValue,
                            .Line = tokenStartLine,
                            .Column = tokenStartColumn
                        }
                    );
                }
                else
                {
                    if (ec == std::errc::result_out_of_range)
                    {
                        m_errorCollector.report(ErrorType::FLOAT_LITERAL_OVERFLOW, tokenStartLine, tokenStartColumn, { value });
                    }
                }
            }
        }
        else if (IsIdentifier(value))
        {
            result.push_back(
                Token {
                    .Type = TOKEN_TYPE::IDENTIFIER,
                    .Value = origin,
                    .Line = tokenStartLine,
                    .Column = tokenStartColumn
                }
            );
        }
        else
        {
            m_errorCollector.report(ErrorType::UNKNOWN_INSTRUCTION, line, tokenStartColumn, { value });
        }
    }
    
    return result;
}

void PrintToken(const Token& token)
{
    std::cout << "{ TOKEN_TYPE = "
              << Stringify::tokenType(token.Type)
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
