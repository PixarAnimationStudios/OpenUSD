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
#include "pxr/usd/sdf/variableExpressionImpl.h"

#include "pxr/usd/sdf/variableExpression.h"
#include "pxr/usd/sdf/variableExpressionParser.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/visitValue.h"

#include "pxr/base/tf/scoped.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

namespace Sdf_VariableExpressionImpl
{

// Metafunctions describing supported expression variable value types.
template <class T> 
using IsSupportedScalarType = std::integral_constant<
    bool, 
    std::is_same<T, std::string>::value
    || std::is_same<T, bool>::value
    || std::is_same<T, int64_t>::value
>;

template <class T> 
using IsSupportedListType = std::integral_constant<
    bool, 
    std::is_same<T, VtArray<std::string>>::value
    || std::is_same<T, VtArray<bool>>::value
    || std::is_same<T, VtArray<int64_t>>::value
>;

class _TypeVisitor
{
public:
    ValueType operator()(const std::string&) const
    {
        return ValueType::String;
    }

    ValueType operator()(bool) const
    {
        return ValueType::Boolean;
    }

    ValueType operator()(int64_t) const
    {
        return ValueType::Integer;
    }

    ValueType operator()(const VtValue& v) const
    {
        if (v.IsEmpty()) {
            return ValueType::None;
        }
        if (v.IsHolding<SdfVariableExpression::EmptyList>()) {
            return ValueType::List;
        }
        return ValueType::Unknown;
    }

    template <class T>
    std::enable_if_t<IsSupportedListType<T>::value, ValueType>
    operator()(const T&) const {
        return ValueType::List;
    }

