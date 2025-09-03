//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/vt/dictionary.h"
#include "pxr/usd/sdf/booleanExpression.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/schema.h"

#include <iomanip>
#include <iostream>
#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

bool evaluateExpression(SdfBooleanExpression const& expression,
    VtDictionary const& variables)
{
    auto valueCallback = [&](TfToken const& var) {
        auto it = variables.find(var);
        if (it == variables.end()) {
            return VtValue{};
        }

        return it->second;
    };

    return expression.Evaluate(valueCallback);
}

void testExpression(std::string const& str, VtDictionary const& variables,
    bool shouldParse, bool shouldConstruct, bool expectedValue)
{
    // Test standalone validation
    std::string errorMessage;
    bool didParse = SdfBooleanExpression::Validate(str, &errorMessage);
    if (didParse != shouldParse) {
        if (didParse) {
            TF_FATAL_ERROR("Expected expression to yield a parse error: '%s'",
                str.c_str());
        } else {
            TF_FATAL_ERROR("Unexpected parse error for expression '%s': %s",
                str.c_str(), errorMessage.c_str());
        }
    }

    // Try to construct an expression
    SdfBooleanExpression expression{str};
    auto didConstruct = !expression.IsEmpty();
    if (didConstruct != shouldConstruct) {
        if (didConstruct) {
            TF_FATAL_ERROR("Expected expression to yield a parse error: '%s'",
                str.c_str());
        } else {
            TF_FATAL_ERROR("Unexpected parse error for expression '%s': %s",
                str.c_str(), expression.GetParseError().c_str());
        }
    }

    auto value = evaluateExpression(expression, variables);
    if (value != expectedValue) {
        std::stringstream s;
        s << "Unexpected result evaluating expression ";
        s << std::quoted(str) <<  " with values " << variables;
        s << "; Expected: " << std::boolalpha << expectedValue;
        s << ", actual: " << std::boolalpha << value;
        TF_FATAL_ERROR("%s", s.str().c_str());
    }
}

void testValidExpression(std::string const& str, VtDictionary const& variables,
    bool expectedValue)
{
    testExpression(str, variables, true, true, expectedValue);
}

void testInvalidExpression(std::string const& str, VtDictionary const& variables)
{
    testExpression(str, variables, false, false, false);
}

void testRenaming(std::string const& str, std::string const& expected)
{
    TfToken prefix{"inputs"};
    auto transform = [&](TfToken const& name) {
        TfTokenVector parts{prefix, name};
        return TfToken(SdfPath::JoinIdentifier(parts));
    };

    SdfBooleanExpression expression{str};
    auto renamed = expression.RenameVariables(transform).GetText();
    if (renamed != expected) {
        TF_FATAL_ERROR("Renamed expression does not match expected: '%s'",
            renamed.c_str());
    }
}

void testAllOps(VtDictionary const& vars)
{
    // The string equivalents for each supported comparison operator
    std::vector<std::string> operatorStrings{
        "==", "!=", ">", "<", ">=", "<="
    };

    // Test all combinations of operators and variable types.
    for (auto const& pair : vars) {
        auto variable = pair.first;
        auto value = pair.second;
        int trueCount = 0;
        int falseCount = 0;

        for (auto const& op : operatorStrings) {
            // Start the expression with the variable namd and operator
            std::stringstream s;
            s << variable << " " << op << " ";


            // Append the variable value, quoted if necessary
            if (value.IsHolding<std::string>()) {
                s << std::quoted(value.Get<std::string>());
            } else if (value.IsHolding<TfToken>()) {
                s << std::quoted(value.Get<TfToken>().GetString());
            } else {
                s << value;
            }

            // Make sure the expression can be parsed.
            SdfBooleanExpression expression{s.str()};
            if (expression.IsEmpty()) {
                TF_FATAL_ERROR("Unexpected parse error for expression '%s': %s",
                    s.str().c_str(), expression.GetParseError().c_str());
            }

            // Evaluate the expression and count the result.
            auto result = evaluateExpression(expression, vars);
            if (result) {
                trueCount++;
            } else {
                falseCount++;
            }
        }

        // Without knowing anything about the result of the individual
        // expressions, the total number of true and false results should be
        // independent of the actual expression, since each operator has a
        // "mirror" operator which always produces the opposite value.
        if (trueCount != 3 || falseCount != 3) {
            TF_FATAL_ERROR("Unexpected results from expressions using '%s'",
                variable.c_str());
        }
    }
}

void testVariableCoercion()
{
    VtDictionary vars{
        {"foo", VtValue(true)},
        {"bar", VtValue(false)},
        {"int0", VtValue(0)},
        {"int1", VtValue(1)},
    };

    // test casting variables to booleans
    testValidExpression("foo", vars, true);
    testValidExpression("bar", vars, false);
    testValidExpression("int1", vars, true);
    testValidExpression("int0", vars, false);

    // test complementing the result of a cast
    testValidExpression("!foo", vars, false);
    testValidExpression("!bar", vars, true);
    testValidExpression("!int1", vars, false);
    testValidExpression("!int0", vars, true);

    // test boolean ops on casted variables
    testValidExpression("!foo || bar", vars, false);
    testValidExpression("foo && !bar", vars, true);
}

