//
// Copyright 2023 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/variableExpression.h"

#include "pxr/usd/sdf/variableExpressionImpl.h"
#include "pxr/usd/sdf/variableExpressionParser.h"

#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

SdfVariableExpression::SdfVariableExpression()
{
    _errors.push_back("No expression specified");
}

SdfVariableExpression::SdfVariableExpression(
    const std::string& expr)
    : _expressionStr(expr)
{
    using ParserResult = Sdf_VariableExpressionParserResult;

    ParserResult parseResult = Sdf_ParseVariableExpression(expr);
    _expression.reset(parseResult.expression.release());
    _errors = std::move(parseResult.errors);
}

SdfVariableExpression::~SdfVariableExpression() = default;

bool
SdfVariableExpression::IsExpression(const std::string& s)
{
    return Sdf_IsVariableExpression(s);
}

bool
SdfVariableExpression::IsValidVariableType(const VtValue& value)
{
    using namespace Sdf_VariableExpressionImpl;

    const VtValue coerced = CoerceIfUnsupportedValueType(value);
    return 
        (coerced.IsEmpty() ? GetValueType(value) : GetValueType(coerced)) != 
        ValueType::Unknown;
}

SdfVariableExpression::operator bool() const
{
    return static_cast<bool>(_expression);
}

const std::string&
SdfVariableExpression::GetString() const
{
    return _expressionStr;
}

const std::vector<std::string>&
SdfVariableExpression::GetErrors() const
{
    return _errors;
}

SdfVariableExpression::Result
SdfVariableExpression::Evaluate(const VtDictionary& stageVariables) const
{
    namespace Impl = Sdf_VariableExpressionImpl;

    if (!_expression) {
        return { VtValue(), GetErrors() };
    }

    Impl::EvalContext ctx(&stageVariables);
    Impl::EvalResult result = _expression->Evaluate(&ctx);

    return { std::move(result.value), 
             std::move(result.errors),
             std::move(ctx.GetRequestedVariables()) };
}

std::string
SdfVariableExpression::_FormatUnexpectedTypeError(
    const VtValue& got, const VtValue& expected)
{
    return TfStringPrintf(
        "Expression evaluated to '%s' but expected '%s'",
        got.GetTypeName().c_str(), expected.GetTypeName().c_str());
}

PXR_NAMESPACE_CLOSE_SCOPE
