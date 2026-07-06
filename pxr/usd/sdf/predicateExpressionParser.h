//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_SDF_PREDICATE_EXPRESSION_PARSER_H
#define PXR_USD_SDF_PREDICATE_EXPRESSION_PARSER_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/pegtl/pegtl.hpp"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

// fwd decl, from parserHelpers.cpp.
std::string
Sdf_EvalQuotedString(const char* x, size_t n,
                     size_t trimBothSides, unsigned int* numLines=NULL);

struct SdfPredicateExprBuilder
{
    SdfPredicateExprBuilder() { OpenGroup(); }
    
    void PushOp(SdfPredicateExpression::Op op) { _stacks.back().PushOp(op); }

    void PushCall(SdfPredicateExpression::FnCall::Kind kind) {
        _stacks.back().PushCall(
            kind, std::move(_funcName), std::move(_funcArgs));
        _funcName.clear();
        _funcArgs.clear();
    }

    void SetFuncName(std::string const &name) {
        _funcName = name;
    }
    
    void AddFuncArg(VtValue const &val) {
        _funcArgs.push_back({ std::move(_funcKwArgName), val });
        _funcKwArgName.clear();
    }

    void SetFuncArgKWName(std::string const &kw) {
        _funcKwArgName = kw;
    }

    void OpenGroup() { _stacks.emplace_back(); }

    void CloseGroup() {
        SdfPredicateExpression innerExpr = _stacks.back().Finish();
        _stacks.pop_back();
        _stacks.back().PushExpr(std::move(innerExpr));
    }

    SdfPredicateExpression Finish() {
        SdfPredicateExpression result = _stacks.back().Finish();
        _stacks.clear();
        _funcArgs.clear();
        _funcName.clear();
        return result;
    }

private:
    struct _Stack {

        void PushOp(SdfPredicateExpression::Op op) {
            using Op = SdfPredicateExpression::Op;
            auto higherPrec = [](Op left, Op right) {
                return (left < right) || (left == right && left != Op::Not);
            };            
            // Reduce while prior ops have higher precedence.
            while (!opStack.empty() && higherPrec(opStack.back(), op)) {
                _Reduce();
            }
            opStack.push_back(op);
        }

        void PushCall(SdfPredicateExpression::FnCall::Kind kind,
                      std::string &&name,
                      std::vector<SdfPredicateExpression::FnArg> &&args) {
            exprStack.push_back(
                SdfPredicateExpression::MakeCall({
                        kind, std::move(name), std::move(args) }));
        }

        void PushExpr(SdfPredicateExpression &&expr) {
            exprStack.push_back(std::move(expr));
        }

        SdfPredicateExpression Finish() {
            while (!opStack.empty()) {
                _Reduce();
            }
            SdfPredicateExpression ret = std::move(exprStack.back());
            exprStack.clear();
            return ret;
        }

    private:
        void _Reduce() {
            SdfPredicateExpression::Op op = opStack.back();
            opStack.pop_back();
            SdfPredicateExpression right = std::move(exprStack.back());
            exprStack.pop_back();

            if (op == SdfPredicateExpression::Not) {
                // Not is the only unary op.
                exprStack.push_back(
                    SdfPredicateExpression::MakeNot(std::move(right)));
            }
            else {
                // All other ops are binary.
                SdfPredicateExpression left = std::move(exprStack.back());
                exprStack.pop_back();
                exprStack.push_back(
                    SdfPredicateExpression::MakeOp(
                        op, std::move(left), std::move(right))
                    );
            }
        }                
        
        // Working space.
        std::vector<SdfPredicateExpression::Op> opStack;
        std::vector<SdfPredicateExpression> exprStack;
    };

    std::vector<_Stack> _stacks;

    std::string _funcName;
    std::string _funcKwArgName;
    std::vector<SdfPredicateExpression::FnArg> _funcArgs;
};



////////////////////////////////////////////////////////////////////////
// Grammar.

