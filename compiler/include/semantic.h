#pragma once

#include "error.h"
#include "parser.h"
#include "target.h"

struct InstructionSpec
{
    std::vector<OPERAND_KIND> operands;
};

enum class RESOLVE_STATE
{
    UNVISITED,
    IN_PROGRESS,
    DONE,
};

using FoldedValue = std::variant<int64_t, float>;
using SymbolValue = std::variant<int64_t, float, termino_platform::Address>;

struct Symbol
{
    SymbolValue value;
    const ExpressionNode* expression;
    SYMBOL_KIND kind;
    RESOLVE_STATE state = RESOLVE_STATE::UNVISITED;
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
    void resolveConstants();
    
    std::optional<FoldedValue> resolveSymbol(const std::string& name);
    std::optional<FoldedValue> foldExpression(const ExpressionNode* expression);
    void validateInstruction(CodeStatementNode& stmt);
    void validateDataNode(DataNode& data);
    
    bool tryAddSymbol(std::optional<DeclaredSymbol> declaredSymbol, size_t line, size_t column);

private:
    const ProgramNode& m_root;
    ErrorCollector& m_errorCollector;
    std::unordered_map<OP_CODE, InstructionSpec> m_instructionSpecs;
    std::unordered_map<std::string, Symbol> m_symbols;
};
