//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_VARIABLE_EXPRESSION_AST_H
#define PXR_USD_SDF_VARIABLE_EXPRESSION_AST_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/variableExpression.h"

#include <memory>
#include <string>
#include <variant>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace SdfVariableExpressionASTNodes
{

/// \class Node
/// Base class for nodes in the SdfVariableExpression abstract syntax tree.
class Node
{
public:
    SDF_API virtual ~Node();
    SDF_API std::unique_ptr<Node> Clone() const;

    /// Return SdfVariableExpression for this node.
    ///
    /// \see SdfVariableExpressionAST::GetExpression
    SDF_API SdfVariableExpression GetExpression() const;

    /// Return SdfVariableExpression::Builder for this node.
    /// This can be used when creating expressions using the various
    /// SdfVariableExpression API for building expressions without
    /// incurring extra overhead from parsing into an SdfVariableExpression.
    SDF_API SdfVariableExpression::Builder GetExpressionBuilder() const;

    /// \name Node Casts
    /// Return a pointer to this node as the specified type or nullptr
    /// if this node does not represent that type.
    ///
    /// \code
    ///  using namespace SdfVariableExpressionASTNodes;
    ///  Node* n = ...;
    ///  FunctionNode* fnNode = n->As<FunctionNode>();
    ///  if (fnNode) { /* node represents a function */ }
    /// \endcode
    /// @{
    template <class T>
    T* As()
    {
        static_assert(std::is_base_of_v<Node, T>);
        return dynamic_cast<T*>(this);
    }

    template <class T>
    const T* As() const
    {
        static_assert(std::is_base_of_v<Node, T>);
        return dynamic_cast<const T*>(this);
    }
    /// @}

protected:
    Node() = default;

    virtual Node* _Clone() const = 0;
    virtual SdfVariableExpression::Builder _GetExpressionBuilder() const = 0;
};

/// \class NodeList
/// Ordered list of abstract syntax tree nodes.
class NodeList
{
public:
    NodeList() = default;
    NodeList(std::vector<std::unique_ptr<Node>>&& nodes);

    /// Return list of nodes in this collection.
    SDF_API std::vector<Node*> GetNodes();

    /// Return list of nodes in this collection.
    SDF_API std::vector<const Node*> GetNodes() const;

    /// Append a copy of \p node to the end of this collection.
    SDF_API void Append(const Node& node);

    /// Set the node at index \p i to a copy of \p node.
    ///
    /// If \p i is not the index of a node in this list, a coding
    /// error will be emitted and no changes will be made.
    SDF_API void Set(size_t i, const Node& node);

    /// Insert a copy of \p node at index \p i.
    ///
    /// \p i must be in the range [0, GetNodes().size()]. Otherwise,
    /// a coding error will be emitted and no changes will be made
    /// to this list.
    SDF_API void Insert(size_t i, const Node& node);

    /// Remove the node at index \p i.
    ///
    /// If \p i is not the index of a node in this list, a coding
    /// error will be emitted and no changes will be made.
    SDF_API void Remove(size_t i);

    /// Clear all nodes.
    SDF_API void Clear();
    
    /// Copy all nodes in this collection to a new collection.
    SDF_API NodeList Clone() const;

private:
    std::vector<std::unique_ptr<Node>> _nodes;
};

/// \class LiteralNode
/// Abstract syntax tree node representing a literal value.
class LiteralNode
    : public Node
{
public:
    /// \name Value
    /// @{

    /// Variant representing possible literal value types.
    /// std::monostate represents the special "None" value.
    using LiteralValue = std::variant<
        std::monostate,
        bool, int64_t, std::string
    >;

    const LiteralValue& GetValue() const { return _value; }
    void SetValue(const LiteralValue& value) { _value = value; }

    /// @}

private:
    friend class _NodeCreator;

    LiteralNode() = default;

    template <class T>
    LiteralNode(T&& value)
        : _value(std::forward<T>(value))
    { }

    Node* _Clone() const final;
    SdfVariableExpression::Builder _GetExpressionBuilder() const final;

    LiteralValue _value;
};

/// \class VariableNode
/// Abstract syntax tree node representing a raw variable reference,
/// i.e. ${FOO}. Variables may also have a fallback value.  When evaluating
/// a variable, if a fallback value is specified, it will be returned if
/// the variable is not defined in the current context.
/// 
/// This does not include variable references within quoted strings, 
/// i.e. "FOO_${BAR}". See LiteralNode.
class VariableNode
    : public Node
{
public:
    /// \name Variable Name
    /// @{
    const std::string& GetName() const { return _name; }
    void SetName(const std::string& name) { _name = name; }
    /// @}

    /// \name Fallback Value
    /// @{

    /// Returns the fallback value for this variable. If no fallback value
    /// is set, this value will be nullptr.
    Node* GetFallbackValue() { return _fallbackValue.get(); }

    /// \overload
    const Node* GetFallbackValue() const { return _fallbackValue.get(); }

    /// Sets the fallback value for this node. \p node must be a LiteralNode
    /// or a ListNode. If \p node is not one of the allowed types a coding
    /// error will be emitted and no changes will be made.
    SDF_API void SetFallbackValue(const Node& node);

    /// Clears any set fallback value
    void ClearFallbackValue() { _fallbackValue = nullptr; }
    /// @}

private:
    friend class _NodeCreator;

    VariableNode(std::string&& varName, 
                 std::unique_ptr<Node>&& fallbackValue) 
        : _name(std::move(varName)),
          _fallbackValue(std::move(fallbackValue))
    { }

    Node* _Clone() const final;
    SdfVariableExpression::Builder _GetExpressionBuilder() const final;

    std::string _name;
    std::unique_ptr<Node> _fallbackValue;
};

/// \class ListNode
/// Abstract syntax tree node representing a list.
class ListNode
    : public Node
{
public:
    SDF_API SdfVariableExpression::ListBuilder GetExpressionBuilder() const;

    /// \name List Elements
    /// @{
    NodeList& GetElements() { return _elems; }
    const NodeList& GetElements() const { return _elems; }
    /// @}

private:
    friend class _NodeCreator;

    ListNode(NodeList&& elems)
        : _elems(std::move(elems))
    { }

    Node* _Clone() const final;
    SdfVariableExpression::Builder _GetExpressionBuilder() const final;

    NodeList _elems;
};

/// \class FunctionNode
/// Abstract syntax tree node representing a function call.
class FunctionNode
    : public Node
{
public:
    SDF_API SdfVariableExpression::FunctionBuilder GetExpressionBuilder() const;

    /// \name Function Name
    /// @{
    const std::string& GetName() const { return _functionName; }
    void SetName(const std::string& name) { _functionName = name; }
    /// @}

    /// \name Function Arguments
    /// @{
    const NodeList& GetArguments() const { return _functionArgs; }
    NodeList& GetArguments() { return _functionArgs; }
    /// @}

private:
    friend class _NodeCreator;

    FunctionNode(std::string&& functionName, NodeList&& functionArgs)
        : _functionName(std::move(functionName))
        , _functionArgs(std::move(functionArgs))
    { }

    Node* _Clone() const final;
    SdfVariableExpression::Builder _GetExpressionBuilder() const final;

    std::string _functionName;
    NodeList _functionArgs;
};

} // end namespace SdfVariableExpressionASTNodes

