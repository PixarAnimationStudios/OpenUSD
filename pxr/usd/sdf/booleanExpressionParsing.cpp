//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/sdf/booleanExpressionParsing.h"

#include "pxr/usd/sdf/textFileFormatParser.h"

#include "pxr/base/pegtl/pegtl.hpp"

#include <sstream>
#include <variant>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

namespace PEGTL_NS = PXR_PEGTL_NAMESPACE;

// Builder /////////////////////////////////////////////////////////////////////

using UnaryOperator = SdfBooleanExpression::UnaryOperator;
using BinaryOperator = SdfBooleanExpression::BinaryOperator;

// Helper for building an expression tree as it is parsed
class ExpressionBuilder {
public:
    ExpressionBuilder() {
        _stacks.emplace_back();
    }

    void OpenGroup() {
        _stacks.emplace_back();
    }

    void CloseGroup() {
        auto inner = _stacks.back().Finish();
        _stacks.pop_back();
        _stacks.back().PushExpr(std::move(inner));
    }

    void PushBinaryOp(BinaryOperator op) {
        _stacks.back().PushOperator(op);
    }

    void PushUnaryOp(UnaryOperator op) {
        _stacks.back().PushOperator(op);
    }

    void PushVariable(std::string const& str) {
        TfToken name{str};
        _stacks.back().PushExpr(SdfBooleanExpression::MakeVariable(name));
    }

    void PushConstant(VtValue const& value) {
        _stacks.back().PushExpr(SdfBooleanExpression::MakeConstant(value));
    }

    SdfBooleanExpression Finish() {
        auto result = _stacks.back().Finish();
        _stacks.clear();
        return result;
    }

private:
    struct _Stack {
        void PushExpr(SdfBooleanExpression expression) {
            _expressions.push_back(std::move(expression));
        }

        void PushOperator(BinaryOperator op) {
            _PushOperator(op);
        }

        void PushOperator(UnaryOperator op) {
            _PushOperator(op);
        }

        SdfBooleanExpression Finish() {
            while (!_ops.empty()) {
                _Reduce();
            }

            auto result = std::move(_expressions.back());
            _expressions.clear();
            return result;
        }

    private:
        struct _Operator {
            _Operator(UnaryOperator op) : _op(op) {}
            _Operator(BinaryOperator op) : _op(op) {}

            bool IsUnary() const {
                return std::holds_alternative<UnaryOperator>(_op);
            }

            int Precedence() const {
                if (IsUnary()) {
                    // Currently only one unary op
                    return 0;
                }

                switch (std::get<BinaryOperator>(_op)) {
                    case BinaryOperator::LessThan:
                    case BinaryOperator::LessThanOrEqualTo:
                    case BinaryOperator::GreaterThan:
                    case BinaryOperator::GreaterThanOrEqualTo:
                        return 1;
                    case BinaryOperator::EqualTo:
                    case BinaryOperator::NotEqualTo:
                        return 2;
                    case BinaryOperator::And:
                        return 3;
                    case BinaryOperator::Or:
                        return 4;
                    default:
                        // should not be reachable
                        return 0;
                }
            }

            bool operator<(_Operator const& rhs) const {
                return Precedence() < rhs.Precedence();
            }

            SdfBooleanExpression
            MakeExpression(SdfBooleanExpression lhs,
                           SdfBooleanExpression rhs) const {
                return SdfBooleanExpression::MakeBinaryOp(std::move(lhs),
                    std::get<BinaryOperator>(_op), std::move(rhs));
            }

            SdfBooleanExpression
            MakeExpression(SdfBooleanExpression rhs) const {
                return SdfBooleanExpression::MakeUnaryOp(std::move(rhs),
                std::get<UnaryOperator>(_op));
            }

        private:
            std::variant<UnaryOperator, BinaryOperator> _op;
        };

        void _PushOperator(_Operator op) {
            auto higherPrec = [](_Operator lhs, _Operator rhs) {
                return lhs < rhs;
            };

            while (!_ops.empty() && higherPrec(_ops.back(), op)) {
                _Reduce();
            }

            _ops.push_back(op);
        }

