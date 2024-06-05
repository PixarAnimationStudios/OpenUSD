//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_MAP_EXPRESSION_H
#define PXR_USD_PCP_MAP_EXPRESSION_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/mapFunction.h"

#include "pxr/base/tf/delegatedCountPtr.h"

#include <tbb/spin_mutex.h>

#include <atomic>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

/// \class PcpMapExpression
///
/// An expression that yields a PcpMapFunction value.
///
/// Expressions comprise constant values, variables, and operators
/// applied to sub-expressions.  Expressions cache their computed values
/// internally.  Assigning a new value to a variable automatically
/// invalidates the cached values of dependent expressions.  Common
/// (sub-)expressions are automatically detected and shared.
///
/// PcpMapExpression exists solely to support efficient incremental
/// handling of relocates edits.  It represents a tree of the namespace
/// mapping operations and their inputs, so we can narrowly redo the
/// computation when one of the inputs changes.
///
class PcpMapExpression
{
public:
    /// The value type of PcpMapExpression is a PcpMapFunction.
    typedef PcpMapFunction Value;

    /// Evaluate this expression, yielding a PcpMapFunction value.
    /// The computed result is cached.
    /// The return value is a reference to the internal cached value.
    /// The cache is automatically invalidated as needed.
    PCP_API
    const Value & Evaluate() const;

    /// Default-construct a NULL expression.
    PcpMapExpression() noexcept = default;

    ~PcpMapExpression() noexcept = default;

    /// Swap this expression with the other.
    void Swap(PcpMapExpression &other) noexcept {
        _node.swap(other._node);
    }

    /// Return true if this is a null expression.
    bool IsNull() const noexcept {
        return !_node;
    }

    /// \name Creating expressions
    /// @{

    /// Return an expression representing PcpMapFunction::Identity().
    PCP_API
    static PcpMapExpression Identity();

    /// Create a new constant.
    PCP_API
    static PcpMapExpression Constant( const Value & constValue );

    /// A Variable is a mutable memory cell that holds a value.
    /// Changing a variable's value invalidates any expressions using
    /// that variable.
    class Variable {
        Variable(Variable const &) = delete;
        Variable &operator=(Variable const &) = delete;
    public:
        Variable() = default;
        virtual ~Variable();
        /// Return the current value.
        virtual const Value & GetValue() const = 0;
        /// Mutate the variable to have the new value.
        /// This will also invalidate dependant expressions.
        virtual void SetValue(Value && value) = 0;
        /// Return an expression representing the value of this variable.
        /// This lets you use the variable as a sub-term in other expressions.
        virtual PcpMapExpression GetExpression() const = 0;
    };

    /// Variables are held by reference.
    typedef std::unique_ptr<Variable> VariableUniquePtr;

    /// Create a new variable.
    /// The client is expected to retain the reference for as long as
    /// it wishes to continue being able to set the value of the variable.
    /// After the reference is dropped, expressions using the variable
    /// will continue to be valid, but there will be no way to further
    /// change the value of the variable.
    PCP_API
    static VariableUniquePtr NewVariable(Value && initialValue);

    /// Create a new PcpMapExpression representing the application of
    /// f's value, followed by the application of this expression's value.
    PCP_API
    PcpMapExpression Compose(const PcpMapExpression &f) const;

    /// Create a new PcpMapExpression representing the inverse of f.
    PCP_API
    PcpMapExpression Inverse() const;

    /// Return a new expression representing this expression with an added
    /// (if necessary) mapping from </> to </>.
    PCP_API
    PcpMapExpression AddRootIdentity() const;

    /// Return true if the map function is the constant identity function.
    bool IsConstantIdentity() const {
        return _node && _node->key.op == _OpConstant &&
            _node->key.valueForConstant.IsIdentity();
    }

    /// @}

    /// \name Convenience API
    /// The following API just forwards through to the underlying evaluated
    /// mapfunction value.
    /// @{

    /// Return true if the evaluated map function is the identity function.
    /// For identity, MapSourceToTarget() always returns the path unchanged.
    bool IsIdentity() const {
        return Evaluate().IsIdentity();
    }