/// \class SdfVariableExpressionAST
/// Abstract syntax tree for an SdfVariableExpression. Provides read-only
/// inspection of the structure of a variable expression.
class SdfVariableExpressionAST
{
public:
    /// Construct an invalid AST.
    SDF_API SdfVariableExpressionAST();

    /// Construct the AST for the variable expression \p expr. Creates an
    /// invalid AST if \p expr cannot be parsed.
    SDF_API explicit SdfVariableExpressionAST(const std::string& expr);

    /// \overload
    /// Equivalent to SdfVariableExpressionAST(expr.GetString());
    SDF_API
    explicit SdfVariableExpressionAST(const SdfVariableExpression& expr);

    /// Copy constructor. Performs a deep copy of the AST in \p rhs.
    SDF_API SdfVariableExpressionAST(const SdfVariableExpressionAST& rhs);

    /// Assignment. Performs a deep copy of the AST in \p rhs.
    SDF_API
    SdfVariableExpressionAST& operator=(const SdfVariableExpressionAST& rhs);

    SdfVariableExpressionAST(SdfVariableExpressionAST&& rhs) = default;
    SdfVariableExpressionAST&
    operator=(SdfVariableExpressionAST&& rhs) = default;

    /// Return true if this is a valid AST, false otherwise.
    SDF_API explicit operator bool() const;

    /// Return list of errors encountered when parsing the expression
    /// passed to the constructor.
    SDF_API const std::vector<std::string>& GetErrors() const;

    /// Return the root node of the syntax tree corresponding to the
    /// outermost expression if this is a valid AST or \c nullptr
    /// otherwise. The returned pointer is valid for the lifetime of
    /// its associated SdfVariableExpressionAST.
    SDF_API SdfVariableExpressionASTNodes::Node* GetRoot();

    /// \overload
    SDF_API const SdfVariableExpressionASTNodes::Node* GetRoot() const;

    /// Return the expression represented by this AST if this is a
    /// valid AST or a default constructed SdfVariableExpression otherwise.
    ///
    /// The expression string may be formatted differently than what
    /// was in the original expression passed to the constructor. 
    /// 
    /// The returned SdfVariableExpression may be an invalid expression
    /// that cannot be evaluated, e.g. if the expression contains an
    /// unrecognized function name. However, calling GetString on the
    /// returned SdfVariableExpression will still return the expression
    /// string for inspection.
    SDF_API SdfVariableExpression GetExpression() const;

private:
    std::unique_ptr<SdfVariableExpressionASTNodes::Node> _node;
    std::vector<std::string> _errors;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
