//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_PATH_NODE_H
#define PXR_USD_SDF_PATH_NODE_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/base/tf/delegatedCountPtr.h"
#include "pxr/base/tf/functionRef.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/mallocTag.h"

PXR_NAMESPACE_OPEN_SCOPE

// Sdf_PathNode
//
// This class is the root of the path node hierarchy.  It used to use ordinary
// C++ polymorphism, but it no longer does.  This is primarily a space
// optimization: the set of node types is fixed, we already have an enum 'type'
// field, and we typically have lots (e.g. ~1e8) of these objects.  Dropping the
// C++ polymorphism saves us the vtable pointer, and we can pack the '_nodeType'
// field in a much smaller space with other flags.
//
// We currently store PathNode objects in two prefix trees.  The "prim like"
// path nodes (the root nodes '/' and '.', prim path nodes, and prim variant
// selection nodes are in one prefix tree, and the "property like" nodes are in
// another prefix tree (prim property nodes, target nodes, expression nodes,
// mapper arg nodes).  We do this because there are far fewer unique property
// nodes (generally) than there are prim nodes.  We allocate these in Sdf_Pools,
// so that the SdfPath class can store a handle to an element in each tree in 64
// bits total.  (Sdf_Pool is designed so that we can refer to objects in memory
// using 32-bit indexes instead of 64-bit pointers).  An SdfPath joins together
// these two elements to form a whole path.  For example, the path
// '/Foo/Bar.attr' would store a prim-part handle to the '/Foo/Bar' node, and a
// property-part handle to 'attr'.
//
class Sdf_PathNode {
    Sdf_PathNode(Sdf_PathNode const &) = delete;
    Sdf_PathNode &operator=(Sdf_PathNode const &) = delete;
public:

    static constexpr uint8_t IsAbsoluteFlag = 1 << 0;
    static constexpr uint8_t ContainsPrimVarSelFlag = 1 << 1;
    static constexpr uint8_t ContainsTargetPathFlag = 1 << 2;

    static constexpr uint32_t HasTokenBit = 1u << 31;
    static constexpr uint32_t RefCountMask = ~HasTokenBit;
    
    // Node types identify what kind of path node a given instance is.
    // There are restrictions on what type of children each node type 
    // can have,
    enum NodeType : uint8_t {
        
        /********************************************************/
        /******************************* Prim portion nodes *****/
        
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

        PrimVariantSelectionNode,
            // Allowable child node types:
            //     PrimNode
            //     PrimPropertyNode
            //     PrimVariantSelectionNode 
            //         (for variants that contain variant sets)

        /********************************************************/
        /******************************* Property portion nodes */
        
        PrimPropertyNode,
            // Allowable child node types:
            //     TargetNode
            //     MapperNode
            //     ExpressionNode

        TargetNode,
            // Allowable child node types:
            //     RelationalAttributeNode (only if parent is PrimPropertyNode)

        MapperNode,
            // Allowable child node types:
            //     MapperArgNode

        RelationalAttributeNode,
            // Allowable child node types:
            //     TargetNode
            //     MapperNode
            //     ExpressionNode

        MapperArgNode,
            // Allowable child node types:
            //     <none>

        ExpressionNode,
            // Allowable child node types:
            //     <none>

        NumNodeTypes ///< Internal sentinel value
    };

    static Sdf_PathPrimNodeHandle
    FindOrCreatePrim(Sdf_PathNode const *parent, const TfToken &name,
                     TfFunctionRef<bool ()> isValid);
    
    static Sdf_PathPropNodeHandle
    FindOrCreatePrimProperty(
        Sdf_PathNode const *parent, const TfToken &name,
        TfFunctionRef<bool ()> isValid);
    
    static Sdf_PathPrimNodeHandle
    FindOrCreatePrimVariantSelection(Sdf_PathNode const *parent,
                                     const TfToken &variantSet,
                                     const TfToken &variant,
                                     TfFunctionRef<bool ()> isValid);

    static Sdf_PathPropNodeHandle
    FindOrCreateTarget(Sdf_PathNode const *parent,
                       SdfPath const &targetPath,
                       TfFunctionRef<bool ()> isValid);

    static Sdf_PathPropNodeHandle
    FindOrCreateRelationalAttribute(Sdf_PathNode const *parent,
                                    const TfToken &name,
                                    TfFunctionRef<bool ()> isValid);

