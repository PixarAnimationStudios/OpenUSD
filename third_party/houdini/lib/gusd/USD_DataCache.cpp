//
// Copyright 2017 Pixar
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
#include "gusd/USD_DataCache.h"
#include "gusd/USD_PropertyMap.h"
#include "gusd/UT_Gf.h"

#include "pxr/usd/usdGeom/xformable.h"

#include <UT/UT_Matrix4.h>


PXR_NAMESPACE_OPEN_SCOPE


GusdUSD_DataCache::GusdUSD_DataCache(GusdStageCache& cache)
    : _stageCache(cache)
{
    cache.AddDataCache(*this);
}


GusdUSD_DataCache::GusdUSD_DataCache()
    : GusdUSD_DataCache(GusdStageCache::GetInstance())
{}


GusdUSD_DataCache::~GusdUSD_DataCache()
{
    _stageCache.RemoveDataCache(*this);
}

bool
GusdUSD_DataCache::ShouldClearPrim(
    const UsdPrim& prim,
    const UT_StringSet& stagesToClear)
{
    if(!prim) {
        // Always clear expired prims.
        return true;
    }
    const std::string& path = prim.GetStage()->GetRootLayer()->GetRealPath();
    return stagesToClear.contains(path);
}


PXR_NAMESPACE_CLOSE_SCOPE
