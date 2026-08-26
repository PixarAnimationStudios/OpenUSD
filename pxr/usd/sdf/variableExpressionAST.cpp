//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/sdf/variableExpression.h"
#include "pxr/usd/sdf/variableExpressionAST.h"
#include "pxr/usd/sdf/variableExpressionParser.h"

PXR_NAMESPACE_OPEN_SCOPE

using namespace Sdf_VariableExpressionImpl;

namespace SdfVariableExpressionASTNodes
{

Node::~Node() = default;

std::unique_ptr<Node>
Node::Clone() const
{
    return std::unique_ptr<Node>(_Clone());
}

SdfVariableExpression
Node::GetExpression() const
{
    return _GetExpressionBuilder();
}

SdfVariableExpression::Builder
Node::GetExpressionBuilder() const
{
    return _GetExpressionBuilder();
}

NodeList::NodeList(std::vector<std::unique_ptr<Node>>&& nodes)
    : _nodes(std::move(nodes))
{
}

std::vector<Node*>
NodeList::GetNodes()
{
    std::vector<Node*> nodes(_nodes.size());
    std::transform(
        _nodes.begin(), _nodes.end(), nodes.begin(),
        [](auto&& n) { return n.get(); });
    return nodes;
}

std::vector<const Node*>
NodeList::GetNodes() const
{
    std::vector<const Node*> nodes(_nodes.size());
    std::transform(
        _nodes.begin(), _nodes.end(), nodes.begin(),
        [](auto&& n) { return n.get(); });
    return nodes;
}

void
NodeList::Append(const Node& n)
{
    _nodes.push_back(n.Clone());
}

void
NodeList::Set(size_t i, const Node& n)
{
    if (i >= _nodes.size()) {
        TF_CODING_ERROR("Invalid index %zu", i);
        return;
    }

    _nodes[i] = n.Clone();
}

void
NodeList::Insert(size_t i, const Node& n)
{
    if (i > _nodes.size()) {
        TF_CODING_ERROR("Invalid index %zu", i);
        return;
    }

    _nodes.insert(_nodes.begin() + i, n.Clone());
}

void
NodeList::Remove(size_t i)
{
    if (i >= _nodes.size()) {
        TF_CODING_ERROR("Invalid index %zu", i);
        return;
    }

    _nodes.erase(_nodes.begin() + i);
}

void
NodeList::Clear()
{
    _nodes.clear();
}

NodeList
NodeList::Clone() const
{
    return NodeList(
        [this]() {
            std::vector<std::unique_ptr<Node>> dst;
            dst.reserve(_nodes.size());
            for (const auto& e : _nodes) {
                dst.push_back(e->Clone());
            }
            return dst;
        }());
}

Node*
LiteralNode::_Clone() const
{
    LiteralNode* result = new LiteralNode;
    result->_value = _value;
    return result;
}

SdfVariableExpression::Builder
LiteralNode::_GetExpressionBuilder() const
{
    return std::visit(
        [](auto&& arg) -> SdfVariableExpression::Builder {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return SdfVariableExpression::MakeNone();
            }
            else {
                return SdfVariableExpression::MakeLiteral(arg);
            }
        }, _value);
}

void
VariableNode::SetFallbackValue(const Node& node) {
    // A fallback may be a literal or a list. This mirrors the types accepted
    // by the parser's VariableFallbackValue rule.
    if (const LiteralNode* literalNode = node.As<LiteralNode>()) {
        if (std::holds_alternative<std::monostate>(literalNode->GetValue())) {
            TF_CODING_ERROR("VariableNode: fallback value should not be None.");
            return;
        }
    }
    else if (!node.As<ListNode>()) {
        TF_CODING_ERROR(
            "VariableNode: fallback must be a literal or list.");
        return;
    }

    _fallbackValue = node.Clone();
}

Node*
VariableNode::_Clone() const
{
    std::unique_ptr<Node> fallbackClone = _fallbackValue ?
        _fallbackValue->Clone() : nullptr;
    return new VariableNode(std::string(_name), std::move(fallbackClone));
}

SdfVariableExpression::Builder
VariableNode::_GetExpressionBuilder() const
{
    return _fallbackValue ? 
        SdfVariableExpression::MakeVariable(_name, 
            _fallbackValue->GetExpressionBuilder()) :
        SdfVariableExpression::MakeVariable(_name);
}

SdfVariableExpression::ListBuilder
ListNode::GetExpressionBuilder() const
{
    SdfVariableExpression::ListBuilder builder = 
        SdfVariableExpression::MakeList();

    for (const auto& node : _elems.GetNodes()) {
        builder.AddElement(node->GetExpressionBuilder());
    }

    return builder;
}

Node*
ListNode::_Clone() const
{
    return new ListNode(_elems.Clone());
}

SdfVariableExpression::Builder
ListNode::_GetExpressionBuilder() const
{
    // Calls shadowing function defined on this class.
    return GetExpressionBuilder();
}

SdfVariableExpression::FunctionBuilder
FunctionNode::GetExpressionBuilder() const
{
    SdfVariableExpression::FunctionBuilder builder =
        SdfVariableExpression::MakeFunction(_functionName);

    for (const auto& node : _functionArgs.GetNodes()) {
        builder.AddArgument(node->GetExpressionBuilder());
    }

    return builder;
}

Node*
FunctionNode::_Clone() const
{
    return new FunctionNode(
        std::string(_functionName), _functionArgs.Clone());
}

SdfVariableExpression::Builder
FunctionNode::_GetExpressionBuilder() const
{
    // Calls shadowing function defined on this class.
    return GetExpressionBuilder();
}

} // end namespace SdfVariableExpressionASTNodes

SdfVariableExpressionAST::SdfVariableExpressionAST()
{
    _errors.push_back("No expression specified");
}

SdfVariableExpressionAST::SdfVariableExpressionAST(
    const std::string& expr)
{
    using ParserResult = Sdf_VariableExpressionASTParserResult;

    ParserResult parseResult = Sdf_ParseVariableExpressionAST(expr);
    _node.reset(parseResult.astRoot.release());
    _errors = std::move(parseResult.errors);
}

SdfVariableExpressionAST::SdfVariableExpressionAST(
    const SdfVariableExpression& expr)
    : SdfVariableExpressionAST(expr.GetString())
{
}

SdfVariableExpressionAST::SdfVariableExpressionAST(
    const SdfVariableExpressionAST& rhs)
    : _node(rhs._node ? rhs._node->Clone() : decltype(_node){})
    , _errors(rhs._errors)
{
}

SdfVariableExpressionAST&
SdfVariableExpressionAST::operator=(const SdfVariableExpressionAST& rhs)
{
    if (this != &rhs) {
        *this = SdfVariableExpressionAST(rhs);
    }
    return *this;
}

SdfVariableExpressionAST::operator bool() const
{
    return static_cast<bool>(_node);
}

const std::vector<std::string>&
SdfVariableExpressionAST::GetErrors() const
{
    return _errors;
}

SdfVariableExpressionASTNodes::Node*
SdfVariableExpressionAST::GetRoot()
{
    return _node.get();
}

const SdfVariableExpressionASTNodes::Node*
SdfVariableExpressionAST::GetRoot() const
{
    return _node.get();
}

SdfVariableExpression
SdfVariableExpressionAST::GetExpression() const
{
    return _node ? _node->GetExpression() : SdfVariableExpression();
}

PXR_NAMESPACE_CLOSE_SCOPE
