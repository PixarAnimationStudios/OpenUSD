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

EvalContext::EvalContext(const VtDictionary* variables)
    : _variables(variables)
{
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
        const std::unique_ptr<Node>& element = _elements[i];

        EvalResult r = element->Evaluate(ctx);
        if (!r.errors.empty()) {
            errors.insert(errors.end(),
                std::make_move_iterator(r.errors.begin()),
                std::make_move_iterator(r.errors.end()));
        }

        if (r.value.IsEmpty()) {
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

} // end namespace Sdf_VariableExpressionImpl

PXR_NAMESPACE_CLOSE_SCOPE
