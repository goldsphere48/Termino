#include "error.h"

#include <format>
#include <iostream>

std::string TranslateMessageType(ErrorType type)
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
        return "Met unknown directive {}";
    case ErrorType::INVALID_LABEL:
        return "Invalid label {}";
    }

    return "Unknown error";
}

ErrorMessage::ErrorMessage(ErrorType errorType, const std::string& filename, int line, int column, std::initializer_list<std::string> args)
    : m_errorType(errorType), m_line(line), m_column(column), m_args(args), m_filename(filename)
{
    
}

std::string ErrorMessage::format() const
{
    std::string templ = TranslateMessageType(m_errorType);

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
    
    return std::format("{}:{}:{}: error: {}", m_filename, m_line, m_column, templ);
}

ErrorCollector::ErrorCollector(const std::string& filename)
    : m_filename(filename)
{
    
}

void ErrorCollector::report(ErrorType errorType, int line, int column, std::initializer_list<std::string> args)
{
    m_errors.emplace_back(errorType, m_filename, line, column, args);
}

bool ErrorCollector::hasError()
{
    return m_errors.size() > 0;
}

void ErrorCollector::printAll()
{
    for (auto e : m_errors)
    {
        std::cerr << e << std::endl;
    }
}

void ErrorCollector::clear()
{
    m_errors.clear();
}

std::ostream& operator<<(std::ostream& stream, const ErrorMessage& msg)
{
    return stream << msg.format();;
}
