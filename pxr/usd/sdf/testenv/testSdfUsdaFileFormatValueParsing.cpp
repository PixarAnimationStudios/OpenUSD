//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/usd/sdf/textParserUtils.h"
#include "pxr/usd/sdf/types.h"

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

static void
CheckParsedValue(
    const std::string& input,
    const SdfValueTypeName& expectedSdfType,
    const VtValue& expectedValue)
{
    VtValue out = Sdf_ParseValueFromString(input, expectedSdfType);
    if (out != expectedValue) {
        TF_FATAL_ERROR("Failed parsing '%s' from input '%s', "
                       "expected '%s', got '%s'",
                       expectedSdfType.GetType().GetTypeName().c_str(),
                       input.c_str(),
                       TfStringify(expectedValue).c_str(),
                       TfStringify(out).c_str());
    }
}

static void
CheckInvalidParsedValue(
    const std::string& input,
    const SdfValueTypeName& expectedSdfType
)
{
    VtValue out = Sdf_ParseValueFromString(input, expectedSdfType);
    if (!out.IsEmpty()) {
        TF_FATAL_ERROR("Expected empty result indicating failure parsing "
                       "'%s' from input '%s', got '%s'",
                       expectedSdfType.GetType().GetTypeName().c_str(),
                       input.c_str(),
                       TfStringify(out).c_str());
    }
}

void
TestInts()
{
    std::map<std::string, VtValue> exprValueMap = {
        {"0", VtValue(0)},
        {"12345", VtValue(12345)},
        {"-12345", VtValue(-12345)}
    };
    for (auto [input, expected] : exprValueMap) {
        CheckParsedValue(input, SdfValueTypeNames->Int,
                         expected);
    }

    std::vector<std::string> invalidExpressions = {
        "foo",
        " 3",
        "!!"
    };
    for (auto input : invalidExpressions) {
        CheckInvalidParsedValue(input, SdfValueTypeNames->Int);
    }
}

void
TestTuples()
{
    std::map<std::string, VtValue> exprValueMap = {
        {"(0, 1, 2)", VtValue(GfVec3f(0, 1, 2))},
        {"(4.5, 1.0, 8.0)", VtValue(GfVec3f(4.5, 1.0, 8.0))},
    };
    for (auto [input, expected] : exprValueMap) {
        CheckParsedValue(input, SdfValueTypeNames->Float3,
                         expected);
    }

    std::vector<std::string> invalidExpressions = {
        "(3.0, 2.0)", // Three values expected
        "[1.0, 2.0, 3.0]", // List syntax
        "(1.0, 2.0, 3.0, 4.0)" // Too many values
    };
    for (auto input : invalidExpressions) {
        CheckInvalidParsedValue(input, SdfValueTypeNames->Float3);
    }
}

void
TestLists()
{
    std::map<std::string, VtValue> exprValueMap = {
        {"[]", VtValue(VtArray<TfToken>{})},
        {"[\'0\']", VtValue(VtArray<TfToken>{TfToken("0")})},
        {"['foo', 'bar', 'baz']", VtValue(VtArray<TfToken>
            {TfToken("foo"), TfToken("bar"), TfToken("baz")})},
    };
    for (auto [input, expected] : exprValueMap) {
        CheckParsedValue(input, SdfValueTypeNames->TokenArray,
                         expected);
    }

    std::vector<std::string> invalidExpressions = {
        "[3.0]", // Quotes expected
        "'foo'", // No square brackets
        "('1', '2', '3')" // Tuple syntax
    };
    for (auto input : invalidExpressions) {
        CheckInvalidParsedValue(input, SdfValueTypeNames->TokenArray);
    }
}

int
main()
{
    TestInts();
    TestTuples();
    TestLists();
    return 0;
}