    static Sdf_PathPropNodeHandle
    FindOrCreateMapper(Sdf_PathNode const *parent, SdfPath const &targetPath,
                       TfFunctionRef<bool ()> isValid);

    static Sdf_PathPropNodeHandle
    FindOrCreateMapperArg(Sdf_PathNode const *parent, const TfToken &name,
                          TfFunctionRef<bool ()> isValid);
    
    static Sdf_PathPropNodeHandle
    FindOrCreateExpression(Sdf_PathNode const *parent,
                           TfFunctionRef<bool ()> isValid);

    static Sdf_PathNode const *GetAbsoluteRootNode();
    static Sdf_PathNode const *GetRelativeRootNode();

    NodeType GetNodeType() const { return NodeType(_nodeType); }

    static std::pair<Sdf_PathNode const *, Sdf_PathNode const *>
    RemoveCommonSuffix(Sdf_PathNode const *a,
                       Sdf_PathNode const *b,
                       bool stopAtRootPrim);

    // This method returns a node pointer
    inline Sdf_PathNode const *GetParentNode() const { return _parent.get(); }

    size_t GetElementCount() const { return _elementCount; }
    bool IsAbsolutePath() const { return _nodeFlags & IsAbsoluteFlag; }
    bool IsAbsoluteRoot() const { return IsAbsolutePath() & (!_elementCount); }
    bool ContainsTargetPath() const {
        return _nodeFlags & ContainsTargetPathFlag;
    }
    bool IsNamespaced() const {
        // Bitwise-or to avoid branching in the node type comparisons, but
        // logical and to avoid calling _IsNamespacedImpl() unless necessary.
        return ((_nodeType == PrimPropertyNode) |
                (_nodeType == RelationalAttributeNode)) && _IsNamespacedImpl();
    }

    bool ContainsPrimVariantSelection() const {
        return _nodeFlags & ContainsPrimVarSelFlag;
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

    // Return the stringified path to this node as a TfToken lvalue.
    SDF_API static const TfToken &
    GetPathToken(Sdf_PathNode const *primPart, Sdf_PathNode const *propPart);

    // Return the stringified path to this node as a TfToken rvalue.
    SDF_API static TfToken
    GetPathAsToken(Sdf_PathNode const *primPart, Sdf_PathNode const *propPart);

    static char const *
    GetDebugText(Sdf_PathNode const *primPart, Sdf_PathNode const *propPart);

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
    uint32_t GetCurrentRefCount() const {
        return _refCount.load(std::memory_order_relaxed) & RefCountMask;
    }

protected:
    Sdf_PathNode(Sdf_PathNode const *parent, NodeType nodeType)
        : _parent(TfDelegatedCountIncrementTag, parent)
        , _refCount(1)
        , _elementCount(parent ? parent->_elementCount + 1 : 1)
        , _nodeType(nodeType)
        , _nodeFlags(
            (parent ? parent->_nodeFlags : 0) | _NodeTypeToFlags(nodeType))
        {
        }
    
    // This constructor is used only to create the two special root nodes.
    explicit Sdf_PathNode(bool isAbsolute);

    ~Sdf_PathNode() {
        if (_refCount.load(std::memory_order_relaxed) & HasTokenBit) {
            _RemovePathTokenFromTable();
        }
    }

    // Helper to downcast and destroy the dynamic type of this object -- this is
    // required since this class hierarchy doesn't use normal C++ polymorphism
    // for space reasons.
    inline void _Destroy() const;
 
    TfToken _GetElementImpl() const;

    // Helper function for GetPathToken, which lazily creates its token
    static TfToken _CreatePathToken(Sdf_PathNode const *primPart,
                                    Sdf_PathNode const *propPart);

    template <class Buffer>
    static void _WriteTextToBuffer(Sdf_PathNode const *primPart,
                                   Sdf_PathNode const *propPart,
                                   Buffer &out);

    template <class Buffer>
    static void _WriteTextToBuffer(SdfPath const &path, Buffer &out);

    // Append this element's text (same as GetElement()) to \p out.
    template <class Buffer>
    void _WriteText(Buffer &out) const;

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
    friend void TfDelegatedCountIncrement(const Sdf_PathNode*) noexcept;
    friend void TfDelegatedCountDecrement(const Sdf_PathNode*) noexcept;

private:
    static constexpr uint8_t _NodeTypeToFlags(NodeType nt) {
        if (nt == PrimVariantSelectionNode) {
            return ContainsPrimVarSelFlag;
        }
        if (nt == TargetNode || nt == MapperNode) {
            return ContainsTargetPathFlag;
        }
        return 0;
    }

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

    // The high-order bit of _refCount (HasTokenBit) indicates whether or not
    // we've created a token for this path node.
    mutable std::atomic<uint32_t> _refCount;

    const uint16_t _elementCount;
    const NodeType _nodeType;
    const uint8_t _nodeFlags;
    
};

class Sdf_PrimPartPathNode : public Sdf_PathNode {
public:
    using Sdf_PathNode::Sdf_PathNode;
    SDF_API void operator delete (void *p);
};

class Sdf_PropPartPathNode : public Sdf_PathNode {
public:
    using Sdf_PathNode::Sdf_PathNode;
    SDF_API void operator delete (void *p);
};

class Sdf_RootPathNode : public Sdf_PrimPartPathNode {
public:
    typedef bool ComparisonType;
    static const NodeType nodeType = Sdf_PathNode::RootNode;

