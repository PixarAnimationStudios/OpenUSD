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
#ifndef SDF_PATHNODE_H
#define SDF_PATHNODE_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/mallocTag.h"

#include <boost/noncopyable.hpp>
#include <boost/intrusive_ptr.hpp>

#include <tbb/atomic.h>

PXR_NAMESPACE_OPEN_SCOPE

// Sdf_PathNode
//
// This class is the root of the path node hierarchy.  It used to use ordinary
// C++ polymorphism, but it no longer does.  This is primarily a space
// optimization: the set of node types is fixed, we already have an enum 'type'
// field, and we typically have lots (e.g. ~1e8) of these objects.  Dropping the
// C++ polymorphism saves us the vtable pointer, and we can pack the '_nodeType'
// field in a much smaller space with other flags.
class Sdf_PathNode : boost::noncopyable {
public:
    // Node types identify what kind of path node a given instance is.
    // There are restrictions on what type of children each node type 
    // can have,
    enum NodeType {
        RootNode,
            // Allowable child node types:
            //     PrimNode
            //     PrimPropertyNode (only for relative root)
            //     PrimVariantSelectionNode (only for relative root)

        PrimNode,
            // Allowable child node types:
            //     PrimNode
            //     PrimPropertyNode
            //     PrimVariantSelectionNode

        PrimPropertyNode,
            // Allowable child node types:
            //     TargetNode
            //     MapperNode
            //     ExpressionNode

        PrimVariantSelectionNode,
            // Allowable child node types:
            //     PrimNode
            //     PrimPropertyNode
            //     PrimVariantSelectionNode 
            //         (for variants that contain variant sets)

        TargetNode,
            // Allowable child node types:
            //     RelationalAttributeNode (only if parent is PrimPropertyNode)

        RelationalAttributeNode,
            // Allowable child node types:
            //     TargetNode
            //     MapperNode
            //     ExpressionNode

        MapperNode,
            // Allowable child node types:
            //     MapperArgNode

        MapperArgNode,
            // Allowable child node types:
            //     <none>

        ExpressionNode,
            // Allowable child node types:
            //     <none>

        NumNodeTypes ///< Internal sentinel value
    };

    static Sdf_PathNodeConstRefPtr
    FindOrCreatePrim(Sdf_PathNodeConstRefPtr const &parent,
                     const TfToken &name);
    
    static Sdf_PathNodeConstRefPtr
    FindOrCreatePrimProperty(Sdf_PathNodeConstRefPtr const &parent, 
                             const TfToken &name);
    
    static Sdf_PathNodeConstRefPtr
    FindOrCreatePrimVariantSelection(Sdf_PathNodeConstRefPtr const &parent, 
                                     const TfToken &variantSet,
                                     const TfToken &variant);

    static Sdf_PathNodeConstRefPtr
    FindOrCreateTarget(Sdf_PathNodeConstRefPtr const &parent, 
                       Sdf_PathNodeConstRefPtr const &targetPathNode);

    static Sdf_PathNodeConstRefPtr
    FindOrCreateRelationalAttribute(Sdf_PathNodeConstRefPtr const &parent, 
                                    const TfToken &name);

    static Sdf_PathNodeConstRefPtr
    FindOrCreateMapper(Sdf_PathNodeConstRefPtr const &parent, 
                       Sdf_PathNodeConstRefPtr const &targetPathNode);

    static Sdf_PathNodeConstRefPtr
    FindOrCreateMapperArg(Sdf_PathNodeConstRefPtr const &parent, 
                          const TfToken &name);
    
    static Sdf_PathNodeConstRefPtr
    FindOrCreateExpression(Sdf_PathNodeConstRefPtr const &parent);

    static const Sdf_PathNodeConstRefPtr& GetAbsoluteRootNode();
    static const Sdf_PathNodeConstRefPtr& GetRelativeRootNode();

    NodeType GetNodeType() const { return NodeType(_nodeType); }

    void GetPrefixes(SdfPathVector *prefixes, bool includeRoot=false) const;

