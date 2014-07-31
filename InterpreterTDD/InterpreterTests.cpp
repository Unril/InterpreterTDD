#include "stdafx.h"
#include "CppUnitTest.h"
#include "Interpreter.h"
#include <initializer_list>
#include <algorithm>
#include <string>

#pragma warning(disable: 4505)

namespace InterpreterTests {

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Interpreter;
using namespace std;

namespace AssertRange {

template<class T, class ActualRange>
static void AreEqual(initializer_list<T> expect, const ActualRange &actual) {
    auto actualIter = begin(actual);
    auto expectIter = begin(expect);

    Assert::AreEqual(distance(expectIter, end(expect)), distance(actualIter, end(actual)), L"Size differs.");

    for(; expectIter != end(expect) && actualIter != end(actual); ++expectIter, ++actualIter) {
        auto message = L"Mismatch in position " + to_wstring(distance(begin(expect), expectIter));
        Assert::AreEqual(*expectIter, *actualIter, message.c_str());
    }
}

} // namespace AssertRange

TEST_CLASS(LexerTests) {
public:
    TEST_METHOD(Should_return_empty_token_list_when_put_empty_expression) {
        Tokens tokens = Lexer::Tokenize(L"");
        Assert::IsTrue(tokens.empty());
    }

    TEST_METHOD(Should_tokenize_single_plus_operator) {
        Tokens tokens = Lexer::Tokenize(L"+");
        AssertRange::AreEqual({ Operator::Plus }, tokens);
    }
};

}
