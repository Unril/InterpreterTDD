#include "stdafx.h"
#include "CppUnitTest.h"
#include "Interpreter.h"

namespace InterpreterTests {

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Interpreter;
using namespace std;

TEST_CLASS(LexerTests) {
public:
    TEST_METHOD(Should_return_empty_token_list_when_put_empty_expression) {
        Tokens tokens = Lexer::Tokenize("");
        Assert::IsTrue(tokens.empty());
    }
};

}
