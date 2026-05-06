#pragma once

#include "lexer.h"

#include <memory>
#include <iostream>

/*
GRAMMAR
program ::= (data_section | equ_section | code_section)*

code_section ::= ".code" instruction+
data_section ::= ".data" data_item+
equ_section  ::= ".equ" equ_item+

binary_operator ::= "-" | "+"
unary_operator  ::= "-"

int_literal   ::= UNSIGNED_INT | unary_operator UNSIGNED_INT
float_literal ::= UNSIGNED_FLOAT | unary_operator UNSIGNED_FLOAT
number        ::= int_literal | float_literal
literal       ::= number | STRING_LITERAL

equ_item ::= IDENTIFIER literal

data_item      ::= LABEL_DEF data_directive
data_directive ::= ".bytes" int_literal+ | ".string" STRING_LITERAL

instruction ::= COMMAND operand*
term        ::= number | IDENTIFIER
const_eval_expression ::= term (binary_operator term)*
operand ::= const_eval_expression | term
*/

class ASTNode
{
public:
    virtual ~ASTNode() = default;

    virtual void print() const { };
};

class ExpressionNode : public ASTNode
{
public:
    
};

class NumberNode : public ExpressionNode
{
public:
    std::variant<int, float> value;
};

class BinaryOperationNode : public ExpressionNode
{
public:
    std::unique_ptr<ExpressionNode> left;
    std::unique_ptr<ExpressionNode> right;
    OPERATOR oper;
};

class InstructionNode : public ASTNode
{
public:
    OP_CODE command;
    std::vector<std::unique_ptr<ExpressionNode>> operands;
};

class DataNode : public ASTNode
{
public:
    std::string label;
    std::vector<std::unique_ptr<ExpressionNode>> expressions;
    std::string stringValue;
    DIRECTIVE dataDirective;

    void print() const override
    {
        std::cout << "DATA NODE" << std::endl;
        std::cout << "Label: " << label << " " << "value: ";
        if (dataDirective == DIRECTIVE::BYTE)
        {
            for (const auto& e : expressions)
            {
                e->print();
                std::cout << " ";
            }            
        }
        else if (dataDirective == DIRECTIVE::STRING)
        {
            std::cout << stringValue;
        }
            
        std::cout << std::endl;
    }
};

class DataSectionNode : public ASTNode
{
public:
    std::vector<std::unique_ptr<DataNode>> datas;

    void print() const override
    {
        std::cout << "DATA SECTION" << std::endl;
        for (const auto& node : datas)
        {
            node->print();
        }
    }
};

class CodeSectionNode : public ASTNode
{
public:
    std::vector<InstructionNode> instructions;
};

class EquSectionNode : public ASTNode
{
public:
};

class ProgramNode : public ASTNode
{
public:
    std::vector<std::unique_ptr<CodeSectionNode>> codeNodes;
    std::vector<std::unique_ptr<DataSectionNode>> dataNodes;

    void print() const override
    {
        std::cout << "PROGRAM" << std::endl;
        for (const auto& node : codeNodes)
        {
            node->print();
        }
        
        for (const auto& node : dataNodes)
        {
            node->print();            
        }
    }
};

class Parser
{
public:
    explicit Parser(const std::vector<Token>& tokens, ErrorCollector& collector) noexcept;
    ProgramNode parse();
    
private:
    const Token& peek() const;
    const Token& advance();
    
    std::unique_ptr<CodeSectionNode> parseCodeSection();
    std::unique_ptr<DataSectionNode> parseDataSection();
    std::unique_ptr<EquSectionNode> parseEquSection();
    
    std::unique_ptr<DataNode> parseDataNode();
    std::unique_ptr<ExpressionNode> parseExpression();

    bool check(TOKEN_TYPE type) const;
    bool check(DIRECTIVE type) const;
    bool check(OPERATOR type) const;

    bool match(TOKEN_TYPE type);
    bool match(DIRECTIVE type);
    bool match(OPERATOR type);

    const Token* expect(TOKEN_TYPE type);
    const Token* expect(DIRECTIVE type);
    const Token* expect(OPERATOR type);
    
private:
    ErrorCollector& m_errorCollector;
    const std::vector<Token>& m_tokens;
    std::size_t m_pos;
};
