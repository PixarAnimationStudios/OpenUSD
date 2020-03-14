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
#ifndef PXR_USD_PCP_TYPES_H
#define PXR_USD_PCP_TYPES_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/site.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/denseHashSet.h"

#include <limits>
#include <vector>

#include <boost/operators.hpp>

/// \file pcp/types.h

PXR_NAMESPACE_OPEN_SCOPE

/// \enum PcpArcType
///
/// Describes the type of arc connecting two nodes in the prim index.
///
enum PcpArcType {
    // The root arc is a special value used for the root node of 
    // the prim index. Unlike the following arcs, it has no parent node.
    PcpArcTypeRoot,
    
    // The following arcs are listed in strength order.
    PcpArcTypeInherit,
    PcpArcTypeVariant,
    PcpArcTypeRelocate,
    PcpArcTypeReference,
    PcpArcTypePayload,
    PcpArcTypeSpecialize,
    
    PcpNumArcTypes
};

/// \enum PcpRangeType
enum PcpRangeType {
    // Range including just the root node.
    PcpRangeTypeRoot,

    // Ranges including child arcs, from the root node, of the specified type 
    // as well as all descendants of those arcs.
    PcpRangeTypeInherit,
    PcpRangeTypeVariant,
    PcpRangeTypeReference,
    PcpRangeTypePayload,
    PcpRangeTypeSpecialize,

    // Range including all nodes.
    PcpRangeTypeAll,

    // Range including all nodes weaker than the root node.
    PcpRangeTypeWeakerThanRoot,

    // Range including all nodes stronger than the payload
    // node.
    PcpRangeTypeStrongerThanPayload,

    PcpRangeTypeInvalid
};

/// Returns true if \p arcType represents an inherit arc, false
/// otherwise.
inline bool 
PcpIsInheritArc(PcpArcType arcType)
{
    return (arcType == PcpArcTypeInherit);
}

/// Returns true if \p arcType represents a specialize arc, false
/// otherwise.
inline bool 
PcpIsSpecializeArc(PcpArcType arcType)
{
    return (arcType == PcpArcTypeSpecialize);
}

/// Returns true if \p arcType represents a class-based 
/// composition arc, false otherwise.
///
/// The key characteristic of these arcs is that they imply 
/// additional sources of opinions outside of the site where 
/// the arc is introduced.
inline bool
PcpIsClassBasedArc(PcpArcType arcType)
{
    return PcpIsInheritArc(arcType) || PcpIsSpecializeArc(arcType);
}

/// \struct PcpSiteTrackerSegment
///
/// Used to keep track of which sites have been visited and through
/// what type of arcs. 
///
struct PcpSiteTrackerSegment {
    PcpSiteStr site;
    PcpArcType arcType;
};

/// \typedef std::vector<PcpSiteTrackerSegment> PcpSiteTracker
/// Represents a single path through the composition tree. As the tree
/// is being built, we add segments to the tracker. If we encounter a 
/// site that we've already visited, we've found a cycle.
typedef std::vector<PcpSiteTrackerSegment> PcpSiteTracker;

// Internal type for Sd sites.
struct Pcp_SdSiteRef : boost::totally_ordered<Pcp_SdSiteRef> {
    Pcp_SdSiteRef(const SdfLayerRefPtr& layer_, const SdfPath& path_) :
        layer(layer_), path(path_)
    {
        // Do nothing
    }

    bool operator==(const Pcp_SdSiteRef& other) const
    {
        return layer == other.layer && path == other.path;
    }

    bool operator<(const Pcp_SdSiteRef& other) const
    {
        return layer < other.layer ||
               (!(other.layer < layer) && path < other.path);
    }

    // These are held by reference for performance,
    // to avoid extra ref-counting operations.
    const SdfLayerRefPtr & layer;
    const SdfPath & path;
};

// Internal type for Sd sites.
struct Pcp_CompressedSdSite {
    Pcp_CompressedSdSite(size_t nodeIndex_, size_t layerIndex_) :
        nodeIndex(static_cast<uint16_t>(nodeIndex_)),
        layerIndex(static_cast<uint16_t>(layerIndex_))
    {
        TF_VERIFY(nodeIndex_  < (size_t(1) << 16));
        TF_VERIFY(layerIndex_ < (size_t(1) << 16));
    }

    // These are small to minimize the size of vectors of these.
    uint16_t nodeIndex;     // The index of the node in its graph.
    uint16_t layerIndex;    // The index of the layer in the node's layer stack.
};
typedef std::vector<Pcp_CompressedSdSite> Pcp_CompressedSdSiteVector;

// XXX Even with <map> included properly, doxygen refuses to acknowledge
// the existence of std::map, so if we include the full typedef in the
// \typedef directive, it will warn and fail to produce an entry for 
// PcpVariantFallbackMap.  So we instead put the decl inline.
/// \typedef PcpVariantFallbackMap
/// typedef std::map<std::string, std::vector<std::string>> PcpVariantFallbackMap
///
/// A "map of lists" of fallbacks to attempt to use when evaluating
/// variant sets that lack an authored selection.
///
/// This maps a name of a variant set (ex: "shadingComplexity") to a
/// ordered list of variant selection names.  If there is no variant
/// selection in scene description, Pcp will check for each listed
/// fallback in sequence, using the first one that exists.
///
typedef std::map<std::string, std::vector<std::string>> PcpVariantFallbackMap;

typedef TfDenseHashSet<TfToken, TfToken::HashFunctor> PcpTokenSet;

/// \var size_t PCP_INVALID_INDEX
/// A value which indicates an invalid index. This is simply used inplace of
/// either -1 or numeric_limits::max() (which are equivalent for size_t). 
/// for better clarity.
#if defined(doxygen)
constexpr size_t PCP_INVALID_INDEX = unspecified;
#else
constexpr size_t PCP_INVALID_INDEX = std::numeric_limits<size_t>::max();
#endif

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_TYPES_H