namespace SdfPredicateExpressionParser {

using namespace PXR_PEGTL_NAMESPACE;

template <class Rule> using OptSpaced = pad<Rule, blank>;

using OptSpacedComma = OptSpaced<one<','>>;

////////////////////////////////////////////////////////////////////////
// Predicate expression grammar.

struct NotKW  : PXR_PEGTL_KEYWORD("not") {};
struct AndKW  : PXR_PEGTL_KEYWORD("and") {};
struct OrKW   : PXR_PEGTL_KEYWORD("or") {};
struct Inf    : PXR_PEGTL_KEYWORD("inf") {};
struct True   : PXR_PEGTL_KEYWORD("true") {};
struct False  : PXR_PEGTL_KEYWORD("false") {};
struct ImpliedAnd : plus<blank> {};

struct ReservedWord : sor<
    NotKW, AndKW, OrKW, Inf, True, False> {};

struct Digits : plus<range<'0','9'>> {};

struct Exp  : seq<one<'e','E'>, opt<one<'-','+'>>, must<Digits>> {};
struct Frac : if_must<one<'.'>, Digits> {};
struct PredArgFloat : seq<
    opt<one<'-'>>, sor<Inf, seq<Digits, if_then_else<Frac, opt<Exp>, Exp>>>
    > {};
struct PredArgInt : seq<opt<one<'-'>>, Digits> {};

struct PredArgBool : sor<True, False> {};

template <class Quote>
struct Escaped : sor<Quote, one<'\\', 'b', 'f', 'n', 'r', 't'>> {};
template <class Quote>
struct Unescaped : minus<utf8::range<0x20, 0x10FFFF>, Quote> {};

template <class Quote>
struct StringChar : if_then_else<
    one<'\\'>, must<Escaped<Quote>>, Unescaped<Quote>> {};

struct QuotedString : sor<
    if_must<one<'"'>,  until<one<'"'>,  StringChar<one<'"'>>>>,
    if_must<one<'\''>, until<one<'\''>, StringChar<one<'\''>>>>
    > {};

struct UnquotedStringChar
    : sor<identifier_other,
          one<'~', '!', '@', '#', '$', '%', '^', '&', '*', '-', '+', '=',
              '|', '\\', '.', '?', '/'>> {};

struct UnquotedString : star<UnquotedStringChar> {};

struct PredArgString : sor<QuotedString, UnquotedString> {};

struct PredArgVal : sor<
    PredArgFloat, PredArgInt, PredArgBool, PredArgString> {};

struct PredKWArgName : minus<identifier, ReservedWord> {};

struct PredKWArgPrefix : seq<PredKWArgName, OptSpaced<one<'='>>> {};
struct PredKWArg : if_must<PredKWArgPrefix, PredArgVal> {};

struct PredParenPosArg : seq<not_at<PredKWArgPrefix>, PredArgVal> {};

struct PredFuncName : minus<identifier, ReservedWord> {};

// Within keyword args, ',' unambiguously requires another keyword arg.
struct PredKWArgStep : if_must<OptSpacedComma, PredKWArg> {};
struct PredKWArgs : seq<PredKWArg, star<PredKWArgStep>> {};

// The positional-to-keyword transition uses lookahead in the positional list
// because a ',' after a positional arg could precede either another positional
// arg or a keyword arg -- we can't commit to PredParenPosArg specifically.
// Once all positional args are consumed, a ',' commits to requiring kwargs.
struct PredParenArgs
    : if_then_else<list<PredParenPosArg, OptSpacedComma>,
                   opt<if_must<OptSpacedComma, PredKWArgs>>,
                   opt<PredKWArgs>>
{};

struct PredColonArgs;
struct PredColonArgStep : if_must<one<','>, PredArgVal> {};
struct PredColonArgs : seq<PredArgVal, star<PredColonArgStep>> {};

struct PredColonCall : if_must<seq<PredFuncName, one<':'>>, PredColonArgs> {};

// Named for error messaging.
struct PredParenClose : one<')'> {};
struct PredParenCall : seq<
    PredFuncName, OptSpaced<one<'('>>,
    must<PredParenArgs, star<blank>, PredParenClose>
    >
{};

struct PredBareCall : PredFuncName {};

struct PredExpr;

struct PredOpenGroup  : one<'('> {};
struct PredCloseGroup : one<')'> {};

// Named wrapper so the group's inner expression gets a targeted error message.
struct GroupedPredExpr : OptSpaced<PredExpr> {};

struct PredAtom
    : sor<
    PredColonCall,
    PredParenCall,
    PredBareCall,
    if_must<PredOpenGroup, GroupedPredExpr, PredCloseGroup>
    >
{};

struct PredFactor : seq<opt<OptSpaced<list<NotKW, plus<blank>>>>, PredAtom> {};

// 'and'/'or' are unambiguous operators -- commit to requiring another factor.
// Implied and (whitespace) needs a lookahead because trailing whitespace is
// valid and must not commit to requiring another factor.
// Note: the lookahead parses ImpliedAnd+PredFactor twice; acceptable given
// the bounded syntax involved.
struct PredExplicitOp : sor<OptSpaced<AndKW>, OptSpaced<OrKW>> {};
struct PredExprStep : if_must<PredExplicitOp, PredFactor> {};
struct ImpliedAndStep : seq<
    at<seq<ImpliedAnd, PredFactor>>, ImpliedAnd, PredFactor> {};

struct PredExpr : seq<PredFactor, star<sor<PredExprStep, ImpliedAndStep>>> {};

// Wrap eolf so we can attach a targeted error message without specializing
// the PEGTL built-in directly.
struct PredExprEnd : eolf {};

////////////////////////////////////////////////////////////////////////
// Errors.
//
// This is the root of the Errors inheritance chain used across the family
// of grammars that embed predicate expressions:
//
//   SdfPredicateExpressionParser::Errors  (predicate rules, this file)
//     <- SdfPathPatternParser::Errors     (path pattern rules)
//       <- pathExpression.cpp Errors      (path expression rules)
//
// Each layer in the chain handles only its own grammar's rules; errors from
// embedded grammars propagate upward automatically via inheritance.
//
// inline on static member definitions avoids ODR violations since this
// header is included in multiple translation units.

template <class Rule>
struct Errors : public normal<Rule>
{
    static char const *message;

