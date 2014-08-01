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
        Assert::AreEqual<T>(*expectIter, *actualIter, message.c_str());
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

    TEST_METHOD(Should_tokenize_single_digit) {
        Tokens tokens = Lexer::Tokenize(L"1");
        AssertRange::AreEqual({ 1.0 }, tokens);
    }

    TEST_METHOD(Should_tokenize_floating_point_number) {
        Tokens tokens = Lexer::Tokenize(L"12.34");
        AssertRange::AreEqual({ 12.34 }, tokens);
    }

    TEST_METHOD(Should_tokenize_plus_and_number) {
        Tokens tokens = Lexer::Tokenize(L"+12.34");
        AssertRange::AreEqual({ Token(Operator::Plus), Token(12.34) }, tokens);
    }

    TEST_METHOD(Should_skip_spaces) {
        Tokens tokens = Lexer::Tokenize(L" 1 +  12.34  ");
        AssertRange::AreEqual({ Token(1.0), Token(Operator::Plus), Token(12.34) }, tokens);
    }
};

TEST_CLASS(TokenTests) {
public:
    TEST_METHOD(Should_get_type_for_operator_token) {
        Token opToken(Operator::Plus);
        Assert::AreEqual(TokenType::Operator, opToken.Type());
    }

    TEST_METHOD(Should_get_type_for_number_token) {
        Token numToken(1.2);
        Assert::AreEqual(TokenType::Number, numToken.Type());
    }

    TEST_METHOD(Should_get_operator_code_from_operator_token) {
        Token token(Operator::Plus);
        Assert::AreEqual<Operator>(Operator::Plus, token);
    }

    TEST_METHOD(Should_get_number_value_from_number_token) {
        Token token(1.23);
        Assert::AreEqual<double>(1.23, token);
    }
};

}