    static std::pair<Sdf_PathNodeConstRefPtr, Sdf_PathNodeConstRefPtr>
            RemoveCommonSuffix(const Sdf_PathNodeConstRefPtr& a,
                               const Sdf_PathNodeConstRefPtr& b,
                               bool stopAtRootPrim);

    // This method returns a node pointer
    Sdf_PathNodeConstRefPtr const &GetParentNode() const { return _parent; }

    size_t GetElementCount() const { return size_t(_elementCount); }
    bool IsAbsolutePath() const { return _isAbsolute; }
    bool ContainsTargetPath() const { return _containsTargetPath; }
    bool IsNamespaced() const {
        return (_nodeType == PrimPropertyNode ||
                _nodeType == RelationalAttributeNode) && _IsNamespacedImpl();
    }

    bool ContainsPrimVariantSelection() const {
        return _containsPrimVariantSelection;
    }

    // For PrimNode, PrimPropertyNode, RelationalAttributeNode, and 
    // MapperArgNode this is the name (with no "dot" for 
    // properties/relational attributes/mapper args). For others, it 
    // is EmptyToken.
    inline const TfToken &GetName() const;

    // For TargetNode and MapperNode this is the target path.
    // For others, it is InvalidPath.
    inline const SdfPath &GetTargetPath() const;

    typedef std::pair<TfToken, TfToken> VariantSelectionType;
    inline const VariantSelectionType& GetVariantSelection() const;
    
    // Returns the path element string (".name" for properties, "[path]" for
    // targets, etc...)
    inline TfToken GetElement() const;

    // Return the stringified path to this node as a TfToken.
    SDF_API
    const TfToken &GetPathToken() const;

    // Equality check, accounting for interned & non-interned prim property
    // nodes.
    static inline bool Equals(Sdf_PathNode const *lhs, Sdf_PathNode const *rhs);
    
    // Hash, accounting for interned & non-interned prim property nodes.
    static inline size_t Hash(Sdf_PathNode const *p);
    
    // Lexicographic ordering for Compare().
    struct LessThan {
        template <class T>
        inline bool operator()(T const &a, T const &b) const {
            return a < b;
        }
    };

    // This operator only works properly when the rhs has the same parent
    // as this node.
    template <class Less>
    inline bool Compare(const Sdf_PathNode &rhs) const;

    // Return the current ref-count.
    // Meant for diagnostic use.
    unsigned int GetCurrentRefCount() const { return _refCount; }

protected:
    Sdf_PathNode(Sdf_PathNodeConstRefPtr const &parent, NodeType nodeType,
                 bool isInternedPrimPropNode=true)
        : _parent(parent)
        , _refCount(0)
        , _elementCount(parent->_elementCount + 1)
        , _nodeType(nodeType)
        , _isAbsolute(parent->IsAbsolutePath())
        , _containsPrimVariantSelection(
            nodeType == PrimVariantSelectionNode ||
            parent->_containsPrimVariantSelection)
        , _containsTargetPath(nodeType == TargetNode ||
                              nodeType == MapperNode ||
                              parent->_containsTargetPath)
        , _isInternedPrimPropNode(isInternedPrimPropNode)
        , _hasToken(false)
        {}
    
    // This constructor is used only to create the two special root nodes.
    explicit Sdf_PathNode(bool isAbsolute);

    ~Sdf_PathNode() {
        if (_hasToken)
            _RemovePathTokenFromTable();
    }

    // Helper to downcast and destroy the dynamic type of this object -- this is
    // required since this class hierarchy doesn't use normal C++ polymorphism
    // for space reasons.
    inline void _Destroy() const;

    // Helper function for GetPathToken, which lazily creates its token
    TfToken _CreatePathToken() const;

    // Helper for dtor, removes this path node's token from the token table.
    SDF_API void _RemovePathTokenFromTable() const;

    struct _EqualElement {
        template <class T>
        inline bool operator()(T const &a, T const &b) const {
            return a == b;
        }
    };

    friend struct Sdf_PathNodePrivateAccess;

