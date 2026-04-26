#include "lexer.h"

#include <string_view>
#include <unordered_map>
#include <charconv>
#include <optional>

#include "utils.h"

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
    if (view[current] == '-')
    {
        current++;
    }

    int digitCount = 0;
    bool hasDot = false;

    while (current < view.length())
    {
        if (view[current] == '.' && !hasDot)
        {
            hasDot = true;
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

    static const std::unordered_map<std::string_view, OP_CODE> keywords = {
        { "PUSH8",  OP_CODE::PUSH8 },
        { "PUSH16", OP_CODE::PUSH16 },
        { "PUSHF",  OP_CODE::PUSHF },
        { "POP",    OP_CODE::POP },
        { "ADD",    OP_CODE::ADD },
        { "SUB",    OP_CODE::SUB },
        { "MUL",    OP_CODE::MUL },
    };
    
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
            current++;
            column++;
        }
        
        std::string value = source.substr(previous, current - previous);
        if (auto it = keywords.find(value); it != keywords.end())
        {
            result.push_back(Token { .Type = TOKEN_TYPE::INSTRUCTION, .Value = it->second });
        }
        else if (auto type = ParseNumber(value); type != std::nullopt)
        {
            if (type == TOKEN_TYPE::INT)
            {
                int intValue = 0;
                auto[ptr, ec] = std::from_chars(value.data(), value.data() + value.length(), intValue);
                if (ec == std::errc())
                {
                    result.push_back(
                        Token {
                            .Type = TOKEN_TYPE::INT,
                            .Value = intValue
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
                            .Value = floatValue
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
        else
        {
            m_errorCollector.report(ErrorType::UNKNOWN_INSTRUCTION, line, tokenStartColumn, { value });
        }
    }
    
    return result;
}

std::string_view TokenTypeToString(TOKEN_TYPE type)
{
    switch(type)
    {
    case TOKEN_TYPE::INSTRUCTION:
        return "INSTRUCTION";
    case TOKEN_TYPE::INT:
        return "INT";
    case TOKEN_TYPE::FLOAT:
        return "FLOAT";
    }
    
    return "?";
}

void PrintToken(const Token& token)
{
    std::cout << "{ TOKEN_TYPE = "
              << TokenTypeToString(token.Type)
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
        std::cout << OpCodeToString(token.getOpCode());
    }
    
    std::cout << " }" << std::endl;
}
