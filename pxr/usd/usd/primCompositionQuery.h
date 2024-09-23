//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_PRIM_COMPOSITION_QUERY_H
#define PXR_USD_USD_PRIM_COMPOSITION_QUERY_H

/// \file usd/primCompositionQuery.h

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/primIndex.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdPrimCompositionQueryArc
///
/// This represents a composition arc that is returned by a 
/// UsdPrimCompositionQuery. It contains the node in the composition graph that
/// is the target of this arc as well as access to information about how the 
/// arc was introduced to the composition graph.
///
/// \section UsdQueryRootArc Root Arc
/// If this arc's \ref GetArcType "arc type" is \ref PcpArcType "PcpArcTypeRoot",
/// then this arc represents the root node of the graph. The composition graph's
/// root arc is not an authored arc; it exists to target the root node of the 
/// graph which represents any local opinions that may be defined for the prim 
/// in the root layer stack.
class UsdPrimCompositionQueryArc
{
public:
    ~UsdPrimCompositionQueryArc() = default;

    /// \name Target and Introducing Nodes
    /// These functions access either the target or the introducing nodes in 
    /// the composition graph that this arc represents. The returned node from
    /// GetTargetNode(), along with a layer obtained from the layer stack 
    /// accessible through the node's GetLayerStack() function, can be used to
    /// create a  \ref UsdEditTarget to direct edits to the target of the arc.
    /// The same can be done with the introducing node but there are additional
    /// functions below that are more convenient for directly editing the 
    /// included arcs.
    ///
    /// It is important to be aware that the nodes returned by GetTargetNode()
    /// and GetIntroducingNode() are only valid through the collective lifetime
    /// of the UsdCompositionQuery and all the UsdPrimCompositionQueryArcs the
    /// query returns. After the query and all the arcs have gone out of scope
    /// every PcpNodeRef returned by these two functions will become immediately
    /// invalid and its behavior will be undefined.
    /// @{
    
    /// Returns the targeted node of this composition arc.
    USD_API
    PcpNodeRef GetTargetNode() const;

    /// Returns the node that introduces this arc into composition graph. This
    /// is the node where the authored composition opinion exists and is not
    /// necessarily the target node's parent. If this arc is the 
    /// \ref UsdQueryRootArc "root arc" then this function returns the same
    /// node as GetTargetNode which is the root node of the composition graph.
    USD_API
    PcpNodeRef GetIntroducingNode() const;

    /// @}

    /// \name Arc Target Details
    /// @{

    /// Returns the root layer of the layer stack that holds the prim spec 
    /// targeted by this composition arc.
    USD_API
    SdfLayerHandle GetTargetLayer() const;

    /// Returns the path of the prim spec that is targeted by this composition
    /// arc in the target layer stack.
    USD_API
    SdfPath GetTargetPrimPath() const;

    /// Creates and returns a resolve target that, when passed to a 
    /// UsdAttributeQuery for one of this prim's attributes, causes value 
    /// resolution to only consider node sites weaker than this arc, up to and
    /// and including this arc's site itself.
    ///
    /// If \p subLayer is provided, it must be a layer in this arc's layer stack
    /// and it will further limit value resolution to only the weaker layers up
    /// to and including \p subLayer within this layer stack. (This is only with 
    /// respect to this arc; all layers will still be considered in the arcs 
    /// weaker than this arc).
    USD_API
    UsdResolveTarget MakeResolveTargetUpTo(
        const SdfLayerHandle &subLayer = nullptr) const;

    /// Creates and returns a resolve target that, when passed to a 
    /// UsdAttributeQuery for one of this prim's attributes, causes value 
    /// resolution to only consider node sites stronger than this arc, not 
    /// including this arc itself (unless \p subLayer is provided).
    ///
    /// If \p subLayer is provided, it must be a layer in this arc's layer stack
    /// and it will cause value resolution to additionally consider layers in 
    /// this arc but only if they are stronger than subLayer within this arc's 
    /// layer stack.
    USD_API
    UsdResolveTarget MakeResolveTargetStrongerThan(
        const SdfLayerHandle &subLayer = nullptr) const;

    /// @}

    /// \name Arc Editing
    /// This set of functions returns information about where the specific 
    /// opinions are authored that cause this arc to be included in the 
    /// composition graph. They can be used to edit the composition arcs 
    /// themselves.
    /// @{

    /// Returns the specific layer in the layer stack that adds this arc to the
    /// composition graph. This layer combined with the path returned from 
    /// GetIntroducingPrimPath can be used to find the prim spec which owns
    /// the field that ultimately causes this arc to exist. If this arc is 
    /// the \ref UsdQueryRootArc "root arc" of the composition graph, it is not
    /// an authored composition arc and this returns a null layer handle.
    USD_API
    SdfLayerHandle GetIntroducingLayer() const;

