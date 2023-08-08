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
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace Sdf_VariableExpressionImpl
{

/// \enum ValueType
/// Enumeration of value types supported by variable expressions.
enum class ValueType
{
    Unknown,
    Boolean,
    Integer,
    String,
    List,
    None
};

/// Returns the value type held by \p val. If \p val is empty or is holding
/// a value type that is not supported by variable expressions, returns
/// ValueType::Unknown.
ValueType 
GetValueType(const VtValue& val);

/// Returns a descriptive name for the value type held by \p val.
std::string
GetValueTypeName(const VtValue& val);

/// If \p val contains a type that may be converted to a supported value
/// type, perform the conversion and return the result. If \p val already
/// contains a supported value type or \p val cannot be converted, return an
/// empty VtValue.
VtValue
CoerceIfUnsupportedValueType(const VtValue& val);

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

    /// Returns whether a variable named \p var is defined.
    /// 
    /// This does not examine the value of \p var if it is defined. If
    /// its value is itself an expression, that expression will not be
    /// evaluated.
    bool HasVariable(const std::string& var);

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

/// \class ConstantNode
/// Expression node for simple constant values.
template <class Type>
class ConstantNode
    : public Node
{
public:
    ConstantNode(Type value);
    EvalResult Evaluate(EvalContext* ctx) const override;

private:
    Type _value;
};

using IntegerNode = ConstantNode<int64_t>;
using BoolNode = ConstantNode<bool>;

/// \class NoneNode
/// Expression node for special 'None' value.
class NoneNode
    : public Node
{
public:
    NoneNode();
    EvalResult Evaluate(EvalContext* ctx) const override;
};

/// \class ListNode
/// Expression node for lists of values.
class ListNode
    : public Node
{
public:
    ListNode(std::vector<std::unique_ptr<Node>>&& elements);
    EvalResult Evaluate(EvalContext *ctx) const override;

private:
    std::vector<std::unique_ptr<Node>> _elements;
};

/// \class FunctionWithNArgsNode
/// Base class for nodes representing functions that require
/// a specific number of arguments.
template <size_t N>
class FunctionWithNArgsNode
    : public Node
{
public:
    static constexpr bool IsVariadic = false;
    static constexpr size_t NumArgs = N;

protected:
    FunctionWithNArgsNode() = default;
};

/// \class FunctionWithAtLeastNArgsNode
/// Base class for nodes representing functions that require
/// at least N arguments.
template <size_t N>
class FunctionWithAtLeastNArgsNode
    : public Node
{
public:
    static constexpr bool IsVariadic = true;
    static constexpr auto MinNumArgs = N;

protected:
    FunctionWithAtLeastNArgsNode() = default;
};

/// \class IfNode
/// Base class for conditional function node.
class IfNode
{
public:
    static const char* GetFunctionName();

protected:
    IfNode() = default;

    static EvalResult
    _Evaluate(
        EvalContext* ctx,
        const std::unique_ptr<Node>& condition,
        const std::unique_ptr<Node>& ifValue,
        const std::unique_ptr<Node>& elseValue);
};

/// \class If2Node
/// Expression node for conditional function that takes two
/// arguments: a condition and a value. The node evaluates
/// to the value if the condition evaluates to True, otherwise
/// it evalutes to None.
class If2Node
    : public IfNode
    , public FunctionWithNArgsNode<2>
{
public:
    If2Node(
        std::unique_ptr<Node>&& condition,
        std::unique_ptr<Node>&& ifValue);

    EvalResult Evaluate(EvalContext* ctx) const override;

private:
    std::unique_ptr<Node> _condition;
    std::unique_ptr<Node> _ifValue;
};

/// \class If2Node
/// Expression node for conditional function that takes three
/// arguments: a condition, an "if" value, and an "else" value. The 
/// node evaluates to the "if" value if the condition evaluates to
/// True, otherwise it evalutes to the "else" value.
class If3Node
    : public IfNode
    , public FunctionWithNArgsNode<3>
{
public:
    If3Node(
        std::unique_ptr<Node>&& condition,
        std::unique_ptr<Node>&& ifValue,
        std::unique_ptr<Node>&& elseValue);

    EvalResult Evaluate(EvalContext* ctx) const override;

private:
    std::unique_ptr<Node> _condition;
    std::unique_ptr<Node> _ifValue;
    std::unique_ptr<Node> _elseValue;
};

/// \class ComparisonNode
/// Expression node for comparison functions.
template <template <typename> class Comparator>
class ComparisonNode
    : public FunctionWithNArgsNode<2>
{
public:
    static const char* GetFunctionName();

    ComparisonNode(std::unique_ptr<Node>&& x, std::unique_ptr<Node>&& y);
    EvalResult Evaluate(EvalContext* ctx) const override;

private:
    std::unique_ptr<Node> _x;
    std::unique_ptr<Node> _y;
};

using EqualNode = ComparisonNode<std::equal_to>;
using NotEqualNode = ComparisonNode<std::not_equal_to>;
using LessNode = ComparisonNode<std::less>;
using LessEqualNode = ComparisonNode<std::less_equal>;
using GreaterNode = ComparisonNode<std::greater>;
using GreaterEqualNode = ComparisonNode<std::greater_equal>;

/// \class LogicalNode
/// Expression node for logic functions.
template <template <typename> class Operator>
class LogicalNode
    : public FunctionWithAtLeastNArgsNode<2>
{
public:
    static const char* GetFunctionName();

    LogicalNode(std::vector<std::unique_ptr<Node>>&& conditions);
    EvalResult Evaluate(EvalContext* ctx) const override;

private:
    std::vector<std::unique_ptr<Node>> _conditions;
};

using LogicalAndNode = LogicalNode<std::logical_and>;
using LogicalOrNode = LogicalNode<std::logical_or>;

/// \class LogicalNotNode
/// Expression node for logical "not" function.
class LogicalNotNode
    : public FunctionWithNArgsNode<1>
{
public:
    static const char* GetFunctionName();

    LogicalNotNode(std::unique_ptr<Node>&& condition);
    EvalResult Evaluate(EvalContext* ctx) const override;

private:
    std::unique_ptr<Node> _condition;
};

/// \class ContainsNode
/// Expression node for function that checks if a list contains a value
/// or if a string contains a substring.
class ContainsNode
    : public FunctionWithNArgsNode<2>
{
public:
    static const char* GetFunctionName();

    ContainsNode(
        std::unique_ptr<Node>&& searchIn,
        std::unique_ptr<Node>&& searchFor);

    EvalResult Evaluate(EvalContext* ctx) const override;

private:
    std::unique_ptr<Node> _searchIn;
    std::unique_ptr<Node> _searchFor;
};

/// \class AtNode
/// Expression node for function that retrieves a value from a list 
/// or a character from a string at a given index.
class AtNode
    : public FunctionWithNArgsNode<2>
{
public:
    static const char* GetFunctionName();

    AtNode(
        std::unique_ptr<Node>&& source,
        std::unique_ptr<Node>&& index);

    EvalResult Evaluate(EvalContext* ctx) const override;

private:
    std::unique_ptr<Node> _source;
    std::unique_ptr<Node> _index;
};

/// \class LenNode
/// Expression node for function that retrieves the number of elements
/// in a list of the number of characters in a string.
class LenNode
    : public FunctionWithNArgsNode<1>
{
public:
    static const char* GetFunctionName();

    LenNode(std::unique_ptr<Node>&& source);
    EvalResult Evaluate(EvalContext* ctx) const override;

private:
    std::unique_ptr<Node> _source;
};

/// \class DefinedNode
/// Expression node for function checking if one or more
/// expression variables have been defined.
class DefinedNode
    : public FunctionWithAtLeastNArgsNode<1>
{
public:
    static const char* GetFunctionName();

    DefinedNode(std::vector<std::unique_ptr<Node>>&& nodes);
    EvalResult Evaluate(EvalContext* ctx) const override;

private:
    std::vector<std::unique_ptr<Node>> _nodes;
};

} // end namespace Sdf_VariableExpressionImpl

PXR_NAMESPACE_CLOSE_SCOPE

#endif