    template <class T>
    std::enable_if_t<!IsSupportedListType<T>::value, ValueType>
    operator()(const T&) const {
        return ValueType::Unknown;
    }
};

ValueType
GetValueType(const VtValue& value)
{
    return VtVisitValue(value, _TypeVisitor());
}

std::string
GetValueTypeName(const VtValue& value)
{
    switch (GetValueType(value)) {
    case ValueType::String:
        return "string";
    case ValueType::Boolean:
        return "bool";
    case ValueType::Integer:
        return "int";
    case ValueType::List:
        return "list";
    case ValueType::None:
        return "None";
    case ValueType::Unknown:
        break;
    }

    return value.GetTypeName();
}

VtValue
CoerceIfUnsupportedValueType(const VtValue& value)
{
    // We do not use VtValue's built-in casting mechanism as we want to 
    // tightly control the coercions we allow in the expression language.

    // Coerce int -> int64.
    if (value.IsHolding<int>()) {
        return VtValue(int64_t(value.UncheckedGet<int>()));
    }

    // Coerce VtArray<int> -> VtArray<int64>.
    if (value.IsHolding<VtArray<int>>()) {
        const VtArray<int>& intArray = value.UncheckedGet<VtArray<int>>();
        return VtValue(VtArray<int64_t>(intArray.begin(), intArray.end()));
    }

    return VtValue();
}

// ------------------------------------------------------------
// Helper functions for collecting or combining errors from one or
// more EvalResult objects into a single list.

static bool
_CollectErrors(
    std::vector<std::string>* errors,
    EvalResult* result)
{
    if (result->errors.empty()) {
        return false;
    }

    errors->insert(
        errors->end(),
        std::make_move_iterator(result->errors.begin()),
        std::make_move_iterator(result->errors.end()));
    return true;
}

template <class Result, class... Others>
static bool
_CollectErrors(
    std::vector<std::string>* errors,
    Result* result, Others*... others)
{
    // Bitwise-or is intentional to prevent short-circuiting.
    return _CollectErrors(errors, result) | _CollectErrors(errors, others...);
}

template <class... Results>
static std::vector<std::string>
_CombineErrors(Results*... results)
{
    std::vector<std::string> errors;
    _CollectErrors(&errors, results...);
    return errors;
}

// ------------------------------------------------------------

EvalContext::EvalContext(const VtDictionary* variables)
    : _variables(variables)
{
}

bool
EvalContext::HasVariable(const std::string& var)
{
    return static_cast<bool>(TfMapLookupPtr(*_variables, var));
}

std::pair<EvalResult, bool>
EvalContext::GetVariable(const std::string& var)
{
    // Check if we have circular variable substitutions.
    if (std::find(_variableStack.begin(), _variableStack.end(), var) 
        != _variableStack.end()) {

        std::vector<std::string> formattedVars;
        for (const std::string& s : _variableStack) {
            formattedVars.push_back("'" + s + "'");
        }

        return { 
            EvalResult::Error({ 
                TfStringPrintf(
                    "Encountered circular variable substitutions: [%s, '%s']",
                    TfStringJoin(
                        formattedVars.begin(),  formattedVars.end(), ", ")
                        .c_str(),
                    var.c_str()) }),
            true
        };
    }

    _requestedVariables.insert(var);

    const VtValue* value = TfMapLookupPtr(*_variables, var);
    if (!value) {
        return { EvalResult::NoValue(), false };
    }

    // Coerce the variable to a supported type if necessary.
    const VtValue coercedValue = CoerceIfUnsupportedValueType(*value);
    if (!coercedValue.IsEmpty()) {
        value = &coercedValue;
    }

    // If the variable isn't a supported type, return an error.
    if (GetValueType(*value) == ValueType::Unknown) {
        return { 
            EvalResult::Error({
                TfStringPrintf(
                    "Variable '%s' has unsupported type %s",
                    var.c_str(), GetValueTypeName(*value).c_str()) }),
            true
        };
    }

    // If the value of the variable is itself an expression,
    // parse and evaluate it and return the result.
    if (value->IsHolding<std::string>()) {
        const std::string& strValue = value->UncheckedGet<std::string>();
        if (Sdf_IsVariableExpression(strValue)) {
            Sdf_VariableExpressionParserResult subExpr =
                Sdf_ParseVariableExpression(strValue);
            if (subExpr.expression) {
                _variableStack.push_back(var);
                TfScoped<> popStack(
                    [this]() { _variableStack.pop_back(); });

                return { subExpr.expression->Evaluate(this), true };
            }
            else if (!subExpr.errors.empty()) {
                for (std::string& err : subExpr.errors) {
                    err += TfStringPrintf(
                        " (in variable '%s')", var.c_str());
                }

                return { EvalResult::Error(std::move(subExpr.errors)), true };
            }
            else {
                return { EvalResult::NoValue(), true };
            }
        }
    }

    return { EvalResult::Value(*value), true };
}

// ------------------------------------------------------------

Node::~Node() = default;

// ------------------------------------------------------------

StringNode::StringNode(std::vector<Part>&& parts)
    : _parts(std::move(parts))
{
    // Handle escape sequences in the expression here so we don't have to
    // do it every time we evaluate this node.
    for (Part& part : _parts) {
        if (!part.isVariable) {
            part.content = TfEscapeString(part.content);
        }
    }
}

EvalResult
StringNode::Evaluate(EvalContext* ctx) const
{
    std::string result;
    for (const Part& part : _parts) {
        if (part.isVariable) {
            const std::string& variable = part.content;

            EvalResult varResult;
            bool varHasValue = false;
            std::tie(varResult, varHasValue) = ctx->GetVariable(variable);

            if (!varHasValue) {
                // No value for variable. Leave the substitution
                // string in place in case downstream clients want to
                // handle it.
                result += part.content;
            }
            else if (!varResult.value.IsEmpty()) {
                if (varResult.value.IsHolding<std::string>()) {
                    // Substitute the value of the variable into the
                    // result string.
                    result += varResult.value.UncheckedGet<std::string>();
                }
                else {
                    // The value of the variable was not a string.
                    // Flag an error and abort evaluation.
                    return EvalResult::Error({
                       TfStringPrintf(
                           "String value required for substituting "
                           "variable '%s', got %s.",
                           variable.c_str(),
                           GetValueTypeName(varResult.value).c_str())
                    });
                }
            }
            else if (!varResult.errors.empty()) {
                // There was an error when obtaining the value for the
                // variable. For example, the value was itself an
                // expression but could not be evaluated due to a syntax
                // error. In this case we copy the errors to the result
                // and abort evaluation with an error.
                return EvalResult::Error(std::move(varResult.errors));
            }
            else {
                // The variable value was empty, but no errors occurred.
                // This could happen if the variable was a subexpression
                // that returned no value. We treat this as though it were the
                // empty string.
            }
        }
        else {
            result += part.content;
        }
    }

    return EvalResult::Value(std::move(result));
}

// ------------------------------------------------------------

VariableNode::VariableNode(std::string&& var)
    : _var(std::move(var))
{
}

EvalResult
VariableNode::Evaluate(EvalContext* ctx) const
{
    std::pair<EvalResult, bool> varResult = ctx->GetVariable(_var);

    if (!varResult.second) {
        return EvalResult::Error({
            TfStringPrintf("No value for variable '%s'", _var.c_str())
        });
    }

    return varResult.first;
}

// ------------------------------------------------------------

template <class T>
ConstantNode<T>::ConstantNode(T value)
    : _value(value)
{
}

template <class T>
EvalResult
ConstantNode<T>::Evaluate(EvalContext* ctx) const
{
    return EvalResult::Value(_value);
}

template class ConstantNode<int64_t>;
template class ConstantNode<bool>;

// ------------------------------------------------------------

NoneNode::NoneNode() = default;

EvalResult
NoneNode::Evaluate(EvalContext* ctx) const
{
    return EvalResult::NoValue();
}

// ------------------------------------------------------------

ListNode::ListNode(std::vector<std::unique_ptr<Node>>&& elements)
    : _elements(std::move(elements))
{
}

class _ListVisitor
{
public:
    template <class T>
    typename std::enable_if_t<IsSupportedScalarType<T>::value, bool>
    operator()(T v)
    {
        if (list.IsEmpty()) {
            list = VtArray<T>(1, std::move(v));
            return true;
        }
        else if (list.IsHolding<VtArray<T>>()) {
            list.UncheckedMutate<VtArray<T>>(
                [&v](VtArray<T>& arr) { arr.push_back(std::move(v)); });
            return true;
        }

        return false;
    }