        void _Reduce() {
            auto op = _ops.back();
            _ops.pop_back();
            auto rhs = std::move(_expressions.back());
            _expressions.pop_back();

            if (op.IsUnary()) {
                _expressions.push_back(op.MakeExpression(std::move(rhs)));
            } else {
                auto lhs = std::move(_expressions.back());
                _expressions.pop_back();
                _expressions.push_back(op.MakeExpression(std::move(lhs),
                    std::move(rhs)));
            }
        }

        std::vector<SdfBooleanExpression> _expressions;
        std::vector<_Operator> _ops;
    };

    std::vector<_Stack> _stacks;
};

// Rules ///////////////////////////////////////////////////////////////////////

struct Expression;

struct Space : Sdf_TextFileFormatParser::Space {};

// Parenthesized expression
struct LeftParen : PEGTL_NS::one<'('> {};
struct RightParen : PEGTL_NS::one<')'> {};
struct ParenExpression : PEGTL_NS::if_must<
    LeftParen, PEGTL_NS::pad<Expression, Space>, RightParen> {};

// Constant (number, string, boolean)
struct Number : Sdf_TextFileFormatParser::Number {};
struct String : PEGTL_NS::sor<
    Sdf_TextFileFormatParser::SinglelineSingleQuoteString,
    Sdf_TextFileFormatParser::SinglelineDoubleQuoteString> {};
struct True : PXR_PEGTL_KEYWORD("true") {};
struct False : PXR_PEGTL_KEYWORD("false") {};
struct BooleanLiteral : PEGTL_NS::sor<True, False> {};
struct Constant : PEGTL_NS::sor<Number, String, BooleanLiteral> {};

// Atom (Constant, Variable, or Parenthesized expression)
struct Variable : Sdf_TextFileFormatParser::NamespacedIdentifier {};
struct Atom : PEGTL_NS::sor<Constant, Variable, ParenExpression> {};

// Operators
struct EqualTo : PXR_PEGTL_STRING("==") {};
struct NotEqualTo : PXR_PEGTL_STRING("!=") {};
struct LessThan : PEGTL_NS::one<'<'> {};
struct LessThanOrEqualTo : PXR_PEGTL_STRING("<=") {};
struct GreaterThan : PEGTL_NS::one<'>'> {};
struct GreaterThanOrEqualTo : PXR_PEGTL_STRING(">=") {};
struct And : PXR_PEGTL_STRING("&&") {};
struct Or : PXR_PEGTL_STRING("||") {};
struct Not : PEGTL_NS::one<'!'> {};
struct InfixOperator : PEGTL_NS::sor<
    EqualTo, NotEqualTo,
    LessThanOrEqualTo, LessThan,
    GreaterThanOrEqualTo, GreaterThan,
    And, Or> {};

struct PrefixOperator : PEGTL_NS::sor<Not> {};

struct PrefixedAtom :
    PEGTL_NS::if_then_else<PrefixOperator, PrefixedAtom, Atom> {};

// Expression
struct Expression :
    PEGTL_NS::list<PrefixedAtom, PEGTL_NS::pad<InfixOperator, Space>> {};

// A valid expression consumes all available input, including any whiespace
struct GrammarGoal : PEGTL_NS::must<
    PEGTL_NS::pad<Expression, Space>,
    PEGTL_NS::eof> {};

// Actions /////////////////////////////////////////////////////////////////////

template <BinaryOperator op>
struct BinaryOpAction {
    template <class Input>
    static void apply(Input const& in, ExpressionBuilder& builder) {
        builder.PushBinaryOp(op);
    }
};

template <typename Rule>
struct ParseAction {};

template <> struct ParseAction<And> :
    BinaryOpAction<BinaryOperator::And> {};
template <> struct ParseAction<Or> :
    BinaryOpAction<BinaryOperator::Or> {};
template <> struct ParseAction<EqualTo> :
    BinaryOpAction<BinaryOperator::EqualTo> {};