    // Ref-counting ops manage _refCount.
    friend void intrusive_ptr_add_ref(const Sdf_PathNode*);
    friend void intrusive_ptr_release(const Sdf_PathNode*);

private:
    // Downcast helper, just sugar to static_cast this to Derived const *.
    template <class Derived>
    Derived const *_Downcast() const {
        return static_cast<Derived const *>(this);
    }

    // Helper to scan this node's name for the property namespace delimiter.
    bool _IsNamespacedImpl() const;

    // Helper to return a const lvalue variant selection.
    VariantSelectionType const &_GetEmptyVariantSelection() const;

    // Instance variables.  PathNode's size is important to keep small.  Please
    // be mindful of that when making any changes here.
    const Sdf_PathNodeConstRefPtr _parent;
    mutable tbb::atomic<unsigned int> _refCount;

    const short _elementCount;
    const unsigned char _nodeType;
    const bool _isAbsolute:1;
    const bool _containsPrimVariantSelection:1;
    const bool _containsTargetPath:1;
    const bool _isInternedPrimPropNode:1;

    // This is racy -- we ensure that the token creation code carefully
    // synchronizes so that if we read 'true' from this flag, it guarantees that
    // there's a token for this path node in the token table.  If we read
    // 'false' it means there may or may not be, unless we're in the destructor,
    // which must run exclusively, then reading 'false' guarantees there is no
    // token in the table.  We use this flag to do that optimization in the
    // destructor so we can avoid looking in the table in the case where we
    // haven't created a token.
    mutable bool _hasToken:1;
};

class Sdf_RootPathNode : public Sdf_PathNode {
public:
    typedef bool ComparisonType;
    static const NodeType nodeType = Sdf_PathNode::RootNode;

    static Sdf_PathNodeConstRefPtr New(bool isAbsolute) {
        return Sdf_PathNodeConstRefPtr(new Sdf_RootPathNode(isAbsolute));
    }
    
private:
    // This constructor is used only to create the two special root nodes.
    Sdf_RootPathNode(bool isAbsolute) : Sdf_PathNode(isAbsolute) {}

    ComparisonType _GetComparisonValue() const {
        // Root nodes, there are only two, one absolute and one relative.
        // (absolute < relative...)
        return !IsAbsolutePath();
    }

    friend class Sdf_PathNode;
    template <int nodeType, class Comp> friend struct Sdf_PathNodeCompare;
};

class Sdf_PrimPathNode : public Sdf_PathNode {
public:
    typedef TfToken ComparisonType;
    static const NodeType nodeType = Sdf_PathNode::PrimNode;

    TF_MALLOC_TAG_NEW("Sdf", "new Sdf_PrimPathNode");

private:
    Sdf_PrimPathNode(Sdf_PathNodeConstRefPtr const &parent,
                     const TfToken &name)
        : Sdf_PathNode(parent, nodeType)
        , _name(name) {}

    SDF_API ~Sdf_PrimPathNode();
    
    const ComparisonType &_GetComparisonValue() const { return _name; }

    friend class Sdf_PathNode; 
    friend struct Sdf_PathNodePrivateAccess;
    template <int nodeType, class Comp> friend struct Sdf_PathNodeCompare;

    // Instance variables
    TfToken _name;
};

class Sdf_PrimPropertyPathNode : public Sdf_PathNode {
public:
    typedef TfToken ComparisonType;
    static const NodeType nodeType = Sdf_PathNode::PrimPropertyNode;

    TF_MALLOC_TAG_NEW("Sdf", "new Sdf_PrimPropertyPathNode");

    // Return this prim property node's property name.
    TfToken const &GetName() const { return _name; }
    
    // Create a "floating" non-interned prim property path node.  This is used
    // by SdfPath::AppendProperty() to let us create property paths as quickly
    // as possible.  These path nodes are not allowed to have children -- so
    // target, mapper, expression nodes, will never have one of these as their
    // parent.  Also, equality comparisons and hash functions are carefully
    // written to ensure that interned and non-interned prim property nodes
    // always behave the same.
    static Sdf_PathNodeConstRefPtr
    NewFloatingNode(Sdf_PathNodeConstRefPtr const &parent,
                    const TfToken &name) {
        return Sdf_PathNodeConstRefPtr(
            new Sdf_PrimPropertyPathNode(
                parent, name, /*isInternedPrimPropNode=*/false));
    }

private:
    Sdf_PrimPropertyPathNode(Sdf_PathNodeConstRefPtr const &parent,
                             const TfToken &name,
                             bool isInternedPrimPropNode=true)
        : Sdf_PathNode(parent, nodeType, isInternedPrimPropNode)
        , _name(name) {}

