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
#ifndef PCP_NODE_H
#define PCP_NODE_H

#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/types.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/hashset.h"

#include <boost/operators.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/reverse_iterator.hpp>

struct PcpArc;
class PcpLayerStackSite;
class PcpMapExpression;
class PcpNodeRef;
class PcpNodeRef_ChildrenIterator;
class PcpNodeRef_ChildrenReverseIterator;

TF_DECLARE_WEAK_PTRS(PcpPrimIndex_Graph);

/// PcpNode represents a node in an expression tree for compositing
/// scene description.
///
/// A node represents the opinions from a particular site.  In addition,
/// it may have child nodes, representing nested expressions that are
/// composited over/under this node.
///
/// Child nodes are stored and composited in strength order.
///
/// Each node holds information about the arc to its parent.
/// This captures both the relative strength of the sub-expression
/// as well as any value-mapping needed, such as to rename opinions
/// from a model to use in a particular instance.
///
class PcpNodeRef : 
    public boost::totally_ordered<PcpNodeRef>
{
public:
    typedef PcpNodeRef_ChildrenIterator child_const_iterator;
    typedef PcpNodeRef_ChildrenReverseIterator child_const_reverse_iterator;
    typedef std::pair<child_const_iterator,
                      child_const_iterator> child_const_range;

    PcpNodeRef() : _graph(0), _nodeIdx(PCP_INVALID_INDEX) {}

    /// \name Operators / Miscellaneous
    /// @{

    /// Returns true if this is a valid node reference, false otherwise.
    typedef size_t PcpNodeRef::*UnspecifiedBoolType;
    inline operator UnspecifiedBoolType() const {
        return (_graph and _nodeIdx != PCP_INVALID_INDEX) ? &PcpNodeRef::_nodeIdx : 0;
    }        
    
    /// Returns true if this references the same node as \p rhs.
    inline bool operator==(const PcpNodeRef& rhs) const {
        return _nodeIdx == rhs._nodeIdx and _graph == rhs._graph;
    }

    /// Returns true if this node is 'less' than \p rhs. 
    /// The ordering of nodes is arbitrary and does not indicate the relative
    /// strength of the nodes.
    bool operator<(const PcpNodeRef& rhs) const;

    /// Hash functor.
    struct Hash {
        size_t operator()(const PcpNodeRef& rhs) const
        { return (size_t)rhs.GetUniqueIdentifier(); }
    };

    /// Returns the graph that this node belongs to.
    PcpPrimIndex_GraphPtr GetOwningGraph() const;

    /// Returns a value that uniquely identifies this node.
    void* GetUniqueIdentifier() const;

    /// @}

    /// \name Arc information
    /// Information pertaining to the arcs connecting this node to its
    /// parent and child nodes.
    /// @{

    /// Returns the type of arc connecting this node to its parent node.
    PCP_API PcpArcType GetArcType() const;

    /// Returns this node's immediate parent node. Will return NULL if this
    /// node is a root node.
	PCP_API PcpNodeRef GetParentNode() const;

    /// Returns an iterator range over the children nodes in strongest to
    /// weakest order.
	PCP_API child_const_range GetChildrenRange() const;

    /// Inserts a new child node for \p site, connected to this node via
    /// \p arc.
	PCP_API PcpNodeRef InsertChild(const PcpLayerStackSite& site, const PcpArc& arc);

    /// Inserts \p subgraph as a child of this node, with the root node of
    /// \p subtree connected to this node via \p arc.
    PcpNodeRef InsertChildSubgraph(
        const PcpPrimIndex_GraphPtr& subgraph, const PcpArc& arc);

    /// Returns the immediate origin node for this node. The origin node
    /// is the node that caused this node to be brought into the prim index.
    /// In most cases, this is the same as the parent node. For implied 
    /// inherits, the origin is the node from which this node was propagated.
	PCP_API PcpNodeRef GetOriginNode() const;

    /// Walk up to the root origin node for this node. This is the very
    /// first node that caused this node to be added to the graph. For
    /// instance, the root origin node of an implied inherit is the
    /// original inherit node.
	PCP_API PcpNodeRef GetOriginRootNode() const;

    /// Walk up to the root node of this expression.
	PCP_API PcpNodeRef GetRootNode() const;

    /// Returns mapping function used to translate paths and values from
    /// this node to its parent node.
	PCP_API const PcpMapExpression& GetMapToParent() const;

    /// Returns mapping function used to translate paths and values from
    /// this node directly to the root node.
	PCP_API const PcpMapExpression& GetMapToRoot() const;

    /// Returns this node's index among siblings with the same arc type
    /// at this node's origin.
	PCP_API int GetSiblingNumAtOrigin() const;

    /// Returns the absolute namespace depth of the node that introduced
    /// this node. Note that this does *not* count any variant selections.
	PCP_API int GetNamespaceDepth() const;

    /// Return the number of levels of namespace this node's site is
    /// below the level at which it was introduced by an arc.
	PCP_API int GetDepthBelowIntroduction() const;

    /// Returns the path for this node's site when it was introduced.
	PCP_API SdfPath GetPathAtIntroduction() const;

    /// Get the path that introduced this node.
    /// Specifically, this is the path the parent node had at the level
    /// of namespace where this node was added as a child.
    /// For a root node, this returns the absolute root path.
    /// See also GetDepthBelowIntroduction().
	PCP_API SdfPath GetIntroPath() const;

    /// @} 

    /// \name Node information
    /// Information pertaining specifically to this node and the opinions
    /// that it may or may not provide.
    /// @{

    /// Get the site this node represents.
	PCP_API PcpLayerStackSite GetSite() const;

    /// Returns the path for the site this node represents.
	PCP_API const SdfPath& GetPath() const;

    /// Returns the layer stack for the site this node represents.
	PCP_API const PcpLayerStackPtr& GetLayerStack() const;

    /// Returns true if this node is a source of direct opinions.
    /// There should only be one direct node per prim index.
	PCP_API bool IsDirect() const;

    /// Returns true if this node is due to an ancestral opinion.
	PCP_API bool IsDueToAncestor() const;

    /// Get/set whether this node provides any symmetry opinions, either
    /// directly or from a namespace ancestor.
	PCP_API void SetHasSymmetry(bool hasSymmetry);
	PCP_API bool HasSymmetry() const;

    /// Get/set whether this node provides any variant selection opinions,
    /// either directly or from a namespace ancestor.
	PCP_API void SetHasVariantSelections(bool hasVariantSelections);
	PCP_API bool HasVariantSelections() const;

    /// Get/set the permission for this node. This indicates whether specs
    /// on this node can be accessed from other nodes.
	PCP_API void SetPermission(SdfPermission perm);
	PCP_API SdfPermission GetPermission() const;

    /// Get/set whether this node is inert. An inert node never provides
    /// any opinions to a prim index. Such a node may exist purely as a
    /// marker to represent certain composition structure, but should never 
    /// contribute opinions.
	PCP_API void SetInert(bool inert);
	PCP_API bool IsInert() const;
    
    /// Get/set whether this node is culled. If a node is culled, it and
    /// all descendant nodes provide no opinions to the index. A culled
    /// node is also considered inert.
	PCP_API void SetCulled(bool culled);
	PCP_API bool IsCulled() const;

    /// Get/set whether this node is restricted. A restricted node is a
    /// node that cannot contribute opinions to the index due to permissions.
	PCP_API void SetRestricted(bool restricted);
	PCP_API bool IsRestricted() const;

    /// Get/set whether this node should contribute specs for dependency
    /// tracking even if it is not allowed to contribute opinions.
	PCP_API void SetShouldContributeDependencies(bool shouldContribute);
	PCP_API bool ShouldContributeDependencies() const;

    /// Returns true if this node is allowed to contribute opinions
    /// for composition, false otherwise.
	PCP_API bool CanContributeSpecs() const;
    
    /// Returns true if this node has opinions authored
    /// for composition, false otherwise.
	PCP_API void SetHasSpecs(bool hasSpecs);
	PCP_API bool HasSpecs() const;

    /// @}

    // Returns a compressed Sd site.  For internal use only.
    Pcp_CompressedSdSite GetCompressedSdSite(size_t layerIndex) const
    {
        return Pcp_CompressedSdSite(_nodeIdx, layerIndex);
    }

private:
    friend class PcpPrimIndex_Graph;
    friend class PcpNodeIterator;
    friend class PcpNodeRef_ChildrenIterator;
    friend class PcpNodeRef_ChildrenReverseIterator;
    friend class PcpNodeRef_PrivateChildrenConstIterator;
    friend class PcpNodeRef_PrivateChildrenConstReverseIterator;

    // Private constructor for internal use.
    PcpNodeRef(PcpPrimIndex_Graph* graph, size_t idx)
        : _graph(graph), _nodeIdx(idx)
    {}

    size_t _GetNodeIndex() const { return _nodeIdx; }

    size_t _GetParentIndex() const;
    size_t _GetOriginIndex() const;

private: // Data
    PcpPrimIndex_Graph* _graph;
    size_t _nodeIdx;
};

