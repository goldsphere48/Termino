#pragma once

#include <vector>
#include <string>
#include <initializer_list>

#include "isa.h"

enum class ERROR_TYPE
{
    UNKNOWN_INSTRUCTION = 1000,
    INTEGER_LITERAL_OVERFLOW,
    FLOAT_LITERAL_OVERFLOW,
    UNKNOWN_DIRECTIVE,
    INVALID_LABEL,
    UNEXPECTED_TOKEN_TYPE,
    UNEXPECTED_DIRECTIVE,
    UNEXPECTED_OPERATOR,
    EXPECTED_DATA_DIRECTIVE,
    BYTE_LITERAL_OVERFLOW,
    EXPECTED_BYTE_LITERAL,
    UNRECOGNIZED_TOKEN,
    UNEXPECTED_END_OF_STRING,
    INVALID_EXPRESSION,
    EQU_ITEM_DUPLICATION,
    WAIT_EXPRESSION,
    SYMBOL_REDEFINITION,
    UNDEFINED_SYMBOL,
    WRONG_OPERAND_COUNT,
    WRONG_OPERAND_TYPE,
    VALUE_OUT_OF_RANGE,
    DIVISION_BY_ZERO,
    NON_CONSTANT_EXPRESSION,
    CYCLED_DEPENDECIE,
};

class ErrorMessage
{
public:
    ErrorMessage(ERROR_TYPE errorType, int line, int column, std::initializer_list<std::string> args = {});

    std::string format(const std::string& filename) const;
private:
    ERROR_TYPE                m_errorType;
    int                      m_line;
    int                      m_column;
    std::vector<std::string> m_args;
};

enum class TOKEN_TYPE;
struct Token;

class ErrorCollector
{
public:
    ErrorCollector(const std::string& filename, bool immidiate = false);

    void reportUnexpectedToken(TOKEN_TYPE expected, const Token& token);
    void reportUnexpectedDirective(DIRECTIVE expected, DIRECTIVE actual, const Token& token);
    void reportUnexpectedOperator(OPERATOR expected, OPERATOR actual, const Token& token);
    void reportUnrecognizedToken(const Token& token);

    void report(ERROR_TYPE errorType, int line, int column, std::initializer_list<std::string> args = {});
    bool hasError();
    void printAll();
    void clear();
private:
    std::vector<ErrorMessage> m_errors;
    std::string m_filename;
    bool m_immidiate;
};
