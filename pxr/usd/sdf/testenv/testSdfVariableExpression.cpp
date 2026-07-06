//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/sdf/variableExpression.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"
#include <cstdio>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE;

template <class T>
static void
_TestExpression(
    const SdfVariableExpression& expr,
    const VtDictionary& exprVars,
    const T& expectedResult)
{
    TF_AXIOM(expr);
    TF_AXIOM(expr.Evaluate(exprVars).value == expectedResult);
}

static void
_TestMakeNone()
{
    _TestExpression(SdfVariableExpression::MakeNone(), {}, VtValue());
}

static void
_TestMakeVariable()
{
    _TestExpression(
        SdfVariableExpression::MakeVariable("FOO"), 
        {{"FOO", VtValue(1)}}, int64_t(1));
}

static void
_TestMakeLiteral()
{
    auto _test = [](const auto& value) {
        _TestExpression(
            SdfVariableExpression::MakeLiteral(value), {}, VtValue(value));
    };

    _test(int64_t(123));
    _test(true);
    _test(false);
    _test("a string");
    _test("'single quotes'");
    _test(R"(\'escaped single quotes\')");
    _test(R"("double quotes")");
    _test(R"(\"escaped double quotes\")");
    _test(R"(mixed 'single quotes' and "double quotes")");
    _test("contains\nnewlines");
}

static void
_TestMakeFunction()
{
    _TestExpression(
        SdfVariableExpression::MakeFunction(
            "contains",
            SdfVariableExpression::MakeListOfLiterals(
                std::vector<int64_t>{1, 2, 3}),
            SdfVariableExpression::MakeVariable("foo")),
        {{"foo", VtValue(2)}}, true);

    _TestExpression(
        SdfVariableExpression::MakeFunction(
            "matches_regex",
            SdfVariableExpression::MakeLiteral("shot_1"),
            SdfVariableExpression::MakeVariable("foo")),
        {{"foo", VtValue("shot_?")}}, true);
}

static void
_TestMakeList()
{
    _TestExpression(SdfVariableExpression::MakeList(), {}, 
        VtValue(SdfVariableExpression::EmptyList()));
    
    _TestExpression(
        SdfVariableExpression::MakeList(
            SdfVariableExpression::MakeLiteral(int64_t(1)),
            SdfVariableExpression::MakeVariable("foo"),
            SdfVariableExpression::MakeFunction(
                "if",
                SdfVariableExpression::MakeLiteral(true),
                SdfVariableExpression::MakeLiteral(int64_t(3)))),
        {{"foo", VtValue(2)}}, VtInt64Array{1, 2, 3});

    auto builder = SdfVariableExpression::MakeList();
    builder.AddElement(SdfVariableExpression::MakeLiteral(int64_t(1)));
    builder.AddElement(SdfVariableExpression::MakeVariable("foo"));
    builder.AddElement(SdfVariableExpression::MakeFunction(
            "if",
            SdfVariableExpression::MakeLiteral(true),
            SdfVariableExpression::MakeLiteral(int64_t(3))));
    _TestExpression(builder, {{"foo", VtValue(2)}}, VtInt64Array{1, 2, 3});
}

static void
_TestMakeListOfLiterals()
{
    auto _test = [](
        const auto& sourceValues, const auto& expectedValue) {

        _TestExpression(
            SdfVariableExpression::MakeListOfLiterals(sourceValues),
            {}, VtValue(expectedValue));

        auto builder = SdfVariableExpression::MakeList();
        builder.AddLiteralValues(sourceValues);
        _TestExpression(builder, {}, VtValue(expectedValue));
    };

    _test(std::vector<int64_t>{}, SdfVariableExpression::EmptyList());
    _test(std::vector<int64_t>{1, 2, 3}, VtInt64Array{1, 2, 3});
    _test(std::vector<bool>{true, false}, VtBoolArray{true, false});
    _test(std::vector<std::string>{"a", "b"}, VtStringArray{"a", "b"});
}

int main()
{
    _TestMakeNone();
    _TestMakeVariable();
    _TestMakeLiteral();
    _TestMakeFunction();
    _TestMakeList();
    _TestMakeListOfLiterals();

    printf("PASSED!\n");
    return 0;
}
