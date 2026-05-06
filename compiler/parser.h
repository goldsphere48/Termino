#pragma once

#include "lexer.h"

#include <memory>
#include <iostream>
#include <unordered_map>

#include "isa_helper.h"

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

    virtual void print(int indent) const { };

    size_t line;
    size_t column;
};

class ExpressionNode : public ASTNode
{
public:
    
};

class NumberNode : public ExpressionNode
{
public:
    std::variant<int, float> value;

    void print(int indent) const override
    {
        std::visit([](auto&& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, int>)
            {
                std::cout << '[' << v << ']';
            }
            if constexpr (std::is_same_v<T, float>)
            {
                std::cout << '[' << v << ']';
            }
        }, value);
    }
};

class BinaryOperationNode : public ExpressionNode
{
public:
    std::unique_ptr<ExpressionNode> left;
    std::unique_ptr<ExpressionNode> right;
    OPERATOR oper;

    void print(int indent) const override
    {
        std::cout << '[';
        left->print(indent);
        std::cout << " " << Stringify::oper(oper) << " ";
        right->print(indent);
        std::cout << ']';
    }
};

class IdentifierNode : public ExpressionNode
{
public:
    std::string identifier;

    void print(int indent) const override
    {
        std::cout << '[' << identifier << ']';
    }
};

class InstructionNode : public ASTNode
{
public:
    OP_CODE command;
    std::vector<std::unique_ptr<ExpressionNode>> operands;

    void print(int indent) const override
    {
        std::cout << "INSTRUCTION:" << std::endl;
        std::cout << Stringify::opCode(command) << " ";
        for (const auto& operand : operands)
        {
            operand->print(indent);
        }
        std::cout << std::endl;
    }
};

class DataNode : public ASTNode
{
public:
    std::string label;
    std::vector<std::unique_ptr<ExpressionNode>> expressions;
    std::string stringValue;
    DIRECTIVE dataDirective;

    void print(int indent) const override
    {
        std::cout << "DATA NODE" << std::endl;
        std::cout << "Label: " << label << " " << "value: " << std::endl;

        indent++;
        
        if (dataDirective == DIRECTIVE::BYTE)
        {
            for (const auto& e : expressions)
            {
                e->print(indent);
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

    void print(int indent) const override
    {
        std::cout << "DATA SECTION" << std::endl;
        for (const auto& node : datas)
        {
            node->print(indent);
        }
    }
};

class CodeSectionNode : public ASTNode
{
public:
    std::vector<std::unique_ptr<InstructionNode>> nodes;

    void print(int indent) const override
    {
        std::cout << "CODE SECTION: " << std::endl;
        for (const auto& instruction : nodes)
        {
            instruction->print(indent);
        }
    }
};

class EquItemNode : public ASTNode
{
public:
    std::string identifier;
    std::unique_ptr<ExpressionNode> expression;

    void print(int indent) const override
    {
        std::cout << identifier << " ";
        expression->print(indent);
        std::cout << std::endl;
    }
};

class EquSectionNode : public ASTNode
{
public:
    std::vector<std::unique_ptr<EquItemNode>> nodes;
};

class ProgramNode : public ASTNode
{
public:
    std::vector<std::unique_ptr<CodeSectionNode>> codeNodes;
    std::vector<std::unique_ptr<DataSectionNode>> dataNodes;
    std::unordered_map<std::string, std::unique_ptr<EquItemNode>> equSection; 

    void print(int indent = 0) const override
    {
        std::cout << "PROGRAM" << std::endl;
        indent++;
        for (const auto& node : codeNodes)
        {
            node->print(indent);
        }
        
        for (const auto& node : dataNodes)
        {
            node->print(indent);
        }

        for (const auto& node : codeNodes)
        {
            node->print(indent);
        }
        
        for (const auto& [name, value] : equSection)
        {
            std::cout << "EQU SECTION ITEM" << std::endl;
            std::cout << "Identifier: " << name << " value: ";
            value->print(indent);
            std::cout << std::endl;
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
    std::unique_ptr<ExpressionNode> parseTerm();
    std::unique_ptr<EquItemNode> parseEquItem();
    std::unique_ptr<InstructionNode> parseInstruction();

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
