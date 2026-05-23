#pragma once

#include "parser.h"
#include "target.h"

struct InstructionSpec
{
    std::vector<OPERAND_KIND> operands;
};

struct Symbol
{
    std::variant<int8_t, int16_t, float, termino_platform::Address> value;
    SYMBOL_KIND kind;
    size_t line = 0;
    size_t column = 0;
};

class SemanticAnalyzer
{
public:
    SemanticAnalyzer(const ProgramNode& root, ErrorCollector& errorCollector);

    void analyze();
    void printDeclaryedSymbols() const;

private:
    void collectSymbols();
    void resolveAndValidate();

    std::optional<std::variant<int, float>> foldExpression() const;
    
    bool tryAddSymbol(std::optional<DeclaredSymbol> declaredSymbol, size_t line, size_t column);

private:
    const ProgramNode& m_root;
    ErrorCollector& m_errorCollector;
    std::unordered_map<OP_CODE, InstructionSpec> m_instructionSpecs;
    std::unordered_map<std::string, Symbol> m_symbols;
};