    /// Returns the path of the prim that introduces this arc to the composition 
    /// graph within the layer in which the composition opinion is authored. 
    /// This path combined with the layer returned from  GetIntroducingLayer
    /// can be used to find the prim spec which owns the field that ultimately 
    /// causes this arc to exist. If this arc is the 
    /// \ref UsdQueryRootArc "root arc" of the composition graph, it is not an 
    /// authored composition arc and this returns an empty path.
    USD_API
    SdfPath GetIntroducingPrimPath() const;

    /// Gets the list editor and authored SdfReference value that introduces
    /// this arc to the composition graph for reference arcs. If this arc's
    /// type is reference, \p editor will be set to the reference list editor
    /// of the introducing prim spec and \p ref will be set to the authored 
    /// value of the SdfReference in the reference list.
    /// 
    /// This returns true if the \ref GetArcType "arc type" is 
    /// \ref PcpArcType "reference" and there are no errors; it returns false 
    /// for all other arc types.
    USD_API
    bool GetIntroducingListEditor(SdfReferenceEditorProxy *editor, 
                                  SdfReference *ref) const;

    /// Gets the list editor and authored SdfPayload value that introduces
    /// this arc to the composition graph for payload arcs. If this arc's
    /// type is payload, \p editor will be set to the payload list editor
    /// of the introducing prim spec and \p payload will be set to the authored 
    /// value of the SdfPayload in the payload list.
    /// 
    /// This returns true if the \ref GetArcType "arc type" is 
    /// \ref PcpArcType "payload" and there are no errors; it returns false 
    /// for all other arc types.
    USD_API
    bool GetIntroducingListEditor(SdfPayloadEditorProxy *editor, 
                                  SdfPayload *payload) const;

    /// Gets the list editor and authored SdfPath value that introduced
    /// this arc to the composition graph for class arcs. If this arc's
    /// type is inherit or specialize, \p editor will be set to the 
    /// corresponding path list editor of the introducing prim spec and \p path
    /// will be set to the authored value of the SdfPath in the path list.
    /// 
    /// This returns true if the \ref GetArcType "arc type" is either 
    /// \ref PcpArcType "inherit" or \ref PcpArcType "specialize" and 
    /// there are no errors; it returns false for all other arc types.
    USD_API
    bool GetIntroducingListEditor(SdfPathEditorProxy *editor, 
                                  SdfPath *path) const;

    /// Gets the list editor and authored string value that introduces
    /// this arc to the composition graph for variant arcs. If this arc's
    /// type is variant, \p editor will be set to the name list editor
    /// of the introducing prim spec and \p name will be set to the authored 
    /// value of the variant set name in the name list.
    /// 
    /// This returns true if the \ref GetArcType "arc type" is 
    /// \ref PcpArcType "variant" and there are no errors; it returns false 
    /// for all other arc types.
    USD_API
    bool GetIntroducingListEditor(SdfNameEditorProxy *editor, 
                                  std::string *name) const;

    /// @}

    /// \name Arc classification
    /// Queries about the arc that are useful for classifying the arc for 
    /// filtering.
    /// @{

    /// Returns the arc type.
    USD_API
    PcpArcType GetArcType() const;

    /// Returns whether this arc was implicitly added to this prim meaning it
    /// exists because of the introduction of another composition arc. These 
    /// will typically exist due to inherits or specializes that are authored 
    /// across a reference.
    USD_API
    bool IsImplicit() const;

    /// Returns whether this arc is ancestral, i.e. it exists because it was 
    /// composed in by a namespace parent's prim index. 
    USD_API
    bool IsAncestral() const; 
    
    /// Returns whether the target node of this arc contributes any local spec 
    /// opinions that are composed for the prim.
    USD_API
    bool HasSpecs() const; 
        
    /// Returns whether the composition opinion that introduces this arc 
    /// is authored in the root layer stack. This returns true for any arcs 
    /// where the composition opinion can be authored in the root layer stack.
    /// This is always true for the root arc.
    USD_API
    bool IsIntroducedInRootLayerStack() const;

    /// Returns whether the composition opinion that introduces this arc is 
    /// authored directly on the prim's prim spec within the root layer stack.
    /// This is always true for the root arc.
    USD_API
    bool IsIntroducedInRootLayerPrimSpec() const;

    /// @}

private:
    // These will only be created by a UsdPrimCompositionQuery itself.
    friend class UsdPrimCompositionQuery;
    UsdPrimCompositionQueryArc(const PcpNodeRef &node);

    PcpNodeRef _node;
    PcpNodeRef _originalIntroducedNode;
    PcpNodeRef _introducingNode;

    std::shared_ptr<PcpPrimIndex> _primIndex;
};

