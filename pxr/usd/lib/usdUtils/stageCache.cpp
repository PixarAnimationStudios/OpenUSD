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
#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/stageCache.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"

#include "pxr/base/tf/hashmap.h"
#include <algorithm>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE


namespace {

// Cache of string keys (currently representing variant selections) to session
// layers.
typedef TfHashMap<std::string, SdfLayerRefPtr, TfHash> _SessionLayerMap;

_SessionLayerMap&
GetSessionLayerMap()
{
    // Heap-allocate and deliberately leak this static cache to avoid
    // problems with static destruction order.
    static _SessionLayerMap *sessionLayerMap = new _SessionLayerMap();
    return *sessionLayerMap;
}

}

UsdStageCache&
UsdUtilsStageCache::Get()
{
    // Heap-allocate and deliberately leak this static cache to avoid
    // problems with static destruction order.
    static UsdStageCache *theCache = new UsdStageCache();
    return *theCache;
}

SdfLayerRefPtr 
UsdUtilsStageCache::GetSessionLayerForVariantSelections(
        const SdfPath& primPath,
        const std::vector<std::pair<std::string, std::string> >&variantSelections)
{
    // Sort so that the key is deterministic.
    std::vector<std::pair<std::string, std::string> > variantSelectionsSorted(
        variantSelections.begin(), variantSelections.end());
    std::sort(variantSelectionsSorted.begin(), variantSelectionsSorted.end());

    std::string sessionKey = primPath.GetString();
    TF_FOR_ALL(itr, variantSelectionsSorted) {
        sessionKey += ":" + itr->first + "=" + itr->second;
    }

    SdfLayerRefPtr ret;
    {
        static std::mutex sessionLayerMapLock;
        std::lock_guard<std::mutex> lock(sessionLayerMapLock);

        _SessionLayerMap& sessionLayerMap = GetSessionLayerMap();
        _SessionLayerMap::iterator itr = sessionLayerMap.find(sessionKey);
        if (itr == sessionLayerMap.end()) {
            SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
            if (!variantSelections.empty()) {
                SdfPrimSpecHandle over = SdfCreatePrimInLayer(
                    layer,
                    primPath);
                TF_FOR_ALL(varSelItr, variantSelections) {
                    // Construct the variant opinion for the session layer.
                    over->GetVariantSelections()[varSelItr->first] =
                        varSelItr->second;
                }
            }
            sessionLayerMap[sessionKey] = layer;
            ret = layer;
        } else {
            ret = itr->second;
        }
    }
    return ret;
}


PXR_NAMESPACE_CLOSE_SCOPE

