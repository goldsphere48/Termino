#include "error.h"

#include "isa_helper.h"
#include "lexer.h"

#include <format>
#include <iostream>

std::string errorTemplate(ERROR_TYPE type)
{
    switch(type)
    {
    case ERROR_TYPE::UNKNOWN_INSTRUCTION:
        return "Unknown instruction {}";
    case ERROR_TYPE::INTEGER_LITERAL_OVERFLOW:
        return "Integer literal {} is out of range";
    case ERROR_TYPE::FLOAT_LITERAL_OVERFLOW:
        return "Floating literal {} is out of range";
    case ERROR_TYPE::UNKNOWN_DIRECTIVE:
        return "Unknown directive {}";
    case ERROR_TYPE::INVALID_LABEL:
        return "Invalid label {}";
    case ERROR_TYPE::UNEXPECTED_TOKEN_TYPE:
        return "Unexpected token type {}, expected {}";
    case ERROR_TYPE::EXPECTED_DATA_DIRECTIVE:
        return "Expected data directive, got {}";
    case ERROR_TYPE::BYTE_LITERAL_OVERFLOW:
        return "Byte literal overflow {}";
    case ERROR_TYPE::UNRECOGNIZED_TOKEN:
        return "Unrecognized token {}";
    case ERROR_TYPE::UNEXPECTED_DIRECTIVE:
        return "Unexpected directive {}, expected {}";
    case ERROR_TYPE::UNEXPECTED_OPERATOR:
        return "Unexpected operator {}, expected {}";
    case ERROR_TYPE::EXPECTED_BYTE_LITERAL:
        return "Expected byte literal, got {}";
    case ERROR_TYPE::UNEXPECTED_END_OF_STRING:
        return "Unexpected end of string literal";
    case ERROR_TYPE::INVALID_EXPRESSION:
        return "Invalid expression";
    case ERROR_TYPE::WAIT_EXPRESSION:
        return "Wait expression, got {}";
    case ERROR_TYPE::EQU_ITEM_DUPLICATION:
        return "Duplication of definition {}";
    case ERROR_TYPE::SYMBOL_REDEFINITION:
        return "Redefinition of symbol '{}'";
    case ERROR_TYPE::UNDEFINED_SYMBOL:
        return "Undefined symbol '{}'";
    case ERROR_TYPE::WRONG_OPERAND_COUNT:
        return "Instruction '{}' expects {} operand(s), got {}";
    case ERROR_TYPE::WRONG_OPERAND_TYPE:
        return "Instruction '{}' expects operand of type {}, got {}";
    case ERROR_TYPE::VALUE_OUT_OF_RANGE:
        return "Value {} is out of range for {}";
    case ERROR_TYPE::DIVISION_BY_ZERO:
        return "Division by zero in constant expression";
    case ERROR_TYPE::NON_CONSTANT_EXPRESSION:
        return "Expression must be constant, but references '{}'";
    case ERROR_TYPE::CYCLED_DEPENDECIE:
        return "Cycled dependencie";
    }

    return "Unknown error";
}

ErrorMessage::ErrorMessage(ERROR_TYPE errorType, int line, int column, std::initializer_list<std::string> args)
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

void ErrorCollector::report(ERROR_TYPE errorType, int line, int column, std::initializer_list<std::string> args)
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
        ERROR_TYPE::UNEXPECTED_TOKEN_TYPE,
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
        ERROR_TYPE::UNEXPECTED_DIRECTIVE,
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
        ERROR_TYPE::UNEXPECTED_OPERATOR,
        token.line,
        token.column,
        { actualStr, expectedStr }
    );
}

void ErrorCollector::reportUnrecognizedToken(const Token& token)
{
    std::string tokenStr = std::string(Stringify::tokenValue(token));
    report(
        ERROR_TYPE::UNRECOGNIZED_TOKEN,
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
