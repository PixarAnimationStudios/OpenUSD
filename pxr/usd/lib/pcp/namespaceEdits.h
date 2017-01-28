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
#ifndef PCP_NAMESPACE_EDITS_H
#define PCP_NAMESPACE_EDITS_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/cache.h"
#include "pxr/base/tf/hashset.h"

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Forward declarations:
class PcpChanges;
class PcpCacheChanges;
class Pcp_Dependencies;
class PcpLayerStackIdentifier;
class PcpLifeboat;
class PcpNodeRef;
class PcpMapFunction;

TF_DECLARE_WEAK_AND_REF_PTRS(PcpLayerStack);
TF_DECLARE_WEAK_AND_REF_PTRS(Pcp_LayerStackRegistry);
TF_DECLARE_REF_PTRS(PcpPayloadDecorator);
SDF_DECLARE_HANDLES(SdfSpec);

/// Sites that must respond to a namespace edit.
struct PcpNamespaceEdits {

    /// Types of namespace edits that a given layer stack site could need
    /// to perform to respond to a namespace edit.
    enum EditType {
        EditPath,      ///< Must namespace edit spec
        EditInherit,   ///< Must fixup inherits
        EditReference, ///< Must fixup references
        EditPayload,   ///< Must fixup payload
        EditRelocate,  ///< Must fixup relocates
    };

    void Swap(PcpNamespaceEdits& rhs)
    {
        cacheSites.swap(rhs.cacheSites);
        layerStackSites.swap(rhs.layerStackSites);
        invalidLayerStackSites.swap(rhs.invalidLayerStackSites);
    }

    /// Cache site that must respond to a namespace edit.
    struct CacheSite {
        size_t cacheIndex;  ///< Index of cache of site.
        SdfPath oldPath;    ///< Old path of site.
        SdfPath newPath;    ///< New path of site.
    };
    typedef std::vector<CacheSite> CacheSites;

    /// Layer stack site that must respond to a namespace edit.  All
    /// of the specs at the site will respond the same way.
    struct LayerStackSite {
        size_t cacheIndex;              ///< Index of cache of site.
        EditType type;                  ///< Type of edit.
        PcpLayerStackPtr layerStack;    ///< Layer stack needing fix.
        SdfPath sitePath;               ///< Path of site needing fix.
        SdfPath oldPath;                ///< Old path.
        SdfPath newPath;                ///< New path.
    };
    typedef std::vector<LayerStackSite> LayerStackSites;

    /// Cache sites that must respond to a namespace edit.
    CacheSites cacheSites;

    /// Layer stack sites that must respond to a namespace edit.
    LayerStackSites layerStackSites;

    /// Layer stack sites that are affected by a namespace edit but
    /// cannot respond properly. For example, in situations involving
    /// relocates, a valid namespace edit in one cache may result in
    /// an invalid edit in another cache in response.
    LayerStackSites invalidLayerStackSites;
};

/// Returns the changes caused in any cache in \p caches due to
/// namespace editing the object at \p curPath in this cache to
/// have the path \p newPath.  \p caches should have all caches,
/// including this cache.  If \p caches includes this cache then
/// the result includes the changes caused at \p curPath in this
/// cache itself.
///
/// To keep everything consistent, a namespace edit requires that
/// everything using the namespace edited site to be changed in an
/// appropriate way.  For example, if a referenced prim /A is renamed
/// to /B then everything referencing /A must be changed to reference
/// /B instead.  There are many other possibilities.
///
/// One possibility is that there are no opinions at \p curPath in
/// this cache's layer stack and the site exists due to some ancestor
/// arc.  This requires a relocation and only sites using \p curPath
/// that include the layer with the relocation must be changed in
/// response.  To find those sites, \p relocatesLayer indicates which
/// layer the client will write the relocation to.
///
/// Clients must perform the changes to correctly perform a namespace
/// edit.  All changes must be performed in a change block, otherwise
/// notices could be sent prematurely.
///
/// This method only works when the affected prim indexes have been
/// computed.  In general, this means you must have computed the prim
/// index of everything in any existing cache, otherwise you might miss
/// changes to objects in those caches that use the namespace edited
/// object.  Using the above example, if a prim with an uncomputed prim
/// index referenced /A then this method would not report that prim. 
/// As a result that prim would continue to reference /A, which no
/// longer exists.
PcpNamespaceEdits
PcpComputeNamespaceEdits(const PcpCache *primaryCache,
                         const std::vector<PcpCache*>& caches,
                         const SdfPath& curPath,
                         const SdfPath& newPath,
                         const SdfLayerHandle& relocatesLayer);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PCP_NAMESPACE_EDITS_H