/// Typedefs and support functions
inline
size_t
hash_value(const PcpNodeRef& x)
{
    return (size_t)x.GetUniqueIdentifier();
}

typedef TfHashSet<PcpNodeRef, PcpNodeRef::Hash> PcpNodeRefHashSet;
typedef std::vector<PcpNodeRef> PcpNodeRefVector;

/// \class PcpNodeRef_ChildrenIterator
/// Object used to iterate over child nodes (not all descendant nodes) of a
/// node in the prim index graph in strong-to-weak order.
class PcpNodeRef_ChildrenIterator
    : public boost::iterator_facade<
                 /* Derived =   */ PcpNodeRef_ChildrenIterator, 
                 /* ValueType = */ PcpNodeRef,
                 /* Category =  */ boost::forward_traversal_tag,
                 /* RefType =   */ PcpNodeRef
             >
{
public:
    /// Constructs an invalid iterator.
    PcpNodeRef_ChildrenIterator();

    /// Constructs an iterator pointing to \p node. Passing a NULL value
    /// for \p node constructs an end iterator.
    PcpNodeRef_ChildrenIterator(const PcpNodeRef& node, bool end = false);

private:
    friend class boost::iterator_core_access;
    void increment();
    bool equal(const PcpNodeRef_ChildrenIterator& other) const
    {
        // Note: The default constructed iterator is *not* equal to any
        //       other iterator.
        return (_node == other._node and _index == other._index);
    }
    reference dereference() const
    {
        return reference(_node._graph, _index);
    }

private:
    // Current graph node this iterator is pointing at.
    PcpNodeRef _node;

    // Index of current child.
    size_t _index;

    friend class PcpNodeRef_ChildrenReverseIterator;
};

