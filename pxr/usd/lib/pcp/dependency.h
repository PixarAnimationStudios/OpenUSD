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
#ifndef PCP_DEPENDENCY_H
#define PCP_DEPENDENCY_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/mapFunction.h"
#include "pxr/usd/sdf/path.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class PcpNodeRef;

/// \enum PcpDependencyType
///
/// A classification of PcpPrimIndex->PcpSite dependencies
/// by composition structure.
///
enum PcpDependencyType {
    /// No type of dependency.
    PcpDependencyTypeNone = 0,

    /// The root dependency of a cache on its root site.
    /// This may be useful to either include, as when invalidating
    /// caches in response to scene edits, or to exclude, as when
    /// scanning dependency arcs to compensate for a namespace edit.
    PcpDependencyTypeRoot = (1 << 0),

    /// Purely direct dependencies involve only arcs introduced
    /// directly at this level of namespace.
    PcpDependencyTypePurelyDirect = (1 << 1),

    /// Partly direct dependencies involve at least one arc introduced
    /// directly at this level of namespace; they may also involve
    /// ancestral arcs along the chain as well.
    PcpDependencyTypePartlyDirect = (1 << 2),

    /// Ancestreal dependencies involve only arcs from ancestral
    /// levels of namespace, and no direct arcs.
    PcpDependencyTypeAncestral = (1 << 3),

    /// Virtual dependencies do not contribute scene description,
    /// yet represent sites whose scene description (or ancestral
    /// scene description) informed the structure of the cache.
    ///
    /// One case of this is when a reference or payload arc
    /// does not specify a prim, and the target layerStack does
    /// not provide defaultPrim metadata either.  In that case
    /// a virtual dependency to the root of that layer stack will
    /// represent the latent dependency on that site's metadata.
    ///
    /// Another case of this is "spooky ancestral" dependencies from
    /// relocates. These are referred to as "spooky" dependencies
    /// because they can be seen as a form of action-at-a-distance. They
    /// only occur as a result of relocation arcs.
    PcpDependencyTypeVirtual = (1 << 4),
    PcpDependencyTypeNonVirtual = (1 << 5),

    /// Combined mask value representing both pure and partly direct
    /// deps.
    PcpDependencyTypeDirect =
        PcpDependencyTypePartlyDirect
        | PcpDependencyTypePurelyDirect,

    /// Combined mask value representing any kind of dependency,
    /// except virtual ones.
    PcpDependencyTypeAnyNonVirtual =
        PcpDependencyTypeRoot
        | PcpDependencyTypeDirect
        | PcpDependencyTypeAncestral
        | PcpDependencyTypeNonVirtual,

    /// Combined mask value representing any kind of dependency.
    PcpDependencyTypeAnyIncludingVirtual =
        PcpDependencyTypeAnyNonVirtual
        | PcpDependencyTypeVirtual,
};

/// A typedef for a bitmask of flags from PcpDependencyType.
typedef unsigned int PcpDependencyFlags;

/// Description of a dependency.
struct PcpDependency {
    /// The path in this PcpCache's root layer stack that depends
    /// on the site.
    SdfPath indexPath;
    /// The site path.  When using recurseDownNamespace, this may
    /// be a path beneath the initial sitePath.
    SdfPath sitePath;
    /// The map function that applies to values from the site.
    PcpMapFunction mapFunc;

    bool operator==(const PcpDependency &rhs) const {
        return indexPath == rhs.indexPath &&
            sitePath == rhs.sitePath &&
            mapFunc == rhs.mapFunc;
    }
    bool operator!=(const PcpDependency &rhs) const {
        return !(*this == rhs);
    }
};

typedef std::vector<PcpDependency> PcpDependencyVector;

/// Classify the dependency represented by a node, by analyzing
/// its structural role in its PcpPrimIndex.  Returns a
/// bitmask of flags from PcpDependencyType.
PCP_API
PcpDependencyFlags PcpClassifyNodeDependency(const PcpNodeRef &n);

PCP_API
std::string PcpDependencyFlagsToString(const PcpDependencyFlags flags);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PCP_DEPENDENCY_H
