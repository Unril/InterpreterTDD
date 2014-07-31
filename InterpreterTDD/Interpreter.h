#pragma once;
#include <vector>

namespace Interpreter {

struct Token {};

typedef std::vector<Token> Tokens;

namespace Lexer {

inline Tokens Tokenize(std::string expr) {
    return{};
}

} // namespace Lexer
} // namespace Interpreter