    static SDF_API Sdf_PathNode const *New(bool isAbsolute);
    
private:
    // This constructor is used only to create the two special root nodes.
    Sdf_RootPathNode(bool isAbsolute) : Sdf_PrimPartPathNode(isAbsolute) {}

    ComparisonType _GetComparisonValue() const {
        // Root nodes, there are only two, one absolute and one relative.
        // (absolute < relative...)
        return !IsAbsolutePath();
    }

    friend class Sdf_PathNode;
    template <int nodeType, class Comp> friend struct Sdf_PathNodeCompare;
};

class Sdf_PrimPathNode : public Sdf_PrimPartPathNode {
public:
    typedef TfToken ComparisonType;
    static const NodeType nodeType = Sdf_PathNode::PrimNode;

private:
    Sdf_PrimPathNode(Sdf_PathNode const *parent,
                     const TfToken &name)
        : Sdf_PrimPartPathNode(parent, nodeType)
        , _name(name) {}

    SDF_API ~Sdf_PrimPathNode();
    
    const ComparisonType &_GetComparisonValue() const { return _name; }

    friend class Sdf_PathNode; 
    friend struct Sdf_PathNodePrivateAccess;
    template <int nodeType, class Comp> friend struct Sdf_PathNodeCompare;

    // Instance variables
    TfToken _name;
};

class Sdf_PrimPropertyPathNode : public Sdf_PropPartPathNode {
public:
    typedef TfToken ComparisonType;
    static const NodeType nodeType = Sdf_PathNode::PrimPropertyNode;

private:
    Sdf_PrimPropertyPathNode(Sdf_PathNode const *parent,
                             const TfToken &name)
        : Sdf_PropPartPathNode(parent, nodeType)
        , _name(name) {}

    SDF_API ~Sdf_PrimPropertyPathNode();
    
    friend class Sdf_PathNode;
    friend struct Sdf_PathNodePrivateAccess;
    template <int nodeType, class Comp> friend struct Sdf_PathNodeCompare;

    const ComparisonType &_GetComparisonValue() const { return _name; }

    // Instance variables
    TfToken _name;
};

class Sdf_PrimVariantSelectionNode : public Sdf_PrimPartPathNode {
public:
    typedef VariantSelectionType ComparisonType;
    static const NodeType nodeType = Sdf_PathNode::PrimVariantSelectionNode;

    const TfToken &_GetNameImpl() const;

    template <class Buffer>
    void _WriteTextImpl(Buffer &out) const;

private:
    Sdf_PrimVariantSelectionNode(Sdf_PathNode const *parent, 
                                 const VariantSelectionType &variantSelection)
        : Sdf_PrimPartPathNode(parent, nodeType)
        , _variantSelection(new VariantSelectionType(variantSelection)) {}

    SDF_API ~Sdf_PrimVariantSelectionNode();

    const ComparisonType &_GetComparisonValue() const {
        return *_variantSelection;
    }

    friend class Sdf_PathNode;
    friend struct Sdf_PathNodePrivateAccess;
    template <int nodeType, class Comp> friend struct Sdf_PathNodeCompare;

    // Instance variables
    std::unique_ptr<VariantSelectionType> _variantSelection;
};

class Sdf_TargetPathNode : public Sdf_PropPartPathNode {
public:
    typedef SdfPath ComparisonType;
    static const NodeType nodeType = Sdf_PathNode::TargetNode;