    SDF_API ~Sdf_PrimPropertyPathNode();
    
    friend class Sdf_PathNode;
    friend struct Sdf_PathNodePrivateAccess;
    template <int nodeType, class Comp> friend struct Sdf_PathNodeCompare;

    const ComparisonType &_GetComparisonValue() const { return _name; }

    // Instance variables
    TfToken _name;
};

class Sdf_PrimVariantSelectionNode : public Sdf_PathNode {
public:
    typedef VariantSelectionType ComparisonType;
    static const NodeType nodeType = Sdf_PathNode::PrimVariantSelectionNode;

    TF_MALLOC_TAG_NEW("Sdf", "new Sdf_PrimVariantSelectionNode");

    const TfToken &_GetNameImpl() const;
    TfToken _GetElementImpl() const;

private:
    Sdf_PrimVariantSelectionNode(Sdf_PathNodeConstRefPtr const &parent, 
                                 const VariantSelectionType &variantSelection)
        : Sdf_PathNode(parent, nodeType)
        , _variantSelection(variantSelection) {}

    SDF_API ~Sdf_PrimVariantSelectionNode();

    const ComparisonType &_GetComparisonValue() const {
        return _variantSelection;
    }

    friend class Sdf_PathNode;
    friend struct Sdf_PathNodePrivateAccess;
    template <int nodeType, class Comp> friend struct Sdf_PathNodeCompare;

    // Instance variables
    VariantSelectionType _variantSelection;
};

class Sdf_TargetPathNode : public Sdf_PathNode {
public:
    typedef SdfPath ComparisonType;
    static const NodeType nodeType = Sdf_PathNode::TargetNode;

    TF_MALLOC_TAG_NEW("Sdf", "new Sdf_TargetPathNode");

    TfToken _GetElementImpl() const;

private:
    Sdf_TargetPathNode(Sdf_PathNodeConstRefPtr const &parent,
                       const SdfPath &targetPath)
        : Sdf_PathNode(parent, nodeType)
        , _targetPath(targetPath) {}

    SDF_API ~Sdf_TargetPathNode();

    const ComparisonType& _GetComparisonValue() const { return _targetPath; }

    friend class Sdf_PathNode;
    friend struct Sdf_PathNodePrivateAccess;
    template <int nodeType, class Comp> friend struct Sdf_PathNodeCompare;

    // Instance variables
    SdfPath _targetPath;
};

class Sdf_RelationalAttributePathNode : public Sdf_PathNode {
public:
    typedef TfToken ComparisonType;
    static const NodeType nodeType = Sdf_PathNode::RelationalAttributeNode;

    TF_MALLOC_TAG_NEW("Sdf", "new Sdf_RelationalAttributePathNode");

private:
    Sdf_RelationalAttributePathNode(Sdf_PathNodeConstRefPtr const &parent,
                                    const TfToken &name)
        : Sdf_PathNode(parent, nodeType)
        , _name(name) {}

    SDF_API ~Sdf_RelationalAttributePathNode();

    const ComparisonType& _GetComparisonValue() const { return _name; }

    friend class Sdf_PathNode;
    friend struct Sdf_PathNodePrivateAccess;
    template <int nodeType, class Comp> friend struct Sdf_PathNodeCompare;

    // Instance variables
    TfToken _name;
};

class Sdf_MapperPathNode : public Sdf_PathNode {
public:
    typedef SdfPath ComparisonType;
    static const NodeType nodeType = Sdf_PathNode::MapperNode;

    TF_MALLOC_TAG_NEW("Sdf", "new Sdf_MapperPathNode");

    TfToken _GetElementImpl() const;

private:
    Sdf_MapperPathNode(Sdf_PathNodeConstRefPtr const &parent,
                       const SdfPath &targetPath)
        : Sdf_PathNode(parent, nodeType)
        , _targetPath(targetPath) {}