    template <class T>
    typename std::enable_if_t<!IsSupportedScalarType<T>::value, bool>
    operator()(T v)
    {
        return false;
    }

    VtValue list;
};

EvalResult
ListNode::Evaluate(EvalContext* ctx) const
{
    _ListVisitor visitor;
    std::vector<std::string> errors;
    
    for (size_t i = 0; i < _elements.size(); ++i) {
        EvalResult r = _elements[i]->Evaluate(ctx);
        if (_CollectErrors(&errors, &r)) {
            continue;
        }

        if (!VtVisitValue(r.value, visitor)) {
            errors.push_back(TfStringPrintf(
                "Unexpected value of type %s in list at element %zu",
                GetValueTypeName(r.value).c_str(), i));
        }
    }

    if (!errors.empty()) {
        return EvalResult::Error(std::move(errors));
    }
    
    if (visitor.list.IsEmpty()) {
        // The expression evaluated to an empty list, but we can't
        // put an empty VtArray into the result because we don't know
        // what type that VtArray ought to be holding. So instead, we
        // return a special object that represents the empty list.
        return EvalResult::Value(SdfVariableExpression::EmptyList());
    }

    return EvalResult::Value(std::move(visitor.list));
}

// ------------------------------------------------------------

template <class FunctionNode>
static std::string
_FormatFunctionError(const std::string& err)
{
    return TfStringPrintf(
        "%s: %s", FunctionNode::GetFunctionName(), err.c_str());
}

// ------------------------------------------------------------

const char* 
IfNode::GetFunctionName()
{
    return "if";
}

EvalResult
IfNode::_Evaluate(
    EvalContext* ctx,
    const std::unique_ptr<Node>& condition,
    const std::unique_ptr<Node>& ifValue,
    const std::unique_ptr<Node>& elseValue)
{
    EvalResult result = condition->Evaluate(ctx);
    if (!result.errors.empty()) {
        return EvalResult::Error(std::move(result.errors));
    }

    if (!result.value.IsHolding<bool>()) {
        return EvalResult::Error({
            _FormatFunctionError<IfNode>("Condition must be a boolean value")});
    }

    // Verify that ifValue and elseValue either evaluate to the same type
    // or one or the other evaluates to None. Nothing in the code actually
    // depends on this. We choose to impose this as an extra requirement
    // because it seems unlikely a user would do this intentionally, and
    // raising this as an error could be a useful indicator. This does
    // impose the extra cost of evaluating both branches, but we don't
    // believe that would be significant.
    const EvalResult ifValueResult = ifValue->Evaluate(ctx);
    const EvalResult elseValueResult = 
        elseValue ? elseValue->Evaluate(ctx) : EvalResult::NoValue();

    if (elseValue) {
        const bool valuesHaveDifferentTypes =
            ifValueResult.value.GetType() != elseValueResult.value.GetType();
        const bool neitherValueIsNone =
            !ifValueResult.value.IsEmpty() && !elseValueResult.value.IsEmpty();

        if (valuesHaveDifferentTypes && neitherValueIsNone) {
            return EvalResult::Error({
                _FormatFunctionError<IfNode>(
                    "if-value and else-value must evaluate to the same type "
                    "or None.")});
        }
    }

    return result.value.UncheckedGet<bool>() ? ifValueResult : elseValueResult;
}

If2Node::If2Node(
    std::unique_ptr<Node>&& condition,
    std::unique_ptr<Node>&& ifValue)
    : _condition(std::move(condition))
    , _ifValue(std::move(ifValue))
{
}

EvalResult
If2Node::Evaluate(EvalContext* ctx) const
{
    return _Evaluate(ctx, _condition, _ifValue, nullptr);
}

If3Node::If3Node(
    std::unique_ptr<Node>&& condition,
    std::unique_ptr<Node>&& ifValue,
    std::unique_ptr<Node>&& elseValue)
    : _condition(std::move(condition))
    , _ifValue(std::move(ifValue))
    , _elseValue(std::move(elseValue))
{
}

EvalResult
If3Node::Evaluate(EvalContext* ctx) const
{
    return _Evaluate(ctx, _condition, _ifValue, _elseValue);
}

// ------------------------------------------------------------

template <class T, template <typename> class Comparator>
EvalResult
_Compare(const T& x, const T& y)
{
    return EvalResult::Value(Comparator<T>()(x, y));
}

// Only allow equality comparisons for 'None' values.
template <template <typename> class Comparator>
EvalResult
_Compare(const VtValue& x, const VtValue& y)
{
    return EvalResult::Error({
        _FormatFunctionError<ComparisonNode<Comparator>>(
            "Comparison operation not supported for None")});
}

template <>
EvalResult
_Compare<std::equal_to>(const VtValue& x, const VtValue& y)
{
    return EvalResult::Value(true);
}

template <>
EvalResult
_Compare<std::not_equal_to>(const VtValue& x, const VtValue& y)
{
    return EvalResult::Value(false);
}

template <template <typename> class Comparator>
class _ComparisonVisitor
{
public:
    _ComparisonVisitor(const VtValue& y) : _y(y) { }