void testConstruction()
{
    using BinaryOperator = SdfBooleanExpression::BinaryOperator;
    using UnaryOperator = SdfBooleanExpression::UnaryOperator;

    // foo
    auto varFoo = TfToken("foo");
    auto variable = SdfBooleanExpression::MakeVariable(varFoo);
    TF_AXIOM(variable.GetText() == "foo");

    // foo == "bar"
    auto stringBar = VtValue("bar");
    auto constant = SdfBooleanExpression::MakeConstant(stringBar);
    auto comparison = SdfBooleanExpression::MakeBinaryOp(variable,
        BinaryOperator::EqualTo, constant);
    TF_AXIOM(comparison.GetText() == "foo == \"bar\"");

    // foo == "bar" || foo == "bar"
    auto OR = SdfBooleanExpression::MakeBinaryOp(comparison,
        BinaryOperator::Or, comparison);
    TF_AXIOM(OR.GetText() == "foo == \"bar\" || foo == \"bar\"");

    // (foo == "bar" || foo == "bar") && (foo == "bar" || foo == "bar")
    auto AND = SdfBooleanExpression::MakeBinaryOp(OR, BinaryOperator::And, OR);
    TF_AXIOM(AND.GetText() == "(foo == \"bar\" || foo == \"bar\") && "
        "(foo == \"bar\" || foo == \"bar\")");

    // !(foo == "bar")
    auto not1 = SdfBooleanExpression::MakeUnaryOp(comparison, UnaryOperator::Not);
    TF_AXIOM(not1.GetText() == "!(foo == \"bar\")");

    // !(foo == "bar" || foo == "bar")
    auto not2 = SdfBooleanExpression::MakeUnaryOp(OR, UnaryOperator::Not);
    TF_AXIOM(not2.GetText() == "!(foo == \"bar\" || foo == \"bar\")");

    // foo == "\"quotes\nand\tnewlines\""
    auto stringWithEscapes = VtValue(std::string("\"quotes\nand\tnewlines\""));
    auto constant2 = SdfBooleanExpression::MakeConstant(stringWithEscapes);
    auto escaped = SdfBooleanExpression::MakeBinaryOp(variable,
        SdfBooleanExpression::BinaryOperator::EqualTo, constant2);
    TF_AXIOM(escaped.GetText() == "foo == '\"quotes\\nand\\tnewlines\"'");
}

} // namespace

int main()
{
    VtDictionary vars{
        {"someInt", VtValue(3)},
        {"anotherInt", VtValue(4)},
        {"someDouble", VtValue(42.0)},
        {"someString", VtValue("test")},
        {"someToken", VtValue(TfToken("testToken"))},
    };

    // int variables
    testValidExpression("someInt == 3", vars, true);
    testValidExpression("someInt == 2", vars, false);
    testValidExpression("someInt != 3", vars, false);
    testValidExpression("someInt != 2", vars, true);
    testValidExpression("someInt > 2", vars, true);
    testValidExpression("someInt >= 2", vars, true);
    testValidExpression("someInt < 2", vars, false);
    testValidExpression("someInt <= 2", vars, false);

    // double variables
    testValidExpression("someDouble == 42.0", vars, true);
    testValidExpression("someDouble == 41.0", vars, false);
    testValidExpression("someDouble != 42.0", vars, false);
    testValidExpression("someDouble != 41.0", vars, true);
    testValidExpression("someDouble > 42", vars, false);
    testValidExpression("someDouble >= 42", vars, true);
    testValidExpression("someDouble < 42", vars, false);
    testValidExpression("someDouble <= 42", vars, true);

    // string variables
    testValidExpression("someString == 'test'", vars, true);
    testValidExpression("someString == 'foo'", vars, false);
    testValidExpression("someString != 'test'", vars, false);
    testValidExpression("someString != 'foo'", vars, true);

    // token variables
    testValidExpression("someToken == 'testToken'", vars, true);
    testValidExpression("someToken == 'foo'", vars, false);
    testValidExpression("someToken != 'testToken'", vars, false);
    testValidExpression("someToken != 'foo'", vars, true);

    // comparison between variables
    testValidExpression("someInt <= anotherInt", vars, true);

    // parenthesis
    testValidExpression("(someInt <= anotherInt)", vars, true);

    // complement
    testValidExpression("!(someInt <= anotherInt)", vars, false);

    // boolean constants
    testValidExpression("true", vars, true);
    testValidExpression("false", vars, false);

    // boolean combinations
    testValidExpression("someInt == 3 && someDouble == 42.0", vars, true);
    testValidExpression("someInt == 3 || someDouble == 42.0", vars, true);
    testValidExpression("(someInt == 3 || someDouble == 2.0) &&"
        "(someInt == 2 || someDouble == 42.0)", vars, true);

    // parse errors
    testInvalidExpression("someInt =< 3", vars);

    // renaming
    testRenaming("foo == 3 && bar == 'baz'",
                 "inputs:foo == 3 && inputs:bar == \"baz\"");
    testRenaming("!foo && bar",
                 "!inputs:foo && inputs:bar");

    // Test all ops against all variables
    VtDictionary nonStringVars{
        {"someInt", VtValue(3)},
        {"anotherInt", VtValue(4)},
        {"someDouble", VtValue(42.0)},
        {"someBool", VtValue(true)},
    };
    testAllOps(nonStringVars);

    // Test coercing variables into boolean values
    testVariableCoercion();

    // Test programmatic construction
    testConstruction();

    std::cout << ">>> Test SUCCEEDED\n";

    return 0;
}
