#include "error.h"

#include "isa_helper.h"
#include "lexer.h"

#include <format>
#include <iostream>

std::string errorTemplate(ErrorType type)
{
    switch(type)
    {
    case ErrorType::UNKNOWN_INSTRUCTION:
        return "Unknown instruction {}";
    case ErrorType::INTEGER_LITERAL_OVERFLOW:
        return "Integer literal {} is out of range";
    case ErrorType::FLOAT_LITERAL_OVERFLOW:
        return "Floating literal {} is out of range";
    case ErrorType::UNKNOWN_DIRECTIVE:
        return "Unknown directive {}";
    case ErrorType::INVALID_LABEL:
        return "Invalid label {}";
    case ErrorType::UNEXPECTED_TOKEN_TYPE:
        return "Unexpected token type {}, expected {}";
    case ErrorType::EXPECTED_DATA_DIRECTIVE:
        return "Expected data directive, got {}";
    case ErrorType::BYTE_LITERAL_OVERFLOW:
        return "Byte literal overflow {}";
    case ErrorType::UNRECOGNIZED_TOKEN:
        return "Unrecognized token {}";
    case ErrorType::UNEXPECTED_DIRECTIVE:
        return "Unexpected directive {}, expected {}";
    case ErrorType::UNEXPECTED_OPERATOR:
        return "Unexpected operator {}, expected {}";
    case ErrorType::EXPECTED_BYTE_LITERAL:
        return "Expected byte literal, got {}";
    case ErrorType::UNEXPECTED_END_OF_STRING:
        return "Unexpected end of string literal";
    case ErrorType::INVALID_EXPRESSION:
        return "Invalid expression";
    case ErrorType::WAIT_EXPRESSION:
        return "Wait expression, got {}";
    case ErrorType::EQU_ITEM_DUPLICATION:
        return "Duplication of definition {}";

    }

    return "Unknown error";
}

ErrorMessage::ErrorMessage(ErrorType errorType, int line, int column, std::initializer_list<std::string> args)
    : m_errorType(errorType), m_line(line), m_column(column), m_args(args)
{
    
}

std::string ErrorMessage::format(const std::string& filename) const
{
    std::string templ = errorTemplate(m_errorType);

    switch(m_args.size())
    {
    case 1:
        templ = std::vformat(templ, std::make_format_args(m_args[0]));
        break;
    case 2:
        templ = std::vformat(templ, std::make_format_args(m_args[0], m_args[1]));
        break;
    case 3:
        templ = std::vformat(templ, std::make_format_args(m_args[0], m_args[1], m_args[2]));
        break;
    }
    
    return std::format("{}:{}:{}: error: {}", filename, m_line, m_column, templ);
}

ErrorCollector::ErrorCollector(const std::string& filename, bool immidiate)
    : m_filename(filename), m_immidiate(immidiate)
{
    
}

void ErrorCollector::report(ErrorType errorType, int line, int column, std::initializer_list<std::string> args)
{
    if (m_immidiate)
    {
        ErrorMessage msg = ErrorMessage(errorType, line, column, args);
        std::cout << msg.format(m_filename) << std::endl;
    }
    else
    {
        m_errors.emplace_back(errorType, line, column, args);
    }
}

void ErrorCollector::reportUnexpectedToken(TOKEN_TYPE expected, const Token& token)
{
    std::string gotStr = std::string(Stringify::tokenType(token.type));
    std::string expectedStr = std::string(Stringify::tokenType(expected));
        
    report(
        ErrorType::UNEXPECTED_TOKEN_TYPE,
        token.line,
        token.column,
        { gotStr, expectedStr }
    );
}

void ErrorCollector::reportUnexpectedDirective(DIRECTIVE expected, DIRECTIVE actual, const Token& token)
{
    std::string actualStr = std::string(Stringify::directive(actual));
    std::string expectedStr = std::string(Stringify::directive(expected));
        
    report(
        ErrorType::UNEXPECTED_DIRECTIVE,
        token.line,
        token.column,
        { actualStr, expectedStr }
    );
}

void ErrorCollector::reportUnexpectedOperator(OPERATOR expected, OPERATOR actual, const Token& token)
{
    std::string actualStr = std::string(Stringify::oper(actual));
    std::string expectedStr = std::string(Stringify::oper(expected));
        
    report(
        ErrorType::UNEXPECTED_OPERATOR,
        token.line,
        token.column,
        { actualStr, expectedStr }
    );
}

void ErrorCollector::reportUnrecognizedToken(const Token& token)
{
    std::string tokenStr = std::string(Stringify::tokenValue(token));
    report(
        ErrorType::UNRECOGNIZED_TOKEN,
        token.line,
        token.column,
        { tokenStr }
    );   
}

bool ErrorCollector::hasError()
{
    return m_errors.size() > 0;
}

void ErrorCollector::printAll()
{
    for (const ErrorMessage& e : m_errors)
    {
        std::cerr << e.format(m_filename) << std::endl;
    }
}

void ErrorCollector::clear()
{
    m_errors.clear();
}
