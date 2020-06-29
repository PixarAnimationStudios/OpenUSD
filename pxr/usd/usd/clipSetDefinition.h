//
// Copyright 2020 Pixar
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
#ifndef PXR_USD_USD_CLIP_SET_DEFINITION_H
#define PXR_USD_USD_CLIP_SET_DEFINITION_H

#include "pxr/pxr.h"

#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/tf/declarePtrs.h"

#include <boost/optional.hpp>

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class PcpPrimIndex;
TF_DECLARE_WEAK_PTRS(PcpLayerStack);

/// \class Usd_ClipSetDefinition
///
/// Collection of metadata from scene description and other information that
/// uniquely defines a clip set.
class Usd_ClipSetDefinition
{
public:
    Usd_ClipSetDefinition() 
        : interpolateMissingClipValues(false)
        , indexOfLayerWhereAssetPathsFound(0) 
    {
    }

    bool operator==(const Usd_ClipSetDefinition& rhs) const
    {
        return (clipAssetPaths == rhs.clipAssetPaths
            && clipManifestAssetPath == rhs.clipManifestAssetPath
            && clipPrimPath == rhs.clipPrimPath
            && clipActive == rhs.clipActive
            && clipTimes == rhs.clipTimes
            && interpolateMissingClipValues == rhs.interpolateMissingClipValues
            && sourceLayerStack == rhs.sourceLayerStack
            && sourcePrimPath == rhs.sourcePrimPath
            && indexOfLayerWhereAssetPathsFound 
                    == rhs.indexOfLayerWhereAssetPathsFound);
    }

    bool operator!=(const Usd_ClipSetDefinition& rhs) const
    {
        return !(*this == rhs);
    }

    size_t GetHash() const
    {
        size_t hash = indexOfLayerWhereAssetPathsFound;
        boost::hash_combine(hash, sourceLayerStack);
        boost::hash_combine(hash, sourcePrimPath);

        if (clipAssetPaths) {
            for (const auto& assetPath : *clipAssetPaths) {
                boost::hash_combine(hash, assetPath.GetHash());
            }
        }
        if (clipManifestAssetPath) {
            boost::hash_combine(hash, clipManifestAssetPath->GetHash());
        }
        if (clipPrimPath) {
            boost::hash_combine(hash, *clipPrimPath);
        }               
        if (clipActive) {
            for (const auto& active : *clipActive) {
                boost::hash_combine(hash, active[0]);
                boost::hash_combine(hash, active[1]);
            }
        }
        if (clipTimes) {
            for (const auto& time : *clipTimes) {
                boost::hash_combine(hash, time[0]);
                boost::hash_combine(hash, time[1]);
            }
        }
        if (interpolateMissingClipValues) {
            boost::hash_combine(hash, *interpolateMissingClipValues);
        }

        return hash;
    }

    boost::optional<VtArray<SdfAssetPath> > clipAssetPaths;
    boost::optional<SdfAssetPath> clipManifestAssetPath;
    boost::optional<std::string> clipPrimPath;
    boost::optional<VtVec2dArray> clipActive;
    boost::optional<VtVec2dArray> clipTimes;
    boost::optional<bool> interpolateMissingClipValues;

    PcpLayerStackPtr sourceLayerStack;
    SdfPath sourcePrimPath;
    size_t indexOfLayerWhereAssetPathsFound;
};

/// Computes clip set definitions for the given \p primIndex and returns
/// them in \p clipSetDefinitions. The clip sets in this vector are sorted in
/// strength order. If \p clipSetNames is provided it will contain the name
/// for each clip set in the corresponding position in \p clipSetDefinitions.
void
Usd_ComputeClipSetDefinitionsForPrimIndex(
    const PcpPrimIndex& primIndex,
    std::vector<Usd_ClipSetDefinition>* clipSetDefinitions,
    std::vector<std::string>* clipSetNames = nullptr);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
