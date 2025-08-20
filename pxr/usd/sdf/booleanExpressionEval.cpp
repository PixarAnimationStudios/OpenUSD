//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/sdf/booleanExpressionEval.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/vt/visitValue.h"

PXR_NAMESPACE_OPEN_SCOPE

using BinaryOperator = SdfBooleanExpression::BinaryOperator;
using UnaryOperator = SdfBooleanExpression::UnaryOperator;
using VariableCallback = SdfBooleanExpression::VariableCallback;
using NameTransform = SdfBooleanExpression::NameTransform;

namespace {

struct PromoteToCommonType {
    // Convert numeric values to double
    VtValue operator()(double const& val) const {
        return VtValue(val);
    }

    // Convert string values to std::string. Note that this overload covers
    // TfToken since it implicitly converts to string
    VtValue operator()(std::string const& str) const {
        return VtValue(str);
    }

    // Return an empty value as a fallback
    VtValue operator()(VtValue const&) const {
        return {};
    }
};

bool
_ApplyComparisonOp(VtValue const& left, VtValue const& right, BinaryOperator op)
{
    // Conver to common types
    VtValue lhs = VtVisitValue(left, PromoteToCommonType());
    VtValue rhs = VtVisitValue(right, PromoteToCommonType());

    // The == and != operators can be performed directly on VtValue.
    switch (op) {
        case BinaryOperator::EqualTo:
            return lhs == rhs;
        case BinaryOperator::NotEqualTo:
            return lhs != rhs;
        default:
            break;
    }

    // For the other comparison operators, convert the incoming values to double
    // first; warn if the types can't be converted.
    if (!lhs.IsHolding<double>() || !rhs.IsHolding<double>()) {
        TF_WARN("The '%s' operator is not supported between '%s' and '%s'",
            TfEnum::GetDisplayName(op).c_str(), left.GetTypeName().c_str(),
            right.GetTypeName().c_str());
        return false;
    }

    double lhsDouble = lhs.Get<double>();
    double rhsDouble = rhs.Get<double>();

    switch (op) {
        case BinaryOperator::GreaterThan:          return lhsDouble > rhsDouble;
        case BinaryOperator::LessThan:             return lhsDouble < rhsDouble;
        case BinaryOperator::GreaterThanOrEqualTo: return lhsDouble >= rhsDouble;
        case BinaryOperator::LessThanOrEqualTo:    return lhsDouble <= rhsDouble;

        // This should not be reachable. All comparison operators are handled
        // above and the boolean operators shouldn't reach this function.
        default:                                   return false;
    }
}

VtValue
_EvalExpression(SdfBooleanExpression const& expression,
                VariableCallback const& callback)
{
    VtValue result;

    // Handle comparison between two subexpressions
    auto binary = [&result, callback](SdfBooleanExpression const& lhs,
                                      BinaryOperator op,
                                      SdfBooleanExpression const& rhs) {

        bool isBoolean = op == BinaryOperator::And ||
                         op == BinaryOperator::Or;
        if (isBoolean) {
            auto left = Sdf_EvalBooleanExpression(lhs, callback);
            if (!left && op == BinaryOperator::And) {
                // short-circuit for &&
                result = false;
            } else if (left && op == BinaryOperator::Or) {
                // short-circuit for ||
                result = true;
            } else {
                // non-short-circuit
                result = Sdf_EvalBooleanExpression(rhs, callback);
            }
        } else {
            auto left = _EvalExpression(lhs, callback);
            auto right = _EvalExpression(rhs, callback);
            result = _ApplyComparisonOp(left, right, op);
        }
    };

    // Handle unary op (only unary-not for now)
    auto unary = [&result, callback](SdfBooleanExpression const& inner,
                                     UnaryOperator op) {
        result = !Sdf_EvalBooleanExpression(inner, callback);
    };

    // Handle variable lookup
    auto variable = [&result, callback](TfToken const& name) {
        result = callback(name);
    };

    // Handle constant values
    auto constant = [&result](VtValue const& value) {
        result = value;
    };

    // Recursively apply our evaluation callbacks
    expression.Visit(variable, constant, binary, unary);

    return result;
}

} // namespace

bool
Sdf_EvalBooleanExpression(SdfBooleanExpression const& expression,
                          VariableCallback const& callback)
{
    VtValue value = _EvalExpression(expression, callback);
    return value.Cast<bool>().GetWithDefault<bool>();
}

PXR_NAMESPACE_CLOSE_SCOPE
