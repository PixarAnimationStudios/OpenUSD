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
#ifndef PXR_USD_SDF_PATH_NODE_H
#define PXR_USD_SDF_PATH_NODE_H

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
    // Node types identify what kind of path node a given instance is.
    // There are restrictions on what type of children each node type 
    // can have,
    enum NodeType {
        
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
    FindOrCreatePrim(Sdf_PathNode const *parent, const TfToken &name);
    
    static Sdf_PathPropNodeHandle
    FindOrCreatePrimProperty(Sdf_PathNode const *parent, const TfToken &name);
    
    static Sdf_PathPrimNodeHandle
    FindOrCreatePrimVariantSelection(Sdf_PathNode const *parent,
                                     const TfToken &variantSet,
                                     const TfToken &variant);

    static Sdf_PathPropNodeHandle
    FindOrCreateTarget(Sdf_PathNode const *parent,
                       SdfPath const &targetPath);

    static Sdf_PathPropNodeHandle
    FindOrCreateRelationalAttribute(Sdf_PathNode const *parent,
                                    const TfToken &name);

    static Sdf_PathPropNodeHandle
    FindOrCreateMapper(Sdf_PathNode const *parent, SdfPath const &targetPath);

    static Sdf_PathPropNodeHandle
    FindOrCreateMapperArg(Sdf_PathNode const *parent, const TfToken &name);
    
    static Sdf_PathPropNodeHandle
    FindOrCreateExpression(Sdf_PathNode const *parent);

    static Sdf_PathNode const *GetAbsoluteRootNode();
    static Sdf_PathNode const *GetRelativeRootNode();

    NodeType GetNodeType() const { return NodeType(_nodeType); }

    static std::pair<Sdf_PathNode const *, Sdf_PathNode const *>
    RemoveCommonSuffix(Sdf_PathNode const *a,
                       Sdf_PathNode const *b,
                       bool stopAtRootPrim);

    // This method returns a node pointer
    Sdf_PathNode const *GetParentNode() const { return _parent.get(); }

    size_t GetElementCount() const { return size_t(_elementCount); }
    bool IsAbsolutePath() const { return _isAbsolute; }
    bool IsAbsoluteRoot() const { return (_isAbsolute) & (!_elementCount); }
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

    // Append this element's text (same as GetElement()) to \p str.
    void AppendText(std::string *str) const;

    // Return the stringified path to this node as a TfToken lvalue.
    SDF_API static const TfToken &
    GetPathToken(Sdf_PathNode const *primPart, Sdf_PathNode const *propPart);

    // Return the stringified path to this node as a TfToken rvalue.
    SDF_API static TfToken
    GetPathAsToken(Sdf_PathNode const *primPart, Sdf_PathNode const *propPart);

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
    Sdf_PathNode(Sdf_PathNode const *parent, NodeType nodeType)
        : _parent(parent)
        , _refCount(1)
        , _elementCount(parent ? parent->_elementCount + 1 : 1)
        , _nodeType(nodeType)
        , _isAbsolute(parent && parent->IsAbsolutePath())
        , _containsPrimVariantSelection(
            nodeType == PrimVariantSelectionNode ||
            (parent && parent->_containsPrimVariantSelection))
        , _containsTargetPath(nodeType == TargetNode ||
                              nodeType == MapperNode ||
                              (parent && parent->_containsTargetPath))
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

    // // Helper function for GetPathToken, which lazily creates its token
    static TfToken _CreatePathToken(Sdf_PathNode const *primPart,
                                    Sdf_PathNode const *propPart);

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
    void _AppendText(std::string *str) const;

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

    void _AppendText(std::string *str) const;

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

    void _AppendText(std::string *str) const;

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

    void _AppendText(std::string *str) const;

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

    void _AppendText(std::string *str) const;

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
        std::string str;
        AppendText(&str);
        return TfToken(str);
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

#endif // PXR_USD_SDF_PATH_NODE_H