    /// Map a path in the source namespace to the target.
    /// If the path is not in the domain, returns an empty path.
    SdfPath MapSourceToTarget(const SdfPath &path) const {
        return Evaluate().MapSourceToTarget(path);
    }

    /// Map a path in the target namespace to the source.
    /// If the path is not in the co-domain, returns an empty path.
    SdfPath MapTargetToSource(const SdfPath &path) const {
        return Evaluate().MapTargetToSource(path);
    }

    /// The time offset of the mapping.
    const SdfLayerOffset &GetTimeOffset() const {
        return Evaluate().GetTimeOffset();
    }

    /// Returns a string representation of this mapping for debugging
    /// purposes.
    std::string GetString() const {
        return Evaluate().GetString();
    }

    /// @}

private:
    // Allow Pcp_Statistics access to internal data for diagnostics.
    friend class Pcp_Statistics;
    friend struct Pcp_VariableImpl;

    class _Node;
    using _NodeRefPtr = TfDelegatedCountPtr<_Node>;

    explicit PcpMapExpression(const _NodeRefPtr & node) : _node(node) {}

private: // data
    enum _Op {
        _OpConstant,
        _OpVariable,
        _OpInverse,
        _OpCompose,
        _OpAddRootIdentity
    };

    class _Node {
        _Node(const _Node&) = delete;
        _Node& operator=(const _Node&) = delete;

        // Ref-counting ops manage _refCount.
        // Need to friend them here to have access to _refCount.
        friend PCP_API void TfDelegatedCountIncrement(_Node*);
        friend PCP_API void TfDelegatedCountDecrement(_Node*) noexcept;
    public:
        // The Key holds all the state needed to uniquely identify
        // this (sub-)expression.
        struct Key {
            const _Op op;
            const _NodeRefPtr arg1, arg2;
            const Value valueForConstant;

            Key( _Op op_,
                 const _NodeRefPtr & arg1_,
                 const _NodeRefPtr & arg2_,
                 const Value & valueForConstant_ )
                : op(op_)
                , arg1(arg1_)
                , arg2(arg2_)
                , valueForConstant(valueForConstant_)
            {}
            inline size_t GetHash() const;
            bool operator==(const Key &key) const;
        };

        // The Key of a node is const, and established when it is created.
        const Key key;

        // Whether or not the expression tree up to and including this node
        // will always include an identity mapping.
        const bool expressionTreeAlwaysHasIdentity;

        // Factory method to create new nodes.
        static _NodeRefPtr
        New( _Op op,
             const _NodeRefPtr & arg1 = _NodeRefPtr(),
             const _NodeRefPtr & arg2 = _NodeRefPtr(),
             const Value & valueForConstant = Value() );
        ~_Node();

        // Evaluate (and internally cache) the value of this node.
        const Value & EvaluateAndCache() const;

        // For _OpVariable nodes, sets the variable's value.
        void SetValueForVariable(Value &&newValue);

        // For _OpVariable nodes, returns the variable's value.
        const Value & GetValueForVariable() const {
            return _valueForVariable;
        }

    private:
        explicit _Node( const Key &key_ );
        void _Invalidate();
        Value _EvaluateUncached() const;

        // Helper to determine if the expression tree indicated by key
        // will always contains the root identity.
        static bool _ExpressionTreeAlwaysHasIdentity(const Key& key);

        // Registry of node instances, identified by Key.
        // Note: variable nodes are not tracked by the registry.
        struct _NodeMap;
        static TfStaticData<_NodeMap> _nodeRegistry;

        mutable std::atomic<int> _refCount;
        mutable Value _cachedValue;
        mutable std::set<_Node*> _dependentExpressions;
        Value _valueForVariable;
        mutable tbb::spin_mutex _mutex;
        mutable std::atomic<bool> _hasCachedValue;
    };

    // Need to friend them here to have visibility to private class _Node.
    friend PCP_API void TfDelegatedCountIncrement(_Node*);
    friend PCP_API void TfDelegatedCountDecrement(_Node*) noexcept;

    _NodeRefPtr _node;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_MAP_EXPRESSION_H