/// \class UsdPrimCompositionQuery
///
/// Object for making optionally filtered composition queries about a prim.
/// It creates a list of strength ordering UsdPrimCompositionQueryArc that
/// can be filtered by a combination of criteria and returned.
///
/// \section UsdPrimCompositionQuery_Invalidation Invalidation
/// This object does not listen for change notification.  If a consumer is
/// holding on to a UsdPrimCompositionQuery, it is their responsibility to
/// dispose of it in response to a resync change to the associated prim.
/// Failing to do so may result in incorrect values or crashes due to
/// dereferencing invalid objects.
class UsdPrimCompositionQuery
{
public:
    /// Choices for filtering composition arcs based on arc type
    enum class ArcTypeFilter
    {
        All = 0,

        // Single arc types
        Reference,
        Payload,
        Inherit,
        Specialize,
        Variant,

        // Related arc types
        ReferenceOrPayload,
        InheritOrSpecialize,

        // Inverse of related arc types
        NotReferenceOrPayload,
        NotInheritOrSpecialize,
        NotVariant
    };

    /// Choices for filtering composition arcs on dependency type. This can
    /// be direct (arc introduced at the prim's level in namespace) or ancestral
    /// (arc introduced by a namespace parent of the prim).
    enum class DependencyTypeFilter
    {
        All = 0,

        Direct,
        Ancestral
    };

    /// Choices for filtering composition arcs based on where the arc is 
    /// introduced. 
    enum class ArcIntroducedFilter
    {
        All = 0,

        // Indicates that we only want arcs that are authored somewhere in the
        // root layer stack.
        IntroducedInRootLayerStack,

        // Indicates that we only want arcs that are authored directly in the
        // in the prim's prim spec in the root layer stack.
        IntroducedInRootLayerPrimSpec
    };

    /// Choices for filtering composition arcs on whether the node contributes
    /// specs to the prim.
    enum class HasSpecsFilter
    {
        All = 0,

        HasSpecs,
        HasNoSpecs
    };

    /// Aggregate filter for filtering composition arcs by the previously
    /// defined criteria.
    struct Filter 
    {
        /// Filters by arc type
        ArcTypeFilter arcTypeFilter {ArcTypeFilter::All};

        /// Filters by dependency type, direct or ancestral.
        DependencyTypeFilter dependencyTypeFilter {DependencyTypeFilter::All};

        /// Filters by where the arc is introduced
        ArcIntroducedFilter arcIntroducedFilter {ArcIntroducedFilter::All};

        /// Filters by whether the arc provides specs for the prim.
        HasSpecsFilter hasSpecsFilter {HasSpecsFilter::All};

        Filter() {};

        bool operator==(const Filter &rhs) {
            return arcIntroducedFilter == rhs.arcIntroducedFilter &&
                arcTypeFilter == rhs.arcTypeFilter &&
                dependencyTypeFilter == rhs.dependencyTypeFilter &&
                hasSpecsFilter == rhs.hasSpecsFilter;
        };

        bool operator!=(const Filter &rhs) {
            return !(*this == rhs);
        };
    };

    /// Returns a prim composition query for the given \p prim with a preset 
    /// filter that only returns reference arcs that are not ancestral.
    USD_API
    static UsdPrimCompositionQuery GetDirectReferences(const UsdPrim & prim);

    /// Returns a prim composition query for the given \p prim with a preset 
    /// filter that only returns inherit arcs that are not ancestral.
    USD_API
    static UsdPrimCompositionQuery GetDirectInherits(const UsdPrim & prim);

    /// Returns a prim composition query for the given \p prim with a preset 
    /// filter that only returns direct arcs that were introduced by opinions 
    /// defined in a layer in the root layer stack.
    USD_API
    static UsdPrimCompositionQuery GetDirectRootLayerArcs(const UsdPrim & prim);

    /// Create a prim composition query for the \p with the given option 
    /// \p filter.
    USD_API
    UsdPrimCompositionQuery(const UsdPrim & prim, 
                            const Filter &filter = Filter());

    ~UsdPrimCompositionQuery() = default;

    /// Change the filter for this query.
    USD_API
    void SetFilter(const Filter &filter);

    /// Return a copy of the current filter parameters.
    USD_API
    Filter GetFilter() const;

    /// Return a list of composition arcs for this query's prim using the 
    /// current query filter. The composition arcs are always returned in order
    /// from strongest to weakest regardless of the filter.
    USD_API
    std::vector<UsdPrimCompositionQueryArc> GetCompositionArcs();

private:
    UsdPrim _prim;
    Filter _filter;
    std::shared_ptr<PcpPrimIndex> _expandedPrimIndex;
    std::vector<UsdPrimCompositionQueryArc> _unfilteredArcs;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_PRIM_COMPOSITION_QUERY_H

