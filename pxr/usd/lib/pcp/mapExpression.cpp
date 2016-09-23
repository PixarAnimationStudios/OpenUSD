//
// Copyright 2016 Pixar
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
#include "pxr/usd/pcp/mapExpression.h"
#include "pxr/usd/pcp/mapFunction.h"
#include "pxr/usd/pcp/layerStack.h"

#include "pxr/base/tracelite/trace.h"

#include <tbb/concurrent_hash_map.h>

struct Pcp_VariableImpl;

// Add a mapping from </> to </> if the function does not already have one.
static PcpMapFunction
_AddRootIdentity(const PcpMapFunction &value)
{
    static const SdfPath absoluteRoot = SdfPath::AbsoluteRootPath();
    if (value.MapSourceToTarget(absoluteRoot) == absoluteRoot) {
        // This function already maps </> to </>; use it as-is.
        return value;
    }
    // Re-create the function with an added root identity mapping.
    PcpMapFunction::PathMap sourceToTargetMap = value.GetSourceToTargetMap();
    sourceToTargetMap[absoluteRoot] = absoluteRoot;
    return PcpMapFunction::Create(sourceToTargetMap, value.GetTimeOffset());
}

////////////////////////////////////////////////////////////////////////

PcpMapExpression::PcpMapExpression()
{
}

bool
PcpMapExpression::IsNull() const
{
    return not _node;
}

void
PcpMapExpression::Swap(PcpMapExpression &other)
{
    _node.swap(other._node);
}

const PcpMapExpression::Value &
PcpMapExpression::Evaluate() const
{
    static PcpMapExpression::Value defaultValue;
    return _node ? _node->EvaluateAndCache() : defaultValue;
}

PcpMapExpression
PcpMapExpression::Identity()
{
    static const PcpMapExpression val = Constant(PcpMapFunction::Identity());
    return val;
}

PcpMapExpression
PcpMapExpression::Constant( const Value & value )
{
    return PcpMapExpression( _Node::New(_OpConstant, _NodeRefPtr(),
                                        _NodeRefPtr(), value) );
}

PcpMapExpression
PcpMapExpression::Compose(const PcpMapExpression &f) const
{
    if (_node->key.op == _OpConstant and f._node->key.op == _OpConstant) {
        // Apply constant folding
        return Constant( Evaluate().Compose( f.Evaluate() ) );
    }
    return PcpMapExpression( _Node::New(_OpCompose, _node, f._node) );
}

PcpMapExpression
PcpMapExpression::Inverse() const
{
    if (_node->key.op == _OpConstant) {
        // Apply constant folding
        return Constant( Evaluate().GetInverse() );
    }
    return PcpMapExpression( _Node::New(_OpInverse, _node) );
}

PcpMapExpression
PcpMapExpression::AddRootIdentity() const
{
    if (_node->key.op == _OpConstant) {
        // Apply constant folding
        return Constant( _AddRootIdentity(Evaluate()) );
    }
    if (_node->expressionTreeAlwaysHasIdentity) {
        return PcpMapExpression(_node);
    }

    return PcpMapExpression( _Node::New(_OpAddRootIdentity, _node) );
}

////////////////////////////////////////////////////////////////////////
// Variable implementation

PcpMapExpression::Variable::~Variable()
{
    // Do nothing
}

// Private implementation for Variable.
struct Pcp_VariableImpl : PcpMapExpression::Variable
{
    virtual ~Pcp_VariableImpl() {}

    Pcp_VariableImpl(const PcpMapExpression::_NodeRefPtr &node) : _node(node) {}

    virtual const PcpMapExpression::Value & GetValue() const {
        return _node->GetValueForVariable();
    }

    virtual void SetValue(const PcpMapExpression::Value & value) {
        _node->SetValueForVariable(value);
    }

    virtual PcpMapExpression GetExpression() const {
        return PcpMapExpression(_node);
    }

    const PcpMapExpression::_NodeRefPtr _node;
};

PcpMapExpression::VariableRefPtr
PcpMapExpression::NewVariable( const Value & initialValue )
{
    Pcp_VariableImpl *var = new Pcp_VariableImpl( _Node::New(_OpVariable) );

    var->SetValue(initialValue);

    return VariableRefPtr(var);
}

////////////////////////////////////////////////////////////////////////
// Node

namespace {

template <class Key>
struct _KeyHashEq
{
    inline bool equal(const Key &l, const Key &r) const { return l == r; }
    inline size_t hash(const Key &k) const { return k.GetHash(); }
};

} // anon

struct PcpMapExpression::_Node::_NodeMap
{
    typedef PcpMapExpression::_Node::Key Key;
    typedef tbb::concurrent_hash_map<
        Key, PcpMapExpression::_Node *, _KeyHashEq<Key> > MapType;
    typedef MapType::accessor accessor;
    MapType map;
};

TfStaticData<PcpMapExpression::_Node::_NodeMap>
PcpMapExpression::_Node::_nodeRegistry;