    SDF_API ~Sdf_MapperPathNode();

    const ComparisonType& _GetComparisonValue() const { return _targetPath; }

    friend class Sdf_PathNode;
    friend struct Sdf_PathNodePrivateAccess;
    template <int nodeType, class Comp> friend struct Sdf_PathNodeCompare;

    // Instance variables
    SdfPath _targetPath;
};

class Sdf_MapperArgPathNode : public Sdf_PathNode {
public:
    typedef TfToken ComparisonType;
    static const NodeType nodeType = Sdf_PathNode::MapperArgNode;

    TF_MALLOC_TAG_NEW("Sdf", "new Sdf_MapperArgPathNode");

    TfToken _GetElementImpl() const;

private:
    Sdf_MapperArgPathNode(Sdf_PathNodeConstRefPtr const &parent,
                          const TfToken &name)
        : Sdf_PathNode(parent, nodeType)
        , _name(name) {}

    SDF_API ~Sdf_MapperArgPathNode();

    const ComparisonType& _GetComparisonValue() const { return _name; }

    friend class Sdf_PathNode;
    friend struct Sdf_PathNodePrivateAccess;
    template <int nodeType, class Comp> friend struct Sdf_PathNodeCompare;

    // Instance variables
    TfToken _name;
};

class Sdf_ExpressionPathNode : public Sdf_PathNode {
public:
    typedef void *ComparisonType;
    static const NodeType nodeType = Sdf_PathNode::ExpressionNode;

    TF_MALLOC_TAG_NEW("Sdf", "new Sdf_ExpressionPathNode");

    TfToken _GetElementImpl() const;

private:
    Sdf_ExpressionPathNode(Sdf_PathNodeConstRefPtr const &parent)
        : Sdf_PathNode(parent, nodeType) {}

    SDF_API ~Sdf_ExpressionPathNode();

    ComparisonType _GetComparisonValue() const { return nullptr; }

    friend class Sdf_PathNode;
    friend struct Sdf_PathNodePrivateAccess;
    template <int nodeType, class Comp> friend struct Sdf_PathNodeCompare;

    // Instance variables
    // <none>
};

template <int nodeType>
struct Sdf_PathNodeTypeToType {
};
template <> struct Sdf_PathNodeTypeToType<Sdf_PathNode::PrimNode> {
    typedef Sdf_PrimPathNode Type;
};
template <> struct Sdf_PathNodeTypeToType<Sdf_PathNode::PrimPropertyNode> {
    typedef Sdf_PrimPropertyPathNode Type;
};
template <> struct Sdf_PathNodeTypeToType<Sdf_PathNode::RelationalAttributeNode> {
    typedef Sdf_RelationalAttributePathNode Type;
};
template <> struct Sdf_PathNodeTypeToType<Sdf_PathNode::MapperArgNode> {
    typedef Sdf_MapperArgPathNode Type;
};
template <> struct Sdf_PathNodeTypeToType<Sdf_PathNode::TargetNode> {
    typedef Sdf_TargetPathNode Type;
};
template <> struct Sdf_PathNodeTypeToType<Sdf_PathNode::MapperNode> {
    typedef Sdf_MapperPathNode Type;
};
template <> struct Sdf_PathNodeTypeToType<Sdf_PathNode::PrimVariantSelectionNode> {
    typedef Sdf_PrimVariantSelectionNode Type;
};
template <> struct Sdf_PathNodeTypeToType<Sdf_PathNode::ExpressionNode> {
    typedef Sdf_ExpressionPathNode Type;
};
template <> struct Sdf_PathNodeTypeToType<Sdf_PathNode::RootNode> {
    typedef Sdf_RootPathNode Type;
};

