add_executable(compiler
   src/main.cpp
   src/lexer.cpp
   src/error.cpp
   src/isa_helper.cpp
   src/parser.cpp
   src/parser_helper.cpp
)
target_compile_features(compiler PRIVATE cxx_std_20)