    EvalResult
    operator()(const VtValue& x) const
    {
        TF_VERIFY(x.IsEmpty() && _y.IsEmpty());
        return _Compare<Comparator>(x, _y);
    }

    template <class T>
    std::enable_if_t<IsSupportedScalarType<T>::value, EvalResult>
    operator()(const T& x) const
    {
        return _Compare<T, Comparator>(x, _y.UncheckedGet<T>());
    }

    template <class T>
    std::enable_if_t<!IsSupportedScalarType<T>::value, EvalResult>
    operator()(const T& x) const
    {
        return EvalResult::Error({
            _FormatFunctionError<ComparisonNode<Comparator>>(
                "Unsupported type for comparison")});
    }

private:
    const VtValue& _y;
};

template <template <typename> class Comparator>
ComparisonNode<Comparator>::ComparisonNode(
    std::unique_ptr<Node>&& x, 
    std::unique_ptr<Node>&& y)
    : _x(std::move(x))
    , _y(std::move(y))
{
}

template <template <typename> class Comparator>
EvalResult
ComparisonNode<Comparator>::Evaluate(EvalContext* ctx) const
{
    EvalResult x = _x->Evaluate(ctx);
    EvalResult y = _y->Evaluate(ctx);

    std::vector<std::string> errors = _CombineErrors(&x, &y);
    if (!errors.empty()) {
        return EvalResult::Error(std::move(errors));
    }

    if (x.value.GetType() != y.value.GetType()) {
        return EvalResult::Error({
            _FormatFunctionError<ComparisonNode<Comparator>>(
                TfStringPrintf(
                    "Cannot compare values of type %s and %s",
                    GetValueTypeName(x.value).c_str(),
                    GetValueTypeName(y.value).c_str())
            )});
    }

    return VtVisitValue(x.value, _ComparisonVisitor<Comparator>(y.value));
}

#define DEFINE_COMPARISON_NODE(ComparatorType, FunctionName)            \
template class ComparisonNode<ComparatorType>;                          \
template <> const char*                                                 \
ComparisonNode<ComparatorType>::GetFunctionName() { return FunctionName; }

DEFINE_COMPARISON_NODE(std::equal_to, "eq");
DEFINE_COMPARISON_NODE(std::not_equal_to, "neq");
DEFINE_COMPARISON_NODE(std::less, "lt");
DEFINE_COMPARISON_NODE(std::less_equal, "leq");
DEFINE_COMPARISON_NODE(std::greater, "gt");
DEFINE_COMPARISON_NODE(std::greater_equal, "geq");

// ------------------------------------------------------------

template <template <typename> class Operator>
LogicalNode<Operator>::LogicalNode(
    std::vector<std::unique_ptr<Node>>&& conditions)
    : _conditions(std::move(conditions))
{
}

template <template <typename> class Operator>
EvalResult
LogicalNode<Operator>::Evaluate(EvalContext* ctx) const
{
    VtValue result;
    std::vector<std::string> errors;

    for (size_t i = 0; i < _conditions.size(); ++i) {
        EvalResult condition = _conditions[i]->Evaluate(ctx);
        if (_CollectErrors(&errors, &condition)) {
            continue;
        }
        else if (!condition.value.IsHolding<bool>()) {
            errors.push_back(
                _FormatFunctionError<LogicalNode<Operator>>(
                    TfStringPrintf(
                        "Invalid type %s for argument %zu", 
                        GetValueTypeName(condition.value).c_str(), i)));
        }
        else {
            if (result.IsEmpty()) {
                result = condition.value.UncheckedGet<bool>();
            }
            else {
                result = Operator<bool>()(
                    result.UncheckedGet<bool>(),
                    condition.value.UncheckedGet<bool>());
            }
        }
    };

    return errors.empty() ?
        EvalResult::Value(std::move(result)) : 
        EvalResult::Error(std::move(errors));
}
    
#define DEFINE_LOGICAL_NODE(LogicType, FunctionName)                \
template class LogicalNode<LogicType>;                              \
template <> const char*                                             \
LogicalNode<LogicType>::GetFunctionName() { return FunctionName; }

DEFINE_LOGICAL_NODE(std::logical_and, "and");
DEFINE_LOGICAL_NODE(std::logical_or, "or");

// ------------------------------------------------------------

const char*
LogicalNotNode::GetFunctionName()
{
    return "not";
}

LogicalNotNode::LogicalNotNode(std::unique_ptr<Node>&& condition)
    : _condition(std::move(condition))
{
}

EvalResult
LogicalNotNode::Evaluate(EvalContext* ctx) const
{
    EvalResult condition = _condition->Evaluate(ctx);
    if (!condition.errors.empty()) {
        return EvalResult::Error(std::move(condition.errors));
    }
    else if (!condition.value.IsHolding<bool>()) {
        return EvalResult::Error({
            _FormatFunctionError<LogicalNotNode>(
                TfStringPrintf(
                    "Invalid type %s for argument",
                    GetValueTypeName(condition.value).c_str()))});
    }

    return EvalResult::Value(!condition.value.UncheckedGet<bool>());
}

// ------------------------------------------------------------

const char* 
ContainsNode::GetFunctionName()
{
    return "contains";
}

ContainsNode::ContainsNode(
    std::unique_ptr<Node>&& searchIn,
    std::unique_ptr<Node>&& searchFor)
    : _searchIn(std::move(searchIn))
    , _searchFor(std::move(searchFor))
{
}

class _ContainsVisitor
{
public:
    _ContainsVisitor(const VtValue& searchFor)
        : _searchFor(searchFor)
    { }