inline bool
Sdf_PathNode::Equals(Sdf_PathNode const *lhs, Sdf_PathNode const *rhs) {
    if (lhs == rhs) {
        return true;
    }
    if (!lhs || !rhs) {
        return false;
    }
    auto lhsNodeType = lhs->GetNodeType();
    if (lhsNodeType != PrimPropertyNode) {
        return false;
    }
    auto rhsNodeType = rhs->GetNodeType();
    if (rhsNodeType != PrimPropertyNode) {
        return false;
    }
    auto pplhs = lhs->_Downcast<Sdf_PrimPropertyPathNode>();
    auto pprhs = rhs->_Downcast<Sdf_PrimPropertyPathNode>();
    return pplhs->_parent == pprhs->_parent && pplhs->_name == pprhs->_name;
}

inline size_t
Sdf_PathNode::Hash(Sdf_PathNode const *p) {
    size_t result = 0;
    if (p) {
        if (p->GetNodeType() == Sdf_PathNode::PrimPropertyNode) {
            auto pp = p->_Downcast<Sdf_PrimPropertyPathNode>();
            result = ((uintptr_t)(pp->_parent.get()) >> 5) ^ pp->_name.Hash();
        }
        else {
            result = (uintptr_t)(p) >> 5;
        }
    }
    return result;
}

template <int nodeType, class Comp>
struct Sdf_PathNodeCompare {
    inline bool operator()(const Sdf_PathNode &lhs,
                           const Sdf_PathNode &rhs) const {
        typedef typename Sdf_PathNodeTypeToType<nodeType>::Type Type;
        return Comp()(static_cast<const Type&>(lhs)._GetComparisonValue(),
                      static_cast<const Type&>(rhs)._GetComparisonValue());
    }
};

template <class Comp>
inline bool
Sdf_PathNode::Compare(const Sdf_PathNode &rhs) const
{
    // Compare two nodes.
    // We first compare types, then, if types match, we compare
    // based on the type-specific content.
    // Names are compared lexicographically.

    // Compare types.  If node types are different use Comp() on them, otherwise
    // continue to node-specific comparisons.

    NodeType nodeType = GetNodeType(), rhsNodeType = rhs.GetNodeType();
    if (nodeType != rhsNodeType) {
        return Comp()(nodeType, rhsNodeType);
    }

    // Types are the same.  Avoid virtual function calls for performance.
    switch (nodeType) {
    case Sdf_PathNode::PrimNode:
        return Sdf_PathNodeCompare<Sdf_PathNode::PrimNode,
                                  Comp>()(*this, rhs);
    case Sdf_PathNode::PrimPropertyNode:
        return Sdf_PathNodeCompare<Sdf_PathNode::PrimPropertyNode,
                                  Comp>()(*this, rhs);
    case Sdf_PathNode::RelationalAttributeNode:
        return Sdf_PathNodeCompare<Sdf_PathNode::RelationalAttributeNode,
                                  Comp>()(*this, rhs);
    case Sdf_PathNode::MapperArgNode:
        return Sdf_PathNodeCompare<Sdf_PathNode::MapperArgNode,
                                  Comp>()(*this, rhs);
    case Sdf_PathNode::TargetNode:
        return Sdf_PathNodeCompare<Sdf_PathNode::TargetNode,
                                  Comp>()(*this, rhs);
    case Sdf_PathNode::MapperNode:
        return Sdf_PathNodeCompare<Sdf_PathNode::MapperNode,
                                  Comp>()(*this, rhs);
    case Sdf_PathNode::PrimVariantSelectionNode:
        return Sdf_PathNodeCompare<Sdf_PathNode::PrimVariantSelectionNode,
                                  Comp>()(*this, rhs);
    case Sdf_PathNode::ExpressionNode:
        return Sdf_PathNodeCompare<Sdf_PathNode::ExpressionNode,
                                  Comp>()(*this, rhs);
    case Sdf_PathNode::RootNode:
        return Sdf_PathNodeCompare<Sdf_PathNode::RootNode,
                                  Comp>()(*this, rhs);
    default:
        TF_CODING_ERROR("Unhandled Sdf_PathNode::NodeType enumerant");
        return false;
    }
}

