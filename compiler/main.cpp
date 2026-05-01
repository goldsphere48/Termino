#include "lexer.h"
#include "parser.h"

#include <iostream>
#include <fstream>

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "error: no input file" << std::endl;
        return 1;
    }

    const char* filename = argv[1];
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "error: cannot open file " << filename << std::endl;
        return 1;
    }

    file.seekg(0, std::ios::end);
    std::size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string source(size, '\0');
    file.read(source.data(), size);
    
    ErrorCollector errors(filename);
    Lexer lexer(errors);
    auto tokens = lexer.tokenize(source);

    if (errors.hasError())
    {
        errors.printAll();
        return 1;
    }

    errors.clear();

    for (const Token token : tokens)
    {
        PrintToken(token);
    }

    Parser parser(tokens, errors);
    ProgramNode ast = parser.parse();
    
    if (errors.hasError())
    {
        errors.printAll();
        return 1;
    }

    ast.print();
}