    EvalResult
    operator()(const std::string& searchIn) const
    {
        if (!_searchFor.IsHolding<std::string>()) {
            return Error("Invalid search value");
        }

        return EvalResult::Value(TfStringContains(
            searchIn, _searchFor.UncheckedGet<std::string>()));
    }

    template <class ListType>
    std::enable_if_t<IsSupportedListType<ListType>::value, EvalResult>
    operator()(const ListType& searchIn) const
    {
        using ElemType = typename ListType::value_type;
        if (!_searchFor.IsHolding<ElemType>()) {
            return Error("Invalid search value");
        }

        return EvalResult::Value(std::find(
            searchIn.begin(), searchIn.end(), 
            _searchFor.UncheckedGet<ElemType>()) != searchIn.end());
    }

    template <class T>
    std::enable_if_t<!IsSupportedListType<T>::value, EvalResult>
    operator()(const T& searchIn) const
    {
        return Error("Value to search must be a list or string");
    }

private:
    static EvalResult
    Error(const std::string& err)
    {
        return EvalResult::Error({_FormatFunctionError<ContainsNode>(err)});
    }

    const VtValue& _searchFor;
};

EvalResult
ContainsNode::Evaluate(EvalContext* ctx) const
{
    EvalResult searchIn = _searchIn->Evaluate(ctx);
    EvalResult searchFor = _searchFor->Evaluate(ctx);

    std::vector<std::string> errors = _CombineErrors(&searchIn, &searchFor);
    if (!errors.empty()) {
        return EvalResult::Error(std::move(errors));
    }

    if (searchIn.value.IsHolding<SdfVariableExpression::EmptyList>()) {
        return EvalResult::Value(false);
    }

    return VtVisitValue(searchIn.value, _ContainsVisitor(searchFor.value));
}

// ------------------------------------------------------------

const char* 
AtNode::GetFunctionName()
{
    return "at";
}

AtNode::AtNode(
    std::unique_ptr<Node>&& source,
    std::unique_ptr<Node>&& index)
    : _source(std::move(source))
    , _index(std::move(index))
{
}

class _AtVisitor
{
public:
    _AtVisitor(int64_t index)
        : _index(index)
    { }