/// \class PcpNodeRef_ChildrenReverseIterator
/// Object used to iterate over nodes in the prim index graph in weak-to-strong
/// order.
class PcpNodeRef_ChildrenReverseIterator
    : public boost::iterator_facade<
                 /* Derived =   */ PcpNodeRef_ChildrenReverseIterator, 
                 /* ValueType = */ PcpNodeRef,
                 /* Category =  */ boost::forward_traversal_tag,
                 /* RefType =   */ PcpNodeRef
             >
{
public:
    /// Constructs an invalid iterator.
    PcpNodeRef_ChildrenReverseIterator();

    /// Constructs a reverse iterator from a forward iterator.
    PcpNodeRef_ChildrenReverseIterator(const PcpNodeRef_ChildrenIterator&);

    /// Constructs an iterator pointing to \p node. Passing a NULL value
    /// for \p node constructs an end iterator.
    PcpNodeRef_ChildrenReverseIterator(const PcpNodeRef& node,bool end = false);

private:
    friend class boost::iterator_core_access;
    void increment();
    bool equal(const PcpNodeRef_ChildrenReverseIterator& other) const
    {
        // Note: The default constructed iterator is *not* equal to any
        //       other iterator.
        return (_node == other._node and _index == other._index);
    }
    reference dereference() const
    {
        return reference(_node._graph, _index);
    }

private:
    // Current graph node this iterator is pointing at.
    PcpNodeRef _node;

    // Index of current child.
    size_t _index;
};

template <>
struct Tf_IteratorInterface<PcpNodeRef::child_const_range, false> {
    typedef PcpNodeRef::child_const_iterator IteratorType;
    static IteratorType Begin(PcpNodeRef::child_const_range const &c)
    {
        return c.first;
    }
    static IteratorType End(PcpNodeRef::child_const_range const &c)
    {
        return c.second;
    }
};

template <>
struct Tf_IteratorInterface<PcpNodeRef::child_const_range, true> {
    typedef PcpNodeRef::child_const_reverse_iterator IteratorType;
    static IteratorType Begin(PcpNodeRef::child_const_range const &c)
    {
        return c.second;
    }
    static IteratorType End(PcpNodeRef::child_const_range const &c)
    {
        return c.first;
    }
};

template <>
struct Tf_ShouldIterateOverCopy<PcpNodeRef::child_const_range> :
    boost::true_type {};

// Helper to count the non-variant path components of a path; equivalent
// to path.StripAllVariantSelections().GetPathElementCount() except
// this method avoids constructing a new SdfPath value.
int PcpNode_GetNonVariantPathElementCount(const SdfPath &path);

#endif
