//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/sdf/booleanExpression.h"

#include "pxr/usd/sdf/booleanExpressionEval.h"
#include "pxr/usd/sdf/booleanExpressionParsing.h"
#include "pxr/usd/sdf/fileIO_Common.h"

#include "pxr/base/tf/registryManager.h"

#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE

using BinaryOperator = SdfBooleanExpression::BinaryOperator;
using UnaryOperator = SdfBooleanExpression::UnaryOperator;

TF_REGISTRY_FUNCTION(TfEnum) {
    // Binary operators
    TF_ADD_ENUM_NAME(BinaryOperator::EqualTo, "==")
    TF_ADD_ENUM_NAME(BinaryOperator::NotEqualTo, "!=")
    TF_ADD_ENUM_NAME(BinaryOperator::LessThan, "<")
    TF_ADD_ENUM_NAME(BinaryOperator::LessThanOrEqualTo, "<=")
    TF_ADD_ENUM_NAME(BinaryOperator::GreaterThan, ">")
    TF_ADD_ENUM_NAME(BinaryOperator::GreaterThanOrEqualTo, ">=")
    TF_ADD_ENUM_NAME(BinaryOperator::And, "&&")
    TF_ADD_ENUM_NAME(BinaryOperator::Or, "||")

    // Unary operators
    TF_ADD_ENUM_NAME(UnaryOperator::Not, "!")
}

// The precedence for each node type, sorted to allow ordered comparisons.
// Used to determine where parenthesis are needed when formatting expressions.
enum class _NodePrecedence {
    Literal,
    Not,
    Comparison,
    And,
    Or,
};

// A helper for outputing a node with optional surrounding parenthesis.
struct _ParensHelper {
    SdfBooleanExpression::_NodeRefPtr node;
    bool needsParenthesis;
};

static std::ostream& operator<<(std::ostream& os, _ParensHelper const& rhs);

// Bundle visitor callbacks together to simplify passing them to nodes.
struct _VisitCallbacks {
    SdfBooleanExpression::VariableVisitor variable;
    SdfBooleanExpression::ConstantVisitor constant;
    SdfBooleanExpression::BinaryVisitor binary;
    SdfBooleanExpression::UnaryVisitor unary;
};

// Sdf_BooleanExpressionNode
//   _ConstantNode:   1.0, "string", true, false
//   _VariableNode : someVar
//   _BinaryOpNode: width > 10.0
//   _UnaryOpNode: !(...)

// The abstract base class for AST nodes.
class SdfBooleanExpression::_Node : public TfSimpleRefBase {
public:
    // Write the receiver to the given stream in a format parsable by the
    // SdfBooleanExpression constructor.
    virtual void WriteToStream(std::ostream& os) const = 0;

    // Indicates the precedence of the node. Used for determining where
    // parenthesis should be inserted when formatting an expression.
    virtual _NodePrecedence Precedence() const = 0;

    // Invokes the appropriate callback based on the type of the receiver.
    virtual void Visit(_VisitCallbacks const& callbacks) const = 0;

    // Recursively collects the set of referenced variable names.
    virtual void CollectVariableNames(std::set<TfToken>& names) const = 0;

    // Helper for formatting child nodes with proper parenthesis based on
    // their relative precedence.
    _ParensHelper Parens(SdfBooleanExpression::_NodeRefPtr const& child) const {
        auto needsParens = Precedence() < child->Precedence();
        return {child, needsParens};
    }

    static SdfBooleanExpression
    Wrap(SdfBooleanExpression::_NodeRefPtr const& node) {
        return {node};
    }
};


namespace {

// Represents a constant value.
class _ConstantNode : public SdfBooleanExpression::_Node {
public:
    static SdfBooleanExpression::_NodeRefPtr New(VtValue const& value) {
        return TfCreateRefPtr(new _ConstantNode(value));
    }

    _ConstantNode(VtValue const& value) : _value(value) {}

    void WriteToStream(std::ostream &os) const override {
        if (_value.CanCast<std::string>()) {
            // Handle quotes and escaped characters
            VtValue value = _value;
            auto string = value.Cast<std::string>().Get<std::string>();
            os << Sdf_FileIOUtility::Quote(string, /*allowTripleQuotes=*/false);
        } else {
            os << std::boolalpha << _value;
        }
    }

