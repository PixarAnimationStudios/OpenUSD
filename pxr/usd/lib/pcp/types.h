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
#ifndef PCP_TYPES_H
#define PCP_TYPES_H

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/pcp/site.h"

#include <limits>
#include <vector>
#include <boost/operators.hpp>

/// \enum PcpArcType
///
/// Describes the type of arc connecting two nodes in the prim index.
///
enum PcpArcType {
    // The root arc is a special value used for the direct/root node of 
    // the prim index. Unlike the following arcs, it has no parent node.
    PcpArcTypeRoot,
    
    // The following arcs are listed in strength order.
    PcpArcTypeLocalInherit,
    PcpArcTypeGlobalInherit,
    PcpArcTypeVariant,
    PcpArcTypeRelocate,
    PcpArcTypeReference,
    PcpArcTypePayload,
    PcpArcTypeLocalSpecializes,
    PcpArcTypeGlobalSpecializes,
    
    PcpNumArcTypes
};

/// \enum PcpRangeType
enum PcpRangeType {
    // Ranges including direct arcs of the specified type.
    PcpRangeTypeRoot,
    PcpRangeTypeLocalInherit,
    PcpRangeTypeGlobalInherit,
    PcpRangeTypeVariant,
    PcpRangeTypeReference,
    PcpRangeTypePayload,
    PcpRangeTypeLocalSpecializes,
    PcpRangeTypeGlobalSpecializes,

    // Range including all nodes.
    PcpRangeTypeAll,

    // Range including all direct local and global inherits.
    PcpRangeTypeAllInherits,

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
    return (arcType == PcpArcTypeLocalInherit or
            arcType == PcpArcTypeGlobalInherit);
}

/// Returns true if \p arcType represents a specializes arc, false
/// otherwise.
inline bool 
PcpIsSpecializesArc(PcpArcType arcType)
{
    return (arcType == PcpArcTypeLocalSpecializes or
            arcType == PcpArcTypeGlobalSpecializes);
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
    return PcpIsInheritArc(arcType) or PcpIsSpecializesArc(arcType);
}

/// Returns true if \p arcType represents a local class-based
/// composition arc, false otherwise.
inline bool
PcpIsLocalClassBasedArc(PcpArcType arcType)
{
    return (arcType == PcpArcTypeLocalInherit or
            arcType == PcpArcTypeLocalSpecializes);
}

/// \struct PcpSiteTrackerSegment
///
/// Used to keep track of which sites have been visited and through
/// what type of arcs. 
///
struct PcpSiteTrackerSegment {
    PcpLayerStackSite site;
    PcpArcType arcType;
};

/// Represents a single path through the composition tree. As the tree
/// is being built, we add segments to the tracker. If we encounter a 
/// site that we've already visited, we've found a cycle.
typedef std::vector<PcpSiteTrackerSegment> PcpSiteTracker;

/// \enum PcpDependencyType
///
/// Defines the types of dependencies.
///
enum PcpDependencyType {
    PcpDirect    = (1 << 0),
    PcpAncestral = (1 << 1)
};

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
        return layer < other.layer or
               (not (other.layer < layer) and path < other.path);
    }

    // These are held by reference for performance,
    // to avoid extra ref-counting operations.
    const SdfLayerRefPtr & layer;
    const SdfPath & path;
};

// Internal type for Sd sites.
struct Pcp_CompressedSdSite {
    Pcp_CompressedSdSite(size_t nodeIndex_, size_t layerIndex_) :
        nodeIndex(static_cast<uint16_t>(nodeIndex_)), layerIndex(static_cast<uint16_t>(layerIndex_))
    {
        TF_VERIFY(nodeIndex_  < (1 << 16));
        TF_VERIFY(layerIndex_ < (1 << 16));
    }

    // These are small to minimize the size of vectors of these.
    uint16_t nodeIndex;     // The index of the node in its graph.
    uint16_t layerIndex;    // The index of the layer in the node's layer stack.
};
typedef std::vector<Pcp_CompressedSdSite> Pcp_CompressedSdSiteVector;

/// A list of fallbacks to attempt to use when evaluating
/// variant sets that lack an authored selection.
///
/// This maps a name of a variant set (ex: "shadingComplexity") to a
/// ordered list of variant selection names.  If there is no variant
/// selection in scene description, Pcp will check for each listed
/// fallback in sequence, using the first one that exists.
///
typedef std::map<std::string, std::vector<std::string>> PcpVariantFallbackMap;

/// A value which indicates an invalid index. This is simply used inplace of
/// either -1 or numeric_limits::max() (which are equivalent for size_t). 
/// for better clarity.
constexpr size_t PCP_INVALID_INDEX = std::numeric_limits<size_t>::max();

#endif // PCP_TYPES_H
