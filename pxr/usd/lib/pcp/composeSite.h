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

#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/site.h"

#include "pxr/base/tf/denseHashSet.h"

#include <set>
#include <vector>

class PcpLayerStackSite;

typedef TfDenseHashSet<TfToken, TfToken::HashFunctor> PcpTokenSet;

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

void PcpComposeSiteReferences( const PcpLayerStackSite & site,
                               SdfReferenceVector *result,
                               PcpSourceReferenceInfoVector *info );
void PcpComposeSitePayload( const PcpLayerStackSite & site,
                            SdfPayload *result,
                            SdfLayerHandle *sourceLayer );
SdfPermission PcpComposeSitePermission( const PcpLayerStackSite & site );
void PcpComposeSitePrimSites( const PcpLayerStackSite & site, 
                              SdfSiteVector *result );
void PcpComposeSitePrimSpecs( const PcpLayerStackSite & site, 
                              SdfPrimSpecHandleVector *result );
void PcpComposeSiteRelocates( const PcpLayerStackSite & site,
                              SdfRelocatesMap *result );
bool PcpComposeSiteHasPrimSpecs( const PcpLayerStackSite& site );
bool PcpComposeSiteHasSymmetry( const PcpLayerStackSite & site );
void PcpComposeSiteInherits( const PcpLayerStackSite & site,
                              SdfPathVector *result );
void PcpComposeSiteSpecializes( const PcpLayerStackSite & site,
                                SdfPathVector *result );
void PcpComposeSiteVariantSets( const PcpLayerStackSite & site,
                                std::vector<std::string> *result );
void PcpComposeSiteVariantSetOptions( const PcpLayerStackSite & site,
                                      const std::string &vsetName,
                                      std::set<std::string> *result );
bool PcpComposeSiteVariantSelection( const PcpLayerStackSite & site,
                                     const std::string & vsetName,
                                     std::string *result );
void PcpComposeSiteVariantSelections( const PcpLayerStackSite & site,
                                      SdfVariantSelectionMap *result );
bool PcpComposeSiteHasVariantSelections( const PcpLayerStackSite & site );

#endif
