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

static const Token plus(Operator::Plus), minus(Operator::Minus);
static const Token mul(Operator::Mul), div(Operator::Div);
static const Token pLeft(Operator::LParen), pRight(Operator::RParen);
static const Token _1(1), _2(2), _3(3), _4(4), _5(5);

TEST_CLASS(LexerTests) {
public:
    TEST_METHOD(Should_return_empty_token_list_when_put_empty_expression) {
        Tokens tokens = Lexer::Tokenize(L"");
        Assert::IsTrue(tokens.empty());
    }

    TEST_METHOD(Should_tokenize_single_plus_operator) {
        Tokens tokens = Lexer::Tokenize(L"+");
        AssertRange::AreEqual({ plus }, tokens);
    }

    TEST_METHOD(Should_tokenize_single_digit) {
        Tokens tokens = Lexer::Tokenize(L"1");
        AssertRange::AreEqual({ _1 }, tokens);
    }

    TEST_METHOD(Should_tokenize_floating_point_number) {
        Tokens tokens = Lexer::Tokenize(L"12.34");
        AssertRange::AreEqual({ 12.34 }, tokens);
    }

    TEST_METHOD(Should_tokenize_plus_and_number) {
        Tokens tokens = Lexer::Tokenize(L"+12.34");
        AssertRange::AreEqual({ plus, Token(12.34) }, tokens);
    }

    TEST_METHOD(Should_skip_spaces) {
        Tokens tokens = Lexer::Tokenize(L" 1 +  12.34  ");
        AssertRange::AreEqual({ _1, plus, Token(12.34) }, tokens);
    }

    TEST_METHOD(Should_tokenize_complex_experssion) {
        Tokens tokens = Lexer::Tokenize(L"1+2*3/(4-5)");
        AssertRange::AreEqual({ _1, plus, _2, mul, _3, div, pLeft, _4, minus, _5, pRight }, tokens);
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
        Tokens tokens = Parser::Parse({ _1 });
        AssertRange::AreEqual({ _1 }, tokens);
    }

    TEST_METHOD(Should_parse_num_plus_num) {
        Tokens tokens = Parser::Parse({ _1, plus, _2 });
        AssertRange::AreEqual({ _1, _2, plus }, tokens);
    }

    TEST_METHOD(Should_parse_two_additions) {
        Tokens tokens = Parser::Parse({ _1, plus, _2, plus, _3 });
        AssertRange::AreEqual({ _1, _2, plus, _3, plus }, tokens);
    }

    TEST_METHOD(Should_get_same_precedence_for_operator_pairs) {
        Assert::AreEqual(Parser::PrecedenceOf(Operator::Plus), Parser::PrecedenceOf(Operator::Minus));
        Assert::AreEqual(Parser::PrecedenceOf(Operator::Mul), Parser::PrecedenceOf(Operator::Div));
    }

    TEST_METHOD(Should_get_greater_precedence_for_multiplicative_operators) {
        Assert::IsTrue(Parser::PrecedenceOf(Operator::Mul) > Parser::PrecedenceOf(Operator::Plus));
    }

    TEST_METHOD(Should_parse_add_and_mul) {
        Tokens tokens = Parser::Parse({ _1, plus, _2, mul, _3 });
        AssertRange::AreEqual({ _1, _2, _3, mul, plus }, tokens);
    }

    TEST_METHOD(Should_parse_complex_experssion) {
        Tokens tokens = Parser::Parse({ _1, plus, _2, div, _3, minus, _4, mul, _5 });
        AssertRange::AreEqual({ _1, _2, _3, div, plus, _4, _5, mul, minus }, tokens);
    }

    TEST_METHOD(Should_skip_parens_around_number) {
        Tokens tokens = Parser::Parse({ pLeft, _1, pRight });
        AssertRange::AreEqual({ _1 }, tokens);
    }

    TEST_METHOD(Should_parse_expression_with_parens_in_beginning) {
        Tokens tokens = Parser::Parse({ pLeft, _1, plus, _2, pRight, mul, _3 });
        AssertRange::AreEqual({ _1, _2, plus, _3, mul }, tokens);
    }

    TEST_METHOD(Should_throw_when_opening_paren_not_found) {
        Assert::ExpectException<std::logic_error>([]() {Parser::Parse({ _1, pRight }); });
    }

    TEST_METHOD(Should_throw_when_closing_paren_not_found) {
        Assert::ExpectException<std::logic_error>([]() {Parser::Parse({ pLeft, _1 }); });
    }

    TEST_METHOD(Should_parse_complex_experssion_with_paren) {
        // (1+2)*(3/(4-5)) = 1 2 + 3 4 5 - / *
        Tokens tokens = Parser::Parse({ pLeft, _1, plus, _2, pRight, mul, pLeft, _3, div, pLeft, _4, minus, _5, pRight, pRight });
        AssertRange::AreEqual({ _1, _2, plus, _3, _4, _5, minus, div, mul }, tokens);
    }
};

TEST_CLASS(EvaluatorTests) {
public:
    TEST_METHOD(Should_return_zero_when_evaluete_empty_list) {
        double result = Evaluator::Evaluate({});
        Assert::AreEqual(0.0, result);
    }

    TEST_METHOD(Should_return_number_when_evaluete_list_with_number) {
        double result = Evaluator::Evaluate({ _1 });
        Assert::AreEqual(1.0, result);
    }

    TEST_METHOD(Should_eval_expression_with_one_operator) {
        double result = Evaluator::Evaluate({ _1, _2, plus });
        Assert::AreEqual(3.0, result);
    }

    TEST_METHOD(Should_eval_expression_with_one_multiplication) {
        double result = Evaluator::Evaluate({ _2, _3, mul });
        Assert::AreEqual(6.0, result);
    }
};

}
