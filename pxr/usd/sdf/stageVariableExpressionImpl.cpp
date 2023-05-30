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
#include "pxr/usd/sdf/stageVariableExpressionImpl.h"

#include "pxr/usd/sdf/stageVariableExpressionParser.h"

#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/visitValue.h"

#include "pxr/base/tf/scoped.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace Sdf_StageVariableExpressionImpl
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

EvalContext::EvalContext(const VtDictionary* stageVariables)
    : _stageVariables(stageVariables)
{
}

std::pair<EvalResult, bool>
EvalContext::GetStageVariable(const std::string& stageVar)
{
    // Check if we have circular stage variable substitutions.
    if (std::find(
            _stageVariableStack.begin(), _stageVariableStack.end(),
            stageVar) != _stageVariableStack.end()) {

        std::vector<std::string> formattedStageVars;
        for (const std::string& s : _stageVariableStack) {
            formattedStageVars.push_back("'" + s + "'");
        }

        return { 
            EvalResult::Error({ 
                TfStringPrintf(
                    "Encountered circular stage variable substitutions: "
                    "[%s, '%s']",
                    TfStringJoin(
                        formattedStageVars.begin(), 
                        formattedStageVars.end(), ", ").c_str(),
                    stageVar.c_str()) }),
            true
        };
    }

    _requestedStageVariables.insert(stageVar);

    const VtValue* value = TfMapLookupPtr(*_stageVariables, stageVar);
    if (!value) {
        return { EvalResult::NoValue(), false };
    }

    // If the stage variable isn't a supported type, return an error.
    if (GetValueType(*value) == ValueType::Unknown) {
        return { 
            EvalResult::Error({
                TfStringPrintf(
                    "Stage variable '%s' has unsupported type %s",
                    stageVar.c_str(), value->GetTypeName().c_str()) }),
            true
        };
    }

    // If the value of the stage variable is itself an expression,
    // parse and evaluate it and return the result.
    if (value->IsHolding<std::string>()) {
        const std::string& strValue = value->UncheckedGet<std::string>();
        if (Sdf_IsStageVariableExpression(strValue)) {
            Sdf_StageVariableExpressionParserResult subExpr =
                Sdf_ParseStageVariableExpression(strValue);
            if (subExpr.expression) {
                _stageVariableStack.push_back(stageVar);
                TfScoped<> popStack(
                    [this]() { _stageVariableStack.pop_back(); });

                return { subExpr.expression->Evaluate(this), true };
            }
            else if (!subExpr.errors.empty()) {
                for (std::string& err : subExpr.errors) {
                    err += TfStringPrintf(
                        " (in stage variable '%s')", stageVar.c_str());
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
        if (!part.isStageVariable) {
            part.content = TfEscapeString(part.content);
        }
    }
}

EvalResult
StringNode::Evaluate(EvalContext* ctx) const
{
    std::string result;
    for (const Part& part : _parts) {
        if (part.isStageVariable) {
            const std::string& stageVariable = part.content;

            EvalResult stageVarResult;
            bool stageVarHasValue = false;
            std::tie(stageVarResult, stageVarHasValue) =
                ctx->GetStageVariable(stageVariable);

            if (!stageVarHasValue) {
                // No value for stage variable. Leave the substitution
                // string in place in case downstream clients want to
                // handle it.
                result += part.content;
            }
            else if (!stageVarResult.value.IsEmpty()) {
                if (stageVarResult.value.IsHolding<std::string>()) {
                    // Substitute the value of the stage variable into the
                    // result string.
                    result += stageVarResult.value.UncheckedGet<std::string>();
                }
                else {
                    // The value of the stage variable was not a string.
                    // Flag an error and abort evaluation.
                    return EvalResult::Error({
                       TfStringPrintf(
                           "String value required for substituting stage "
                           "variable '%s', got %s.",
                           stageVariable.c_str(),
                           stageVarResult.value.GetTypeName().c_str())
                    });
                }
            }
            else if (!stageVarResult.errors.empty()) {
                // There was an error when obtaining the value for the
                // stage variable. For example, the value was itself an
                // expression but could not be evaluated due to a syntax
                // error. In this case we copy the errors to the result
                // and abort evaluation with an error.
                return EvalResult::Error(std::move(stageVarResult.errors));
            }
            else {
                // The stage variable value was empty, but no errors occurred.
                // This could happen if the stage variable was a subexpression
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

StageVariableNode::StageVariableNode(std::string&& stageVar)
    : _stageVar(std::move(stageVar))
{
}

EvalResult
StageVariableNode::Evaluate(EvalContext* ctx) const
{
    std::pair<EvalResult, bool> stageVarResult =
        ctx->GetStageVariable(_stageVar);

    if (!stageVarResult.second) {
        return EvalResult::Error({
            TfStringPrintf("No value for stage var '%s'", _stageVar.c_str())
        });
    }

    return stageVarResult.first;
}

} // end namespace Sdf_StageVariableExpressionImpl

PXR_NAMESPACE_CLOSE_SCOPE