    EvalResult
    operator()(const VtValue& v) const
    {
        if (v.IsHolding<SdfVariableExpression::EmptyList>()) {
            return _Error("Index out of range");
        }
        return _Error("Only supported for lists or strings");
    }

    EvalResult
    operator()(const std::string& s) const
    {
        const std::pair<size_t, bool> idx = _NormalizeIndex(s.size());
        if (!idx.second) {
            return _Error("Index out of range");
        }

        return EvalResult::Value(s.substr(idx.first, 1));
    }

    template <class ListType>
    std::enable_if_t<IsSupportedListType<ListType>::value, EvalResult>
    operator()(const ListType& l) const
    {
        const std::pair<size_t, bool> idx = _NormalizeIndex(l.size());
        if (!idx.second) {
            return _Error("Index out of range");
        }

        return EvalResult::Value(l[idx.first]);
    }

    template <class T>
    std::enable_if_t<!IsSupportedListType<T>::value, EvalResult>
    operator()(const T& searchIn) const
    {
        return _Error("Only supported for lists or strings");
    }

private:
    static EvalResult
    _Error(const std::string& err)
    {
        return EvalResult::Error({_FormatFunctionError<AtNode>(err)});
    }

    // Normalize index to support negative indices. Lifted from
    // TfPyNormalizeIndex.
    std::pair<size_t, bool>
    _NormalizeIndex(size_t size) const
    {
        int64_t normalized = _index;
        if (normalized < 0) {
            normalized += size;
        }

        if (normalized < 0 || static_cast<size_t>(normalized) >= size) {
            return std::make_pair(0, false);
        }
        return std::make_pair(static_cast<size_t>(normalized), true);
    }

