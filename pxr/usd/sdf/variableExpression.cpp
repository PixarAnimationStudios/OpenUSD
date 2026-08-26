//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/variableExpression.h"

#include "pxr/usd/sdf/variableExpressionImpl.h"
#include "pxr/usd/sdf/variableExpressionParser.h"

#include "pxr/usd/sdf/fileIO_Common.h"

#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

SdfVariableExpression::SdfVariableExpression()
{
    _errors.push_back("No expression specified");
}

SdfVariableExpression::SdfVariableExpression(
    const std::string& expr)
    : SdfVariableExpression(std::string(expr))
{
}

SdfVariableExpression::SdfVariableExpression(
    std::string&& expr)
    : _expressionStr(std::move(expr))
{
    using ParserResult = Sdf_VariableExpressionParserResult;

    ParserResult parseResult = Sdf_ParseVariableExpression(_expressionStr);
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

SdfVariableExpression::Builder::
operator SdfVariableExpression() const
{
    return SdfVariableExpression('`' + _expr + '`');
}

SdfVariableExpression::FunctionBuilder::
operator SdfVariableExpression() const
{ 
    return SdfVariableExpression('`' + _expr + ")`");
}

SdfVariableExpression::FunctionBuilder::
operator SdfVariableExpression::Builder() const &
{ 
    return Builder(_expr + ')');
}

SdfVariableExpression::FunctionBuilder::
operator SdfVariableExpression::Builder() &&
{ 
    _expr += ')';
    return Builder(std::move(_expr));
}

SdfVariableExpression::ListBuilder::
operator SdfVariableExpression() const
{ 
    return SdfVariableExpression('`' + _expr + "]`");
}

SdfVariableExpression::ListBuilder::
operator SdfVariableExpression::Builder() const &
{ 
    return Builder(_expr + ']');
}

SdfVariableExpression::ListBuilder::
operator SdfVariableExpression::Builder() &&
{ 
    _expr += ']';
    return Builder(std::move(_expr));
}

void
SdfVariableExpression::_AppendExpression(
    std::string* expr, const SdfVariableExpression& arg, bool first)
{
    if (const std::string& e = arg.GetString(); IsExpression(e)) {
        if (!first) {
            (*expr) += ", ";
        }
        // Expression strings returned by SdfVariableExpression are
        // bracketed by "`" characters, so we need to strip them off
        // before appending them.
        (*expr) += e.substr(1, e.size() - 2);
    }
}

void
SdfVariableExpression::_AppendBuilder(
    std::string* expr, const Builder& b, bool first)
{
    if (!first) {
        (*expr) += ", ";
    }
    (*expr) += b._expr;
}

SdfVariableExpression::Builder
SdfVariableExpression::MakeLiteral(int64_t value)
{
    return Builder(TfStringify(value));
}

SdfVariableExpression::Builder
SdfVariableExpression::MakeLiteral(bool value)
{
    return Builder(std::string(value ? "True" : "False"));
}

SdfVariableExpression::Builder
SdfVariableExpression::MakeLiteral(const std::string& value)
{
    // Variable expression syntax does not allow triple quotes.
    return Builder(
        Sdf_FileIOUtility::Quote(value, /* allowTripleQuotes = */ false));
}

SdfVariableExpression::Builder
SdfVariableExpression::MakeLiteral(const char* value)
{
    return MakeLiteral(std::string(value));
}

SdfVariableExpression::Builder
SdfVariableExpression::MakeNone()
{
    return Builder("None");
}

SdfVariableExpression::Builder
SdfVariableExpression::MakeVariable(const std::string& name)
{
    return Builder("${" + name + "}");
}

SdfVariableExpression::Builder
SdfVariableExpression::MakeVariable(
    const std::string& name,
    const Builder& fallbackValue)
{
    return Builder("${" + name + ":" + fallbackValue._expr + "}");
}

SdfVariableExpression::Builder
SdfVariableExpression::MakeVariable(
    const std::string& name,
    const SdfVariableExpression& fallbackValue)
{
    // Expression strings returned by SdfVariableExpression are bracketed by
    // "`" characters.
    const std::string& e = fallbackValue.GetString();
    const std::string fallbackExpr =
        IsExpression(e) ? e.substr(1, e.size() - 2) : e;
    return Builder("${" + name + ":" + fallbackExpr + "}");
}

PXR_NAMESPACE_CLOSE_SCOPE
