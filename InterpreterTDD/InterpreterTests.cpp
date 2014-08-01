#include "stdafx.h"

#include "CppUnitTest.h"

#include "Interpreter.h"

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

    TEST_METHOD(Should_tokenize_complex_experssion) {
        Tokens tokens = Lexer::Tokenize(L"1+2*3/(4-5)");
        AssertRange::AreEqual({
            Token(1), Token(Operator::Plus), Token(2), Token(Operator::Mul), Token(3), Token(Operator::Div),
            Token(Operator::LParen), Token(4), Token(Operator::Minus), Token(5), Token(Operator::RParen)
        }, tokens);
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

TEST_CLASS(ParserTests) {
public:
    TEST_METHOD(Should_return_empty_list_when_put_empty_list) {
        Tokens tokens = Parser::Parse({});
        Assert::IsTrue(tokens.empty());
    }

    TEST_METHOD(Should_parse_single_number) {
        Tokens tokens = Parser::Parse({ Token(1) });
        AssertRange::AreEqual({ Token(1) }, tokens);
    }

    TEST_METHOD(Should_parse_num_plus_num) {
        Tokens tokens = Parser::Parse({ Token(1), Token(Operator::Plus), Token(2) });
        AssertRange::AreEqual({ Token(1), Token(2), Token(Operator::Plus) }, tokens);
    }

    TEST_METHOD(Should_parse_two_additions) {
        Tokens tokens = Parser::Parse({ Token(1), Token(Operator::Plus), Token(2), Token(Operator::Plus), Token(3) });
        AssertRange::AreEqual({ Token(1), Token(2), Token(Operator::Plus), Token(3), Token(Operator::Plus) }, tokens);
    }

    TEST_METHOD(Should_get_same_precedence_for_operator_pairs) {
        Assert::AreEqual(Parser::PrecedenceOf(Operator::Plus), Parser::PrecedenceOf(Operator::Minus));
        Assert::AreEqual(Parser::PrecedenceOf(Operator::Mul), Parser::PrecedenceOf(Operator::Div));
    }

    TEST_METHOD(Should_get_greater_precedence_for_multiplicative_operators) {
        Assert::IsTrue(Parser::PrecedenceOf(Operator::Mul) > Parser::PrecedenceOf(Operator::Plus));
    }

    TEST_METHOD(Should_parse_add_and_mul) {
        Tokens tokens = Parser::Parse({ Token(1), Token(Operator::Plus), Token(2), Token(Operator::Mul), Token(3) });
        AssertRange::AreEqual({ Token(1), Token(2), Token(3), Token(Operator::Mul), Token(Operator::Plus) }, tokens);
    }

    TEST_METHOD(Should_parse_complex_experssion) {
        // 1 + 2 / 3 - 4 * 5 = 1 2 3 / + 4 5 * -
        Tokens tokens = Parser::Parse({
            Token(1), Token(Operator::Plus), Token(2), Token(Operator::Div), Token(3),
            Token(Operator::Minus), Token(4), Token(Operator::Mul), Token(5)
        });
        AssertRange::AreEqual({
            Token(1), Token(2), Token(3), Token(Operator::Div), Token(Operator::Plus),
            Token(4), Token(5), Token(Operator::Mul), Token(Operator::Minus)
        }, tokens);
    }
};

}