    template <class Buffer>
    void _WriteTextImpl(Buffer &out) const;

private:
    Sdf_TargetPathNode(Sdf_PathNode const *parent,
                       const SdfPath &targetPath)
        : Sdf_PropPartPathNode(parent, nodeType)
        , _targetPath(targetPath) {}

    SDF_API ~Sdf_TargetPathNode();

    const ComparisonType& _GetComparisonValue() const { return _targetPath; }

    friend class Sdf_PathNode;
    friend struct Sdf_PathNodePrivateAccess;
    template <int nodeType, class Comp> friend struct Sdf_PathNodeCompare;

    // Instance variables
    SdfPath _targetPath;
};

class Sdf_RelationalAttributePathNode : public Sdf_PropPartPathNode {
public:
    typedef TfToken ComparisonType;
    static const NodeType nodeType = Sdf_PathNode::RelationalAttributeNode;

private:
    Sdf_RelationalAttributePathNode(Sdf_PathNode const *parent,
                                    const TfToken &name)
        : Sdf_PropPartPathNode(parent, nodeType)
        , _name(name) {}

    SDF_API ~Sdf_RelationalAttributePathNode();

    const ComparisonType& _GetComparisonValue() const { return _name; }

    friend class Sdf_PathNode;
    friend struct Sdf_PathNodePrivateAccess;
    template <int nodeType, class Comp> friend struct Sdf_PathNodeCompare;

    // Instance variables
    TfToken _name;
};

class Sdf_MapperPathNode : public Sdf_PropPartPathNode {
public:
    typedef SdfPath ComparisonType;
    static const NodeType nodeType = Sdf_PathNode::MapperNode;

    template <class Buffer>
    void _WriteTextImpl(Buffer &out) const;

private:
    Sdf_MapperPathNode(Sdf_PathNode const *parent,
                       const SdfPath &targetPath)
        : Sdf_PropPartPathNode(parent, nodeType)
        , _targetPath(targetPath) {}

    SDF_API ~Sdf_MapperPathNode();

    const ComparisonType& _GetComparisonValue() const { return _targetPath; }

    friend class Sdf_PathNode;
    friend struct Sdf_PathNodePrivateAccess;
    template <int nodeType, class Comp> friend struct Sdf_PathNodeCompare;

    // Instance variables
    SdfPath _targetPath;
};

class Sdf_MapperArgPathNode : public Sdf_PropPartPathNode {
public:
    typedef TfToken ComparisonType;
    static const NodeType nodeType = Sdf_PathNode::MapperArgNode;

    template <class Buffer>
    void _WriteTextImpl(Buffer &out) const;

private:
    Sdf_MapperArgPathNode(Sdf_PathNode const *parent,
                          const TfToken &name)
        : Sdf_PropPartPathNode(parent, nodeType)
        , _name(name) {}

    SDF_API ~Sdf_MapperArgPathNode();

    const ComparisonType& _GetComparisonValue() const { return _name; }

    friend class Sdf_PathNode;
    friend struct Sdf_PathNodePrivateAccess;
    template <int nodeType, class Comp> friend struct Sdf_PathNodeCompare;

    // Instance variables
    TfToken _name;
};

class Sdf_ExpressionPathNode : public Sdf_PropPartPathNode {
public:
    typedef void *ComparisonType;
    static const NodeType nodeType = Sdf_PathNode::ExpressionNode;

    template <class Buffer>
    void _WriteTextImpl(Buffer &out) const;

private:
    Sdf_ExpressionPathNode(Sdf_PathNode const *parent)
        : Sdf_PropPartPathNode(parent, nodeType) {}

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
        return *_Downcast<Sdf_PrimVariantSelectionNode>()->_variantSelection;
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
    default:
        return _GetElementImpl();
    };
}

/// Diagnostic output.
SDF_API void Sdf_DumpPathStats();

inline void TfDelegatedCountIncrement(const PXR_NS::Sdf_PathNode* p) noexcept {
    p->_refCount.fetch_add(1, std::memory_order_relaxed);
}
inline void TfDelegatedCountDecrement(const PXR_NS::Sdf_PathNode* p) noexcept {
    if ((p->_refCount.fetch_sub(1) & PXR_NS::Sdf_PathNode::RefCountMask) == 1) {
        p->_Destroy();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PATH_NODE_H