    void Visit(_VisitCallbacks const& callbacks) const override {
        callbacks.constant(_value);
    }

    void CollectVariableNames(std::set<TfToken>& names) const override {}

    _NodePrecedence Precedence() const override {
        return _NodePrecedence::Literal;
    }

private:
    VtValue _value;
};

// Represents a variable.
class _VariableNode : public SdfBooleanExpression::_Node {
public:
    static SdfBooleanExpression::_NodeRefPtr
    New(TfToken const& variable) {
        return TfCreateRefPtr(new _VariableNode(variable));
    }

    _VariableNode(TfToken const& variable) : _variable(variable) {}

    void WriteToStream(std::ostream &os) const override {
        os << _variable;
    }

    void Visit(_VisitCallbacks const& callbacks) const override {
        callbacks.variable(_variable);
    }

    void CollectVariableNames(std::set<TfToken>& names) const override {
        names.insert(_variable);
    }

    _NodePrecedence Precedence() const override {
        return _NodePrecedence::Literal;
    }

private:
    TfToken _variable;
};

// Represents an operator applied to two subexpressions.
class _BinaryOpNode : public SdfBooleanExpression::_Node {
public:
    static SdfBooleanExpression::_NodeRefPtr
    New(SdfBooleanExpression::_NodeRefPtr lhs,
        SdfBooleanExpression::_NodeRefPtr rhs,
        BinaryOperator op) {
        return TfCreateRefPtr(new _BinaryOpNode(std::move(lhs), std::move(rhs), op));
    }

    _BinaryOpNode(SdfBooleanExpression::_NodeRefPtr lhs,
                   SdfBooleanExpression::_NodeRefPtr rhs,
                   BinaryOperator op)
                   : _lhs(std::move(lhs)), _rhs(std::move(rhs)), _op(op) {}

    void WriteToStream(std::ostream &os) const override {
        os << Parens(_lhs) << " " << _op << " " << Parens(_rhs);
    }

    void Visit(_VisitCallbacks const& callbacks) const override {
        callbacks.binary(Wrap(_lhs), _op, Wrap(_rhs));
    }

    void CollectVariableNames(std::set<TfToken>& names) const override {
        _lhs->CollectVariableNames(names);
        _rhs->CollectVariableNames(names);
    }

    _NodePrecedence Precedence() const override {
        switch (_op) {
            case BinaryOperator::And:
                return _NodePrecedence::And;
            case BinaryOperator::Or:
                return _NodePrecedence::Or;
            default:
                return _NodePrecedence::Comparison;
        }
    }

private:
    SdfBooleanExpression::_NodeRefPtr _lhs;
    SdfBooleanExpression::_NodeRefPtr _rhs;
    BinaryOperator _op;
};

// Represents an operator applied to a nested expression.
class _UnaryOpNode : public SdfBooleanExpression::_Node {
public:
    static SdfBooleanExpression::_NodeRefPtr
    New(SdfBooleanExpression::_NodeRefPtr inner, UnaryOperator op) {
        return TfCreateRefPtr(new _UnaryOpNode(std::move(inner), op));
    }

    _UnaryOpNode(SdfBooleanExpression::_NodeRefPtr inner, UnaryOperator op)
        : _inner(std::move(inner)), _op(op) {}

    void WriteToStream(std::ostream &os) const override {
        os << _op << Parens(_inner);
    }

    void Visit(_VisitCallbacks const& callbacks) const override {
        callbacks.unary(Wrap(_inner), _op);
    }

    void CollectVariableNames(std::set<TfToken>& names) const override {
        _inner->CollectVariableNames(names);
    }

    _NodePrecedence Precedence() const override {
        return _NodePrecedence::Not;
    }

private:
    SdfBooleanExpression::_NodeRefPtr _inner;
    UnaryOperator _op;
};

} // namespace


SdfBooleanExpression::SdfBooleanExpression(
    std::string const& text)
{
    std::string errorMessage;
    *this = Sdf_ParseBooleanExpression(text,
        &errorMessage);

    _text = text;
    _parseError = errorMessage;
}

SdfBooleanExpression::SdfBooleanExpression(
    _NodeRefPtr const& node)
    : _rootNode(node)
{
}

