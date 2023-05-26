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
#ifndef PXR_USD_SDF_VARIABLE_EXPRESSION_IMPL_H
#define PXR_USD_SDF_VARIABLE_EXPRESSION_IMPL_H

#include "pxr/pxr.h"

#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"

#include <deque>
#include <string>
#include <unordered_set>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace Sdf_VariableExpressionImpl
{

/// \enum ValueType
/// Enumeration of value types supported by variable expressions.
enum class ValueType
{
    Unknown,
    String
};

/// Returns the value type held by \p val. If \p val is empty or is holding
/// a value type that is not supported by variable expressions, returns
/// ValueType::Unknown.
ValueType 
GetValueType(const VtValue& val);

/// \class EvalResult
/// Contains the result of evaluating an expression.
class EvalResult
{
public:
    template <class V>
    static EvalResult Value(V&& value)
    { return { VtValue(std::forward<V>(value)), { } }; }

    static EvalResult NoValue()
    { return { }; }

    static EvalResult Error(std::vector<std::string>&& errors)
    { return { VtValue(), std::move(errors) }; }

    VtValue value;
    std::vector<std::string> errors;
};

/// \class EvalContext
/// Contains information needed when evaluating expressions.
class EvalContext
{
public:
    explicit EvalContext(const VtDictionary* variables);

    /// Returns value of variable named \p var. 
    ///
    /// If the value of \p var is itself an expression, that expression
    /// will be evaluated and the result returned.
    std::pair<EvalResult, bool> GetVariable(const std::string& var);

    /// Returns the set of variables that were queried using GetVariable.
    std::unordered_set<std::string>& GetRequestedVariables()
    { return _requestedVariables; }

private:
    const VtDictionary* _variables;
    std::unordered_set<std::string> _requestedVariables;
    std::deque<std::string> _variableStack;
};

/// \class Node
/// Base class for expression nodes.
class Node
{
public:
    virtual ~Node();
    virtual EvalResult Evaluate(EvalContext* ctx) const = 0;
};

/// \class StringNode
/// Expression node for string values with embedded variable references,
/// e.g. `"a_${VAR}_string"`.
class StringNode
    : public Node
{
public:
    struct Part
    {
        std::string content;
        bool isVariable = false;
    };

    StringNode(std::vector<Part>&& parts);
    EvalResult Evaluate(EvalContext* ctx) const override;

private:
    std::vector<Part> _parts;
};

/// \class VariableNode
/// Expression node for raw variable references, e.g. `${VAR}`.
class VariableNode
    : public Node
{
public:
    VariableNode(std::string&& var);
    EvalResult Evaluate(EvalContext* ctx) const override;

private:
    std::string _var;
};

} // end namespace Sdf_VariableExpressionImpl

PXR_NAMESPACE_CLOSE_SCOPE

#endif