template <> struct ParseAction<NotEqualTo> :
    BinaryOpAction<BinaryOperator::NotEqualTo> {};
template <> struct ParseAction<GreaterThan> :
    BinaryOpAction<BinaryOperator::GreaterThan> {};
template <> struct ParseAction<LessThan> :
    BinaryOpAction<BinaryOperator::LessThan> {};
template <> struct ParseAction<GreaterThanOrEqualTo> :
    BinaryOpAction<BinaryOperator::GreaterThanOrEqualTo> {};
template <> struct ParseAction<LessThanOrEqualTo> :
    BinaryOpAction<BinaryOperator::LessThanOrEqualTo> {};

template <>
struct ParseAction<Variable> {
    template <class Input>
    static void apply(Input const& in, ExpressionBuilder& builder) {
        builder.PushVariable(in.string());
    }
};

template <>
struct ParseAction<Number> {
    template <class Input>
    static void apply(Input const& in, ExpressionBuilder& builder) {
        std::stringstream s(in.string());
        double number;
        s >> number;
        builder.PushConstant(VtValue(number));
    }
};

template <>
struct ParseAction<String> {
    template <class Input>
    static void apply(Input const& in, ExpressionBuilder& builder) {
        // Strip the quotation marks and unescape if needed
        auto quotedString = in.string();
        auto string = Sdf_EvalQuotedString(quotedString.c_str(),
                                           quotedString.length(), 1);
        builder.PushConstant(VtValue(string));
    }
};

template <>
struct ParseAction<True> {
    template <class Input>
    static void apply(Input const& in, ExpressionBuilder& builder) {
        builder.PushConstant(VtValue(true));
    }
};

template <>
struct ParseAction<False> {
    template <class Input>
    static void apply(Input const& in, ExpressionBuilder& builder) {
        builder.PushConstant(VtValue(false));
    }
};

template <>
struct ParseAction<LeftParen> {
    template <class Input>
    static void apply(Input const& in, ExpressionBuilder& builder) {
        builder.OpenGroup();
    }
};

template <>
struct ParseAction<RightParen> {
    template <class Input>
    static void apply(Input const& in, ExpressionBuilder& builder) {
        builder.CloseGroup();
    }
};

template <>
struct ParseAction<Not> {
    template <class Input>
    static void apply(Input const& in, ExpressionBuilder& builder) {
        builder.PushUnaryOp(UnaryOperator::Not);
    }
};


} // anonymous namespace

SdfBooleanExpression
Sdf_ParseBooleanExpression(std::string const& text,
                           std::string* errorMessage)
{
    ExpressionBuilder builder;
    PEGTL_NS::string_input input{text, "<input>"};

    try {
        PEGTL_NS::parse<GrammarGoal, ParseAction>(input, builder);
    } catch (PEGTL_NS::parse_error const& e) {
        if (errorMessage) {
            std::stringstream s;
            for (auto const& p : e.positions()) {
                s << e.what() << "\n";
                s << input.line_at(p) << "\n";
                s << std::setw((int)p.column) << "^\n";
            }

            *errorMessage = s.str();
        }

        return {};
    } catch (std::exception const& e) {
        if (errorMessage) {
            *errorMessage = e.what();
        }

        return {};
    }

    return builder.Finish();
}


bool
Sdf_ValidateBooleanExpression(std::string const& text,
    std::string* errorMessage)
{
    PEGTL_NS::string_input input{text, "<input>"};

    try {
        return PEGTL_NS::parse<GrammarGoal>(input);
    } catch (PEGTL_NS::parse_error const& e) {
        if (errorMessage) {
            std::stringstream s;
            for (auto const& p : e.positions()) {
                s << e.what() << "\n";
                s << input.line_at(p) << "\n";
                s << std::setw((int)p.column) << "^\n";
            }

            *errorMessage = s.str();
        }
    } catch (std::exception const& e) {
        if (errorMessage) {
            *errorMessage = e.what();
        }
    }

    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