bool SdfBooleanExpression::IsEmpty() const
{
    return !_rootNode;
}

std::string
SdfBooleanExpression::GetText() const
{
    if (!_text.empty()) {
        return _text;
    }

    std::stringstream s;
    s << *this;
    return s.str();
}

std::string const&
SdfBooleanExpression::GetParseError() const
{
    return _parseError;
}

std::set<TfToken>
SdfBooleanExpression::GetVariableNames() const
{
    std::set<TfToken> names;

    if (_rootNode) {
        _rootNode->CollectVariableNames(names);
    }

    return names;
}

SdfBooleanExpression
SdfBooleanExpression::MakeBinaryOp(SdfBooleanExpression lhs,
    BinaryOperator op,
    SdfBooleanExpression rhs)
{
    return {_BinaryOpNode::New(std::move(lhs._rootNode),
        std::move(rhs._rootNode), op)};
}

SdfBooleanExpression
SdfBooleanExpression::MakeUnaryOp(SdfBooleanExpression expression,
    UnaryOperator op)
{
    return {_UnaryOpNode::New(std::move(expression._rootNode), op)};
}

SdfBooleanExpression
SdfBooleanExpression::MakeVariable(TfToken const& variableName)
{
    return {_VariableNode::New(variableName)};
}

SdfBooleanExpression
SdfBooleanExpression::MakeConstant(VtValue const& value)
{
    return {_ConstantNode::New(value)};
}

void
SdfBooleanExpression::Visit(VariableVisitor variable,
                            ConstantVisitor constant,
                            BinaryVisitor binary,
                            UnaryVisitor unary) const
{
    if (!_rootNode) {
        return;
    }

    _rootNode->Visit({variable, constant, binary, unary});
}

bool
SdfBooleanExpression::Evaluate(VariableCallback const& variableCallback) const
{
    return Sdf_EvalBooleanExpression(*this, variableCallback);
}

SdfBooleanExpression
SdfBooleanExpression::RenameVariables(NameTransform const& transform) const
{
    SdfBooleanExpression result = *this;

    // Handle binary operator with two subexpressions
    auto binary = [&result, transform](SdfBooleanExpression const& lhs,
                                       BinaryOperator op,
                                       SdfBooleanExpression const& rhs) {
        result = SdfBooleanExpression::MakeBinaryOp(
            lhs.RenameVariables(transform), op,
            rhs.RenameVariables(transform));
    };

    // Handle unary op
    auto unary = [&result, transform](SdfBooleanExpression const& inner,
                                      UnaryOperator op) {
        result = SdfBooleanExpression::MakeUnaryOp(
            inner.RenameVariables(transform), op);
    };

    // Handle variable lookup
    auto variable = [&result, transform](TfToken const& name) {
        auto renamed = transform(name);
        result = SdfBooleanExpression::MakeVariable(renamed);
    };

    // Handle constant values
    auto constant = [](VtValue const& value) {
        // Do nothing
    };

    // Recursively apply our rename callbacks
    Visit(variable, constant, binary, unary);

    return result;
}

bool
SdfBooleanExpression::Validate(const std::string &expression,
    std::string *errorMessage)
{
    return Sdf_ValidateBooleanExpression(expression, errorMessage);
}

std::ostream& operator<<(std::ostream& os, SdfBooleanExpression const& rhs)
{
    if (!rhs._text.empty()) {
        // prefer the authored expression if it's available
        os << rhs._text;
    } else if (rhs._rootNode) {
        // otherwise fall back to the node tree
        rhs._rootNode->WriteToStream(os);
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, BinaryOperator const& rhs)
{
    os << TfEnum::GetDisplayName(rhs);
    return os;
}

std::ostream& operator<<(std::ostream& os, UnaryOperator const& rhs)
{
    os << TfEnum::GetDisplayName(rhs);
    return os;
}

std::ostream& operator<<(std::ostream& os, _ParensHelper const& rhs)
{
    const bool emitParens = rhs.needsParenthesis;
    if (emitParens) {
        os << "(";
    }

    rhs.node->WriteToStream(os);

    if (emitParens) {
        os << ")";
    }

    return os;
}

PXR_NAMESPACE_CLOSE_SCOPE
