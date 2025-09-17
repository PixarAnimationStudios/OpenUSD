//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_RESOLVE_TARGET_H
#define PXR_USD_USD_RESOLVE_TARGET_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/sdf/declareHandles.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);

/// \class UsdResolveTarget
///
/// Defines a subrange of nodes and layers within a prim's prim index to 
/// consider when performing value resolution for the prim's attributes.
/// A resolve target can then be passed to UsdAttributeQuery during its 
/// construction to have all of the queries made by the UsdAttributeQuery use
/// the resolve target's subrange for their value resolution.
///
/// Resolve targets can be created via methods on UsdPrimCompositionQueryArc to
/// to limit value resolution to a subrange of the prim's composed specs that 
/// are \ref UsdPrimCompositionQueryArc::MakeResolveTargetUpTo "no stronger that arc", 
/// or a subrange of specs that is 
/// \ref UsdPrimCompositionQueryArc::MakeResolveTargetStrongerThan "strictly stronger than that arc" 
/// (optionally providing a particular layer within the arc's layer stack to 
/// further limit the range of specs). 
///
/// Alternatively, resolve targets can also be created via methods on UsdPrim
/// that can limit value resolution to either 
/// \ref UsdPrim::MakeResolveTargetUpToEditTarget "up to" or 
/// \ref UsdPrim::MakeResolveTargetStrongerThanEditTarget "stronger than" 
/// the spec that would be edited when setting a value for the prim using the
/// given UsdEditTarget.
///
/// Unlike UsdEditTarget, a UsdResolveTarget is only relevant to the prim it
/// is created for and can only be used in a UsdAttributeQuery for attributes 
/// on this prim.
///
/// \section UsdResolveTarget_Invalidation Invalidation
/// This object does not listen for change notification.  If a consumer is
/// holding on to a UsdResolveTarget, it is their responsibility to dispose
/// of it in response to a resync change to the associated prim. 
/// Failing to do so may result in incorrect values or crashes due to 
/// dereferencing invalid objects.
///
class UsdResolveTarget {

public:
    UsdResolveTarget() = default;

    /// Get the prim index of the resolve target.
    const PcpPrimIndex *GetPrimIndex() const {
        return _expandedPrimIndex.get();
    }

    /// Returns the node that value resolution with this resolve target will
    /// start at.
    USD_API
    PcpNodeRef GetStartNode() const;

    /// Returns the layer in the layer stack of the start node that value 
    /// resolution with this resolve target will start at.
    USD_API
    SdfLayerHandle GetStartLayer() const;

    /// Returns the node that value resolution with this resolve target will 
    /// stop at when the "stop at" layer is reached.
    USD_API
    PcpNodeRef GetStopNode() const;

    /// Returns the layer in the layer stack of the stop node that value 
    /// resolution with this resolve target will stop at.
    USD_API
    SdfLayerHandle GetStopLayer() const;

    /// Returns true if this is a null resolve target.
    bool IsNull() const {
        return !bool(_expandedPrimIndex);
    }

private:
    // Non-null UsdResolveTargets can only be created by functions in UsdPrim 
    // and UsdPrimCompositionQueryArc.
    friend class UsdPrim;
    friend class UsdPrimCompositionQueryArc;

    // Usd_Resolver wants to access the iterators provided by this target.
    friend class Usd_Resolver;

    USD_API
    UsdResolveTarget(
        const std::shared_ptr<PcpPrimIndex> &index, 
        const PcpNodeRef &node, 
        const SdfLayerHandle &layer);

    USD_API
    UsdResolveTarget(
        const std::shared_ptr<PcpPrimIndex> &index, 
        const PcpNodeRef &node, 
        const SdfLayerHandle &layer,
        const PcpNodeRef &stopNode, 
        const SdfLayerHandle &stopLayer);

    // Resolve targets are created with an expanded prim index either from
    // a composition query (which owns and holds it) or from a UsdPrim (which
    // creates it solely to create the resolve target). The expanded prim index
    // is not otherwise cached, so we have to hold on to it during the lifetime
    // of the resolve target.
    std::shared_ptr<PcpPrimIndex> _expandedPrimIndex;
    PcpNodeRange _nodeRange;

    PcpNodeIterator _startNodeIt;
    SdfLayerRefPtrVector::const_iterator _startLayerIt;
    PcpNodeIterator _stopNodeIt;
    SdfLayerRefPtrVector::const_iterator _stopLayerIt;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_RESOLVE_TARGET_H