bool 
PcpMapExpression::_Node::_ExpressionTreeAlwaysHasIdentity(const Key& key)
{
    switch (key.op) {
    case _OpAddRootIdentity:
        return true;
        
    case _OpVariable:
        return false;

    case _OpConstant:
        {
            // Check if this maps </> back to </> -- in which case this
            // has a root identity mapping.
            SdfPath absRoot = SdfPath::AbsoluteRootPath();
            return key.valueForConstant.MapSourceToTarget(absRoot) == absRoot;
        }

    case _OpCompose:
        // Composing two map expressions may cause the identity
        // mapping to be removed; consider the case where we compose
        // {</>:</>, </A>:</B>} and {</B>:</C>}. The expected result
        // is {</A>:</C>}. 
        //
        // In this case, the expression tree will only have an identity
        // mapping if *both* subtrees being composed have an identity.
        return (key.arg1 and key.arg1->expressionTreeAlwaysHasIdentity and
                key.arg2 and key.arg2->expressionTreeAlwaysHasIdentity);

    default:
        // For any other operation, if either of the subtrees has an
        // identity mapping, so does this tree.
        return (key.arg1 and key.arg1->expressionTreeAlwaysHasIdentity) or
               (key.arg2 and key.arg2->expressionTreeAlwaysHasIdentity);
    }
}

PcpMapExpression::_NodeRefPtr
PcpMapExpression::_Node::New( _Op op_,
                              const _NodeRefPtr & arg1_,
                              const _NodeRefPtr & arg2_,
                              const Value & valueForConstant_ )
{
    TfAutoMallocTag2 tag("Pcp", "PcpMapExpresion");
    const Key key(op_, arg1_, arg2_, valueForConstant_);

    if (key.op != _OpVariable) {
        // Check for existing instance to re-use
        _NodeMap::accessor accessor;
        if (_nodeRegistry->map.insert(accessor, key) or
            accessor->second->_refCount.fetch_and_increment() == 0) {
            // Either there was no node in the table, or there was but it had
            // begun dying (another client dropped its refcount to 0).  We have
            // to create a new node in the table.  When the client that is
            // killing the other node it looks for itself in the table, it will
            // either not find itself or will find a different node and so won't
            // remove it.
            _NodeRefPtr newNode(new _Node(key));
            accessor->second = newNode.get();
            return newNode;
        }
        return _NodeRefPtr(accessor->second, /*add_ref =*/ false);
    }
    return _NodeRefPtr(new _Node(key));
}

PcpMapExpression::_Node::_Node( const Key & key_ )
    : key(key_)
    , expressionTreeAlwaysHasIdentity(_ExpressionTreeAlwaysHasIdentity(key))
{
    _refCount = 0;
    if (key.arg1) {
        tbb::spin_mutex::scoped_lock lock(key.arg1->_mutex);
        key.arg1->_dependentExpressions.insert(this);
    }
    if (key.arg2) {
        tbb::spin_mutex::scoped_lock lock(key.arg2->_mutex);
        key.arg2->_dependentExpressions.insert(this);
    }
}

PcpMapExpression::_Node::~_Node()
{
    if (key.arg1) {
        tbb::spin_mutex::scoped_lock lock(key.arg1->_mutex);
        key.arg1->_dependentExpressions.erase(this);
    }
    if (key.arg2) {
        tbb::spin_mutex::scoped_lock lock(key.arg2->_mutex);
        key.arg2->_dependentExpressions.erase(this);
    }

    if (key.op != _OpVariable) {
        // Remove from node map if present.
        _NodeMap::accessor accessor;
        if (_nodeRegistry->map.find(accessor, key) and
            accessor->second == this) {
            _nodeRegistry->map.erase(accessor);
        }
    }
}

const PcpMapExpression::Value &
PcpMapExpression::_Node::EvaluateAndCache() const
{
    if (not _cachedValue) {
        TRACE_SCOPE("PcpMapExpression::_Node::EvaluateAndCache - cache miss");
        _cachedValue.reset(_EvaluateUncached());
    }
    return _cachedValue.get();
}

PcpMapExpression::Value
PcpMapExpression::_Node::_EvaluateUncached() const
{
    switch(key.op) {
    case _OpConstant:
        return key.valueForConstant;
    case _OpVariable:
        return _valueForVariable;
    case _OpInverse:
        return key.arg1->EvaluateAndCache().GetInverse();
    case _OpCompose:
        return key.arg1->EvaluateAndCache()
            .Compose(key.arg2->EvaluateAndCache());
    case _OpAddRootIdentity:
        return _AddRootIdentity(key.arg1->EvaluateAndCache());
    default:
        TF_VERIFY(false, "unhandled case");
        return PcpMapFunction();
    }
}

void
PcpMapExpression::_Node::_Invalidate()
{
    if (_cachedValue) {
        _cachedValue.reset();
        TF_FOR_ALL(dep, _dependentExpressions) {
            (*dep)->_Invalidate();
        }
    } else {
        // This node is already invalid so dependent nodes are already invalid.
    }
}

void
PcpMapExpression::_Node::SetValueForVariable(const Value & value)
{
    if (key.op != _OpVariable) {
        TF_CODING_ERROR("Cannot set value for non-variable");
        return;
    }
    if (_valueForVariable != value) {
        _valueForVariable = value;
        _Invalidate();
    }
}

inline size_t
PcpMapExpression::_Node::Key::GetHash() const
{
    size_t hash = op;
    boost::hash_combine(hash, boost::get_pointer(arg1));
    boost::hash_combine(hash, boost::get_pointer(arg2));
    boost::hash_combine(hash, valueForConstant);
    return hash;
}

bool
PcpMapExpression::_Node::Key::operator==(const Key &key) const
{
    return op == key.op
        and arg1 == key.arg1
        and arg2 == key.arg2
        and valueForConstant == key.valueForConstant;
}

void
intrusive_ptr_add_ref(PcpMapExpression::_Node* p)
{
    ++p->_refCount;
}

void
intrusive_ptr_release(PcpMapExpression::_Node* p)
{
    if (p->_refCount.fetch_and_decrement() == 1)
        delete p;
}
