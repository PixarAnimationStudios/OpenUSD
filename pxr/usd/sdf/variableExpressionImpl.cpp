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

#include "pxr/usd/sdf/variableExpressionParser.h"

#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/visitValue.h"

#include "pxr/base/tf/scoped.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace Sdf_VariableExpressionImpl
{

class _TypeVisitor
{
public:
    ValueType operator()(const std::string&) const {
        return ValueType::String;
    }

    template <class T>
    ValueType operator()(const T&) const {
        return ValueType::Unknown;
    }
};

ValueType
GetValueType(const VtValue& value)
{
    return VtVisitValue(value, _TypeVisitor());
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

    // If the variable isn't a supported type, return an error.
    if (GetValueType(*value) == ValueType::Unknown) {
        return { 
            EvalResult::Error({
                TfStringPrintf(
                    "Variable '%s' has unsupported type %s",
                    var.c_str(), value->GetTypeName().c_str()) }),
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
                           varResult.value.GetTypeName().c_str())
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

} // end namespace Sdf_VariableExpressionImpl

PXR_NAMESPACE_CLOSE_SCOPE