inline void
Sdf_PathNode::_Destroy() const
{
    // Note: This function deletes this object!
    switch (_nodeType) {
    case RootNode:
        return delete _Downcast<Sdf_RootPathNode>();
    case PrimNode:
        return delete _Downcast<Sdf_PrimPathNode>();
    case PrimPropertyNode:
        return delete _Downcast<Sdf_PrimPropertyPathNode>();
    case PrimVariantSelectionNode:
        return delete _Downcast<Sdf_PrimVariantSelectionNode>();
    case TargetNode:
        return delete _Downcast<Sdf_TargetPathNode>();
    case RelationalAttributeNode:
        return delete _Downcast<Sdf_RelationalAttributePathNode>();
    case MapperNode:
        return delete _Downcast<Sdf_MapperPathNode>();
    case MapperArgNode:
        return delete _Downcast<Sdf_MapperArgPathNode>();
    case ExpressionNode:
        return delete _Downcast<Sdf_ExpressionPathNode>();
    default:
        return;
    };
}

inline const TfToken &
Sdf_PathNode::GetName() const
{
    switch (_nodeType) {
    default:
        return SdfPathTokens->empty;
    case RootNode:
        return IsAbsolutePath() ?
            SdfPathTokens->absoluteIndicator : SdfPathTokens->relativeRoot;
    case PrimNode:
        return _Downcast<Sdf_PrimPathNode>()->_name;
    case PrimPropertyNode:
        return _Downcast<Sdf_PrimPropertyPathNode>()->_name;
    case PrimVariantSelectionNode:
        return _Downcast<Sdf_PrimVariantSelectionNode>()->_GetNameImpl();
    case RelationalAttributeNode:
        return _Downcast<Sdf_RelationalAttributePathNode>()->_name;
    case MapperArgNode:
        return _Downcast<Sdf_MapperArgPathNode>()->_name;
    case ExpressionNode:
        return SdfPathTokens->expressionIndicator;
    }
}

inline const SdfPath &
Sdf_PathNode::GetTargetPath() const
{
    switch (_nodeType) {
    default:
        return SdfPath::EmptyPath();
    case TargetNode:
        return _Downcast<Sdf_TargetPathNode>()->_targetPath;
    case MapperNode:
        return _Downcast<Sdf_MapperPathNode>()->_targetPath;
    };
}

inline const Sdf_PathNode::VariantSelectionType &
Sdf_PathNode::GetVariantSelection() const
{
    if (ARCH_LIKELY(_nodeType == PrimVariantSelectionNode)) {
        return _Downcast<Sdf_PrimVariantSelectionNode>()->_variantSelection;
    }
    return _GetEmptyVariantSelection();
}

inline TfToken
Sdf_PathNode::GetElement() const
{
    switch (_nodeType) {
    case RootNode:
        return TfToken();
    case PrimNode:
        return _Downcast<Sdf_PrimPathNode>()->_name;
    case PrimPropertyNode:
        return TfToken(SdfPathTokens->propertyDelimiter.GetString() +
                       _Downcast<Sdf_PrimPropertyPathNode>()->
                       _name.GetString());

    case PrimVariantSelectionNode:
        return _Downcast<Sdf_PrimVariantSelectionNode>()->_GetElementImpl();
    case TargetNode:
        return _Downcast<Sdf_TargetPathNode>()->_GetElementImpl();
    case RelationalAttributeNode:
        return TfToken(SdfPathTokens->propertyDelimiter.GetString() +
                       _Downcast<Sdf_RelationalAttributePathNode>()->
                       _name.GetString());
    case MapperNode:
        return _Downcast<Sdf_MapperPathNode>()->_GetElementImpl();
    case MapperArgNode:
        return _Downcast<Sdf_MapperArgPathNode>()->_GetElementImpl();
    case ExpressionNode:
        return _Downcast<Sdf_ExpressionPathNode>()->_GetElementImpl();
    default:
        return TfToken();
    };
}

/// Diagnostic output.
SDF_API void Sdf_DumpPathStats();

inline void intrusive_ptr_add_ref(const PXR_NS::Sdf_PathNode* p) {
    ++p->_refCount;
}
inline void intrusive_ptr_release(const PXR_NS::Sdf_PathNode* p) {
    if (p->_refCount.fetch_and_decrement() == 1)
        p->_Destroy();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_PATHNODE_H