    template <class Input, class... States>
    [[noreturn]] static void raise(Input const &in, States &&...) {
        throw parse_error(message, in);
    }
};

template <class Rule>
inline char const *Errors<Rule>::message = "";

#define PARSE_ERROR(rule, msg) \
    template <> inline char const *Errors<rule>::message = msg

PARSE_ERROR(PredExprEnd,        "expected end of predicate expression");
PARSE_ERROR(Digits,             "expected digits");
PARSE_ERROR(Escaped<one<'\''>>, "expected escape character after '\\'");
PARSE_ERROR(Escaped<one<'"'>>,  "expected escape character after '\\'");
PARSE_ERROR(PredArgVal,         "expected argument value after '='");
PARSE_ERROR(PredColonArgs,      "expected argument list after ':'");
PARSE_ERROR(PredKWArg,          "expected keyword argument after ','");
PARSE_ERROR(PredKWArgs,         "expected keyword argument after ','");
PARSE_ERROR(PredParenClose,     "expected ')' to close function call");
PARSE_ERROR(GroupedPredExpr,    "expected predicate expression after '('");
PARSE_ERROR(PredCloseGroup,     "expected ')' to close predicate expression "
                                "group");
PARSE_ERROR(PredFactor,         "expected predicate expression after operator");

#undef PARSE_ERROR

// Actions ///////////////////////////////////////////////////////////////

template <class Rule>
struct PredAction : nothing<Rule> {};

template <SdfPredicateExpression::Op op>
struct PredOpAction
{
    template <class Input>
    static void apply(Input const &in, SdfPredicateExprBuilder &builder) {
        builder.PushOp(op);
    }
};

template <> struct PredAction<NotKW>
    : PredOpAction<SdfPredicateExpression::Not> {};
template <> struct PredAction<AndKW>
    : PredOpAction<SdfPredicateExpression::And> {};
template <> struct PredAction<OrKW>
    : PredOpAction<SdfPredicateExpression::Or> {};
template <> struct PredAction<ImpliedAnd>
    : PredOpAction<SdfPredicateExpression::ImpliedAnd> {};

template <>
struct PredAction<PredOpenGroup>
{
    template <class Input>
    static void apply(Input const &in, SdfPredicateExprBuilder &builder) {
        builder.OpenGroup();
    }
};

template <>
struct PredAction<PredCloseGroup>
{
    template <class Input>
    static void apply(Input const &in, SdfPredicateExprBuilder &builder) {
        builder.CloseGroup();
    }
};

template <>
struct PredAction<PredFuncName>
{
    template <class Input>
    static void apply(Input const &in, SdfPredicateExprBuilder &builder) {
        builder.SetFuncName(in.string());
    }
};

template <>
struct PredAction<PredArgInt>
{
    template <class Input>
    static bool apply(Input const &in, SdfPredicateExprBuilder &builder) {
        bool outOfRange = false;
        int64_t ival = TfStringToInt64(in.string(), &outOfRange);
        if (outOfRange) {
            return false;
        }
        builder.AddFuncArg(VtValue(ival));
        return true;
    }
};

template <>
struct PredAction<PredArgBool>
{
    template <class Input>
    static void apply(Input const &in, SdfPredicateExprBuilder &builder) {
        builder.AddFuncArg(VtValue(in.string()[0] == 't'));
    }
};

template <>
struct PredAction<PredArgFloat>
{
    template <class Input>
    static void apply(Input const &in, SdfPredicateExprBuilder &builder) {
        std::string const &instr = in.string();
        double fval;
        if (instr == "inf") {
            fval = std::numeric_limits<double>::infinity();
        }
        else if (instr == "-inf") {
            fval = -std::numeric_limits<double>::infinity();
        }
        else {
            fval = TfStringToDouble(instr);
        }
        builder.AddFuncArg(VtValue(fval));
    }
};

template <>
struct PredAction<PredArgString>
{
    template <class Input>
    static void apply(Input const &in, SdfPredicateExprBuilder &builder) {
        std::string const &instr = in.string();
        size_t trimAmount = 0;
        if (instr.size() >= 2 &&
            ((instr.front() == '"' && instr.back() == '"') ||
             (instr.front() == '\'' && instr.back() == '\''))) {
            trimAmount = 1;
        }
        builder.AddFuncArg(
            VtValue(Sdf_EvalQuotedString(
                        instr.c_str(), instr.size(), trimAmount)));
    }
};

template <>
struct PredAction<PredKWArgName>
{
    template <class Input>
    static void apply(Input const &in, SdfPredicateExprBuilder &builder) {
        builder.SetFuncArgKWName(in.string());
    }
};

template <SdfPredicateExpression::FnCall::Kind callKind>
struct PredCallAction
{
    template <class Input>
    static void apply(Input const &in, SdfPredicateExprBuilder &builder) {
        builder.PushCall(callKind);
    }
};
template <> struct PredAction<PredBareCall>
    : PredCallAction<SdfPredicateExpression::FnCall::BareCall> {};
template <> struct PredAction<PredParenCall>
    : PredCallAction<SdfPredicateExpression::FnCall::ParenCall> {};
template <> struct PredAction<PredColonCall>
    : PredCallAction<SdfPredicateExpression::FnCall::ColonCall> {};

} // SdfPredicateExpressionParser

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PREDICATE_EXPRESSION_PARSER_H
