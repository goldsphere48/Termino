#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <initializer_list>
#include <ostream>

enum class ErrorType
{
    UNKNOWN_INSTRUCTION = 1000,
    INTEGER_LITERAL_OVERFLOW,
    FLOAT_LITERAL_OVERFLOW,
    UNKNOWN_DIRECTIVE,
    INVALID_LABEL,
};

class ErrorMessage
{
public:
    ErrorMessage(ErrorType errorType, const std::string& filename, int line, int column, std::initializer_list<std::string> args = {});

    std::string format() const;
private:
    ErrorType                m_errorType;
    int                      m_line;
    int                      m_column;
    std::vector<std::string> m_args;
    std::string              m_filename;

    friend std::ostream& operator<<(std::ostream& stream, const ErrorMessage& msg);
};

class ErrorCollector
{
public:
    ErrorCollector(const std::string& filename);
    
    void report(ErrorType errorType, int line, int column, std::initializer_list<std::string> args = {});
    bool hasError();
    void printAll();
    void clear();
private:
    std::vector<ErrorMessage> m_errors;
    std::string m_filename;
};

std::ostream& operator<<(std::ostream& stream, const ErrorMessage& msg);