    int64_t _index;
};

EvalResult
AtNode::Evaluate(EvalContext* ctx) const
{
    EvalResult source = _source->Evaluate(ctx);
    EvalResult index = _index->Evaluate(ctx);

    std::vector<std::string> errors = _CombineErrors(&source, &index);
    if (!errors.empty()) {
        return EvalResult::Error(std::move(errors));
    }

    if (!index.value.IsHolding<int64_t>()) {
        return EvalResult::Error({
            _FormatFunctionError<AtNode>("Index must be an integer")});
    }

    return VtVisitValue(
        source.value, _AtVisitor(index.value.UncheckedGet<int64_t>()));
}

// ------------------------------------------------------------

const char*
LenNode::GetFunctionName()
{
    return "len";
}

LenNode::LenNode(std::unique_ptr<Node>&& source)
    : _source(std::move(source))
{
}

class _LenVisitor
{
public:
    EvalResult
    operator()(const VtValue& v) const
    {
        if (v.IsHolding<SdfVariableExpression::EmptyList>()) {
            return _GetResult(0);
        }
        return _GetUnsupportedTypeError();
    }

    template <class T>
    using CanGetLength = std::integral_constant<
        bool,
        IsSupportedListType<T>::value || std::is_same<T, std::string>::value
    >;

    template <class T>
    std::enable_if_t<CanGetLength<T>::value, EvalResult>
    operator()(const T& v) const
    {
        return _GetResult(v.size());
    }

    template <class T>
    std::enable_if_t<!CanGetLength<T>::value, EvalResult>
    operator()(const T& v) const
    {
        return _GetUnsupportedTypeError();
    }

private:
    EvalResult _GetResult(size_t len) const
    {
        // Explicitly cast to int64_t to ensure the result type
        // holds a value that is supported by the expression 
        // language.
        return EvalResult::Value(static_cast<int64_t>(len));
    }

    EvalResult _GetUnsupportedTypeError() const
    {
        return EvalResult::Error({
            _FormatFunctionError<LenNode>("Unsupported type")});
    }
};

EvalResult
LenNode::Evaluate(EvalContext* ctx) const
{
    EvalResult source = _source->Evaluate(ctx);
    if (!source.errors.empty()) {
        return EvalResult::Error(std::move(source.errors));
    }

    return VtVisitValue(source.value, _LenVisitor());
}

// ------------------------------------------------------------

const char*
DefinedNode::GetFunctionName()
{
    return "defined";
}

DefinedNode::DefinedNode(std::vector<std::unique_ptr<Node>>&& nodes)
    : _nodes(std::move(nodes))
{
}

EvalResult
DefinedNode::Evaluate(EvalContext* ctx) const
{
    VtValue result;
    std::vector<std::string> errors;

    for (size_t i = 0; i < _nodes.size(); ++i) {
        EvalResult varName = _nodes[i]->Evaluate(ctx);
        if (_CollectErrors(&errors, &varName)) {
            continue;
        }
        else if (!varName.value.IsHolding<std::string>()) {
            errors.push_back(
                _FormatFunctionError<DefinedNode>(
                    TfStringPrintf(
                        "Invalid type %s for argument %zu", 
                        GetValueTypeName(varName.value).c_str(), i)));
        }
        else {
            const bool isDefined = 
                ctx->HasVariable(varName.value.UncheckedGet<std::string>());
            result = result.GetWithDefault<bool>(true) && isDefined;
        }
    }

    return errors.empty() ?
        EvalResult::Value(result) :
        EvalResult::Error(std::move(errors));
}

} // end namespace Sdf_VariableExpressionImpl

PXR_NAMESPACE_CLOSE_SCOPE
