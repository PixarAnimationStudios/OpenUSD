//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
        const TfToken& modelName,
        const std::vector<std::pair<std::string, std::string> >&variantSelections)
{
    // Sort so that the key is deterministic.
    std::vector<std::pair<std::string, std::string> > variantSelectionsSorted(
        variantSelections.begin(), variantSelections.end());
    std::sort(variantSelectionsSorted.begin(), variantSelectionsSorted.end());

    std::string sessionKey = modelName;
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
                SdfPrimSpecHandle over = SdfPrimSpec::New(
                    layer,
                    modelName,
                    SdfSpecifierOver);
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

