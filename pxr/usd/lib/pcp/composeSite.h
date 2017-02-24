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
#ifndef PCP_COMPOSE_SITE_H
#define PCP_COMPOSE_SITE_H

/// \file pcp/composeSite.h
///
/// Single-site composition.
///
/// These are helpers that compose specific fields at single sites.
/// They compose the field for a given path across a layer stack,
/// using field-specific rules to combine the values.
/// 
/// These helpers are low-level utilities used by the rest of the
/// Pcp algorithms, to discover composition arcs in scene description.
/// These arcs are what guide the algorithm to pull additional
/// sites of scene description into the PcpPrimIndex.
///
/// Some of these field types support list-editing.  (See SdListOp.)
/// List-editing for these fields is applied across the fixed domain
/// of a single site; you cannot apply list-ops across sites.
/// The intention is to avoid subtle ordering issues in composition
/// semantics.
///
/// Note that these helpers do not take PcpSite as a literal parameter;
/// instead, they require the actual computed layer stack that a site
/// identified.  Rather than tying these helpers to PcpCache and its
/// process of computing layer stacks, they just employ the result.
/// Conceptually, though, they are operating on the scene description
/// identified by a PcpSite.

#include "pxr/pxr.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/site.h"

#include <set>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(PcpLayerStack);

class PcpLayerStackSite;

/// \struct PcpSourceReferenceInfo
///
/// Information about reference arcs.
///
struct PcpSourceReferenceInfo {
    SdfLayerHandle layer;
    SdfLayerOffset layerOffset;
};

/// A vector of reference arc information.
typedef std::vector<PcpSourceReferenceInfo> PcpSourceReferenceInfoVector;

/// References
void
PcpComposeSiteReferences(PcpLayerStackRefPtr const &layerStack,
                         SdfPath const &path,
                         SdfReferenceVector *result,
                         PcpSourceReferenceInfoVector *info);
inline void
PcpComposeSiteReferences(PcpNodeRef const &node,
                         SdfReferenceVector *result,
                         PcpSourceReferenceInfoVector *info)
{
    return PcpComposeSiteReferences(node.GetLayerStack(), node.GetPath(),
                                    result, info);
}

/// Payload
void
PcpComposeSitePayload(PcpLayerStackRefPtr const &layerStack,
                      SdfPath const &path,
                      SdfPayload *result,
                      SdfLayerHandle *sourceLayer);
inline void
PcpComposeSitePayload(PcpNodeRef const &node,
                      SdfPayload *result,
                      SdfLayerHandle *sourceLayer)
{
    return PcpComposeSitePayload(node.GetLayerStack(), node.GetPath(),
                                 result, sourceLayer);
}

/// Permission
SdfPermission
PcpComposeSitePermission(PcpLayerStackRefPtr const &layerStack,
                         SdfPath const &path);

inline SdfPermission
PcpComposeSitePermission(PcpNodeRef const &node)
{
    return PcpComposeSitePermission(node.GetLayerStack(), node.GetPath());
}

/// Prim sites
void
PcpComposeSitePrimSites(PcpLayerStackRefPtr const &layerStack,
                        SdfPath const &path,
                        SdfSiteVector *result);

inline void
PcpComposeSitePrimSites(PcpNodeRef const &node, SdfSiteVector *result)
{
    return PcpComposeSitePrimSites(
        node.GetLayerStack(), node.GetPath(), result);
}

/// Relocates
void
PcpComposeSiteRelocates(PcpLayerStackRefPtr const &layerStack,
                        SdfPath const &path,
                        SdfRelocatesMap *result);

inline void
PcpComposeSiteRelocates(PcpNodeRef const &node, SdfRelocatesMap *result)
{
    return PcpComposeSiteRelocates(
        node.GetLayerStack(), node.GetPath(), result);
}

/// Has prim specs.
bool
PcpComposeSiteHasPrimSpecs(PcpLayerStackRefPtr const &layerStack,
                           SdfPath const &path);
inline bool
PcpComposeSiteHasPrimSpecs(PcpNodeRef const &node)
{
    return PcpComposeSiteHasPrimSpecs(node.GetLayerStack(), node.GetPath());
}

/// Symmetry
bool
PcpComposeSiteHasSymmetry(PcpLayerStackRefPtr const &layerStack,
                          SdfPath const &path);
inline bool
PcpComposeSiteHasSymmetry(PcpNodeRef const &node)
{
    return PcpComposeSiteHasSymmetry(node.GetLayerStack(), node.GetPath());
}

/// Inherits
void
PcpComposeSiteInherits(PcpLayerStackRefPtr const &layerStack,
                       SdfPath const &path, SdfPathVector *result);
inline void
PcpComposeSiteInherits(PcpNodeRef const &node, SdfPathVector *result)
{
    return PcpComposeSiteInherits(node.GetLayerStack(), node.GetPath(), result);
}

/// Specializes
void
PcpComposeSiteSpecializes(PcpLayerStackRefPtr const &layerStack,
                          SdfPath const &path, SdfPathVector *result);
inline void
PcpComposeSiteSpecializes(PcpNodeRef const &node, SdfPathVector *result)
{
    return PcpComposeSiteSpecializes(
        node.GetLayerStack(), node.GetPath(), result);
}

/// VariantSets
void
PcpComposeSiteVariantSets(PcpLayerStackRefPtr const &layerStack,
                          SdfPath const &path,
                          std::vector<std::string> *result);
inline void
PcpComposeSiteVariantSets(PcpNodeRef const &node,
                          std::vector<std::string> *result) {
    return PcpComposeSiteVariantSets(
        node.GetLayerStack(), node.GetPath(), result);
}

/// VariantSetOptions
void
PcpComposeSiteVariantSetOptions(PcpLayerStackRefPtr const &layerStack,
                                SdfPath const &path,
                                std::string const &vsetName,
                                std::set<std::string> *result);
inline void
PcpComposeSiteVariantSetOptions(PcpNodeRef const &node,
                                std::string const &vsetName,
                                std::set<std::string> *result)
{
    return PcpComposeSiteVariantSetOptions(
        node.GetLayerStack(), node.GetPath(), vsetName, result);
}

/// VariantSelection
bool
PcpComposeSiteVariantSelection(PcpLayerStackRefPtr const &layerStack,
                               SdfPath const &path,
                               std::string const &vsetName,
                               std::string *result);
inline bool
PcpComposeSiteVariantSelection(PcpNodeRef const &node,
                               std::string const &vsetName,
                               std::string *result)
{
    return PcpComposeSiteVariantSelection(node.GetLayerStack(), node.GetPath(),
                                          vsetName, result);
}

/// VariantSelections
void 
PcpComposeSiteVariantSelections(PcpLayerStackRefPtr const &layerStack,
                                SdfPath const &path,
                                SdfVariantSelectionMap *result);
inline void
PcpComposeSiteVariantSelections(PcpNodeRef const &node,
                                SdfVariantSelectionMap *result)
{
    return PcpComposeSiteVariantSelections(node.GetLayerStack(), node.GetPath(),
                                           result);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PCP_COMPOSE_SITE_H
