#pragma once

#include "lexer.h"

#include <memory>
#include <unordered_map>

#include "isa_helper.h"

/*
GRAMMAR
program ::= (data_section | equ_section | code_section)*

code_section ::= ".code" (label_def | instruction)*
data_section ::= ".data" data_item*
equ_section  ::= ".equ"  equ_item*

binary_operator ::= "-" | "+"
unary_operator  ::= "-"

int_literal   ::= UNSIGNED_INT   | unary_operator UNSIGNED_INT
float_literal ::= UNSIGNED_FLOAT | unary_operator UNSIGNED_FLOAT
number        ::= int_literal | float_literal

label_def ::= LABEL_DEF
equ_item  ::= LABEL_DEF expression

data_item      ::= LABEL_DEF data_directive
data_directive ::= (".byte" | ".short" | ".word" | ".dword") expression+
                 | ".string" STRING_LITERAL

instruction ::= COMMAND operand*
term        ::= number | IDENTIFIER
expression  ::= term (binary_operator term)?  // TODO: должно быть `*`, см. TODO.md
operand     ::= expression
*/

enum class ASTNodeType
{
    NUMBER,
    BINARY_OPERATION,
    IDENTIFIER,
    INSTRUCTION,
    LABEL_DEF,
    DATA,
    DATA_SECTION,
    CODE_SECTION,
    EQU_ITEM,
    EQU_SECTION,
    PROGRAM,
};

class ASTNode
{
public:
    virtual ~ASTNode() = default;

    virtual ASTNodeType getType() const = 0;
    virtual void print(int indent) const = 0;

    template<typename T>
    T* as()
    {
        return getType() == T::TYPE ? static_cast<T*>(this) : nullptr;
    }

    size_t line;
    size_t column;
};

class ExpressionNode : public ASTNode
{
public:
    std::optional<OPERATOR> unarOperator;
};

class NumberNode : public ExpressionNode
{
public:
    std::variant<int, float> value;

    static constexpr ASTNodeType TYPE = ASTNodeType::NUMBER;

    ASTNodeType getType() const override { return ASTNodeType::NUMBER; }
    void print(int indent) const override;
};

class BinaryOperationNode : public ExpressionNode
{
public:
    std::unique_ptr<ExpressionNode> left;
    std::unique_ptr<ExpressionNode> right;
    OPERATOR oper;

    static constexpr ASTNodeType TYPE = ASTNodeType::BINARY_OPERATION;

    ASTNodeType getType() const override { return ASTNodeType::BINARY_OPERATION; }
    void print(int indent) const override;
};

class IdentifierNode : public ExpressionNode
{
public:
    std::string identifier;

    static constexpr ASTNodeType TYPE = ASTNodeType::IDENTIFIER;

    ASTNodeType getType() const override { return ASTNodeType::IDENTIFIER; }
    void print(int indent) const override;
};

class CodeStatementNode : public ASTNode
{
public:

};

class InstructionNode : public CodeStatementNode
{
public:
    OP_CODE command;
    std::vector<std::unique_ptr<ExpressionNode>> operands;

    static constexpr ASTNodeType TYPE = ASTNodeType::INSTRUCTION;

    ASTNodeType getType() const override { return ASTNodeType::INSTRUCTION; }
    void print(int indent) const override;
};

class LabelDefNode : public CodeStatementNode
{
public:
    std::string identifier;

    static constexpr ASTNodeType TYPE = ASTNodeType::LABEL_DEF;

    ASTNodeType getType() const override { return ASTNodeType::LABEL_DEF; }
    void print(int indent) const override;
};

class DataNode : public ASTNode
{
public:
    std::string label;
    std::vector<std::unique_ptr<ExpressionNode>> expressions;
    std::string stringValue;
    DIRECTIVE dataDirective;

    static constexpr ASTNodeType TYPE = ASTNodeType::DATA;

    ASTNodeType getType() const override { return ASTNodeType::DATA; }
    void print(int indent) const override;
};

class DataSectionNode : public ASTNode
{
public:
    std::vector<std::unique_ptr<DataNode>> datas;

    static constexpr ASTNodeType TYPE = ASTNodeType::DATA_SECTION;

    ASTNodeType getType() const override { return ASTNodeType::DATA_SECTION; }
    void print(int indent) const override;
};

class CodeSectionNode : public ASTNode
{
public:
    std::vector<std::unique_ptr<CodeStatementNode>> nodes;

    static constexpr ASTNodeType TYPE = ASTNodeType::CODE_SECTION;

    ASTNodeType getType() const override { return ASTNodeType::CODE_SECTION; }
    void print(int indent) const override;
};

class EquItemNode : public ASTNode
{
public:
    std::string identifier;
    std::unique_ptr<ExpressionNode> expression;

    static constexpr ASTNodeType TYPE = ASTNodeType::EQU_ITEM;

    ASTNodeType getType() const override { return ASTNodeType::EQU_ITEM; }
    void print(int indent) const override;
};

class EquSectionNode : public ASTNode
{
public:
    std::vector<std::unique_ptr<EquItemNode>> nodes;

    static constexpr ASTNodeType TYPE = ASTNodeType::EQU_SECTION;

    ASTNodeType getType() const override { return ASTNodeType::EQU_SECTION; }
    void print(int indent) const override;
};

class ProgramNode : public ASTNode
{
public:
    std::vector<std::unique_ptr<ASTNode>> nodes;
    std::unordered_map<std::string, std::unique_ptr<EquItemNode>> equSection;

    static constexpr ASTNodeType TYPE = ASTNodeType::PROGRAM;

    ASTNodeType getType() const override { return ASTNodeType::PROGRAM; }
    void print(int indent = 0) const override;
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
    std::vector<std::unique_ptr<ExpressionNode>> parseExpressionsList();
    std::unique_ptr<ExpressionNode> parseTerm();
    std::unique_ptr<ExpressionNode> parseUnmodifiedTerm();
    std::unique_ptr<EquItemNode> parseEquItem();
    std::unique_ptr<InstructionNode> parseInstruction();
    std::unique_ptr<LabelDefNode> parseLabelDef();

    bool check(TOKEN_TYPE type) const;
    bool check(DIRECTIVE type) const;
    bool check(OPERATOR type) const;

    template<typename T>
    bool match(T type)
    {
        if (check(type))
        {
            advance();
            return true;
        }

        return false;
    }

    const Token* expect(TOKEN_TYPE type);
    const Token* expect(DIRECTIVE type);
    const Token* expect(OPERATOR type);

    void recoverLast();
    void recoverTo(size_t pos);

    bool atSectionBoundary() const;
    
    void syncToInstructionStart();
    void syncToDataNodeStart();
    void syncToEquItemStart();
    
private:
    ErrorCollector& m_errorCollector;
    const std::vector<Token>& m_tokens;
    std::size_t m_pos;
};
