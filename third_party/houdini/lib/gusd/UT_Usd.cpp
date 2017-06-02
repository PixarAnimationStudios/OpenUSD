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
#include "UT_Usd.h"

#include "gusd/UT_Assert.h"
#include "gusd/UT_Error.h"

#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/usd/stageCache.h"
#include "pxr/usd/usd/stageCacheContext.h"
#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdUtils/stageCache.h"

#include "pxr/usd/ar/resolver.h"

#include <UT/UT_Assert.h>
#include <SYS/SYS_AtomicInt.h>
#include <UT/UT_ConcurrentHashMap.h>
#include <UT/UT_Error.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_ParallelUtil.h>

#include <algorithm>


PXR_NAMESPACE_OPEN_SCOPE

using std::pair;
using std::size_t;
using std::string;
using std::stringstream;
using std::vector;

// Cache of keys (representing variant selections) to session layers.
typedef TfHashMap<TfToken, SdfLayerRefPtr, TfToken::HashFunctor>
    GusdSessionLayerMap;
static TfStaticData<GusdSessionLayerMap> _sessionLayerMap;

UsdStageRefPtr
GusdUT_GetStage(const char* file, std::string* err)
{
    if(!file)
        return nullptr;

    UsdStageCache& cache = UsdUtilsStageCache::Get();
    UsdStageCacheContext ctx(cache);
    if(UsdStageRefPtr stage = 
            UsdStage::Open(file,ArGetResolver().GetCurrentContext()))
    {
        UT_ASSERT(cache.Contains(stage));
        return stage;
    }
    if(err)
    {
        stringstream ss;
        ss << "Unable to open stage, \"" << file
           << "\", for reading";
        *err = ss.str();
    }
    return nullptr;
}

UsdStageRefPtr
GusdUT_GetStage(
        const char* file,
        SdfLayerHandle sessionLayer,
        std::string* err)
{
    if(!file)
        return nullptr;

    UsdStageCache& cache = UsdUtilsStageCache::Get();
    UsdStageCacheContext ctx(cache);

    UsdStageRefPtr stage;
    if(SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(file)) {
        if(UsdStageRefPtr stage = 
           UsdStage::Open(rootLayer, 
                          sessionLayer,
                          ArGetResolver().GetCurrentContext())) {
            return stage;
        }
    }

    if(err)
    {
        stringstream ss;
        ss << "Unable to open stage, \"" << file
           << "\", for reading";
        *err = ss.str();
    }
    return nullptr;
}

UsdStageRefPtr GusdUT_GetStage(
        const char* file,
        const GusdVariantSelectionVec& primVariants,
        std::string* err)
{
    if(!UTisstring(file)) {
        return nullptr;
    }

    if(primVariants.empty()) {
        return GusdUT_GetStage(file, err);
    }

    stringstream ssKey;
    GusdVariantSelectionVec primVariantsSorted(
            primVariants.begin(), primVariants.end());
    std::sort(primVariantsSorted.begin(), primVariantsSorted.end());
    for(auto& primSelection : primVariantsSorted) {
        ssKey << primSelection.first;
        vector<pair<string, string> > variantsSorted(
                primSelection.second.begin(), primSelection.second.end());
        std::sort(variantsSorted.begin(), variantsSorted.end());
        for(auto& variantPair : variantsSorted) {
            ssKey << "{" << variantPair.first << "=" << variantPair.second << "}";
        }
    }

    TfToken sessionKey(ssKey.str());
    SdfLayerRefPtr sessionLayer;

    static std::mutex sessionLayerMapLock;
    std::lock_guard<std::mutex> lock(sessionLayerMapLock);

    auto sessionLayerIt = _sessionLayerMap->find(sessionKey);
    if(sessionLayerIt == _sessionLayerMap->end()) {
        sessionLayer = SdfLayer::CreateAnonymous();
        for(auto& primSelection : primVariants) {
            SdfPrimSpecHandle over = SdfCreatePrimInLayer(
                                    sessionLayer,
                                    SdfPath(primSelection.first.GetString()));
            for(auto& variantPair : primSelection.second) {
                over->GetVariantSelections()[variantPair.first]
                    = variantPair.second;
            }
        }
        (*_sessionLayerMap)[sessionKey] = sessionLayer;
    }
    else {
        sessionLayer = sessionLayerIt->second;
    }

    return GusdUT_GetStage(file, sessionLayer, err);
}


UsdPrim
GusdUT_GetPrim(const UsdStageRefPtr& stage,
               const SdfPath& primPath,
               string* err)
{
    UT_ASSERT_P(stage);

    if(UsdPrim prim = stage->GetPrimAtPath(primPath))
        return prim;
    if(err) {
        stringstream ss;
        ss << "Unable to find prim '" << primPath << "' in stage '"
           << stage->GetRootLayer()->GetIdentifier() << "'";
        *err = ss.str();
    }
    return UsdPrim();
}


bool
GusdUT_GetLayer(const char* file,
                SdfLayerRefPtr& layer,
                GusdUT_ErrorContext* err)
{
    if(UTisstring(file)) {
        GusdUT_TfErrorScope scope(err);
        if(layer = SdfLayer::FindOrOpen(file))
            return true;
        return false;
    }
    layer = nullptr;
    return true;
}


namespace {


/* TODO: This old form uses a std::string to report errors.
   This is still needed for backwards-compatibility with old code.
   But ideally, this would be using GusdUT_ErrorContext instead.*/
bool
_CreateSdfPath(const char* pathStr, SdfPath& path, std::string* err=NULL)
{
    /* XXX: Paths require parsing which, when dealing with many
            thousands of prims, can be expensive to continually
            recompute. To speed things up, cache the conversions.
            We only cache valid conversions so that we don't have to
            worry about also caching error messages.*/

    if(BOOST_UNLIKELY(!UTisstring(pathStr)))
        return true;
    
    typedef UT_ConcurrentHashMap<UT_StringHolder,SdfPath> PathMap;
    static PathMap map;

    {
        PathMap::const_accessor a;
        if(map.find(a, UT_StringRef(pathStr))) {
            path = a->second;
            return true;
        }
    }
    
    std::string str(pathStr);
    /* TODO: using 'IsValidPathString()' requires us to parse the
       path a second time. It would be better to parse a single 
       time, and capture any warnings produced while parsing.
       This isn't currently possible because Tf warnings can't
       be captured with marks. See BUG: 127366 */
    if(SdfPath::IsValidPathString(str, err)) {
        PathMap::accessor a;
        if(map.insert(a, UT_StringHolder(pathStr)))
            a->second = SdfPath(str);
        path = a->second;
        return true;
    } else {
        if(err) {
            UT_WorkBuffer buf;
            buf.sprintf("Failed parsing path '%s': %s", pathStr, err->c_str());
            *err = buf.toStdString();
        }
        return false;
    }
}


} /*namespace*/


bool
GusdUT_CreateSdfPath(const char* pathStr, SdfPath& path,
                     GusdUT_ErrorContext* err)
{
    if(err) {
        std::string errStr;
        if(_CreateSdfPath(pathStr, path, &errStr))
            return true;
        err->AddError(errStr.c_str());
    }
    return _CreateSdfPath(pathStr, path);
}


UsdPrim 
GusdUT_GetPrim(
        const char* file,
        const char* primPath,
        std::string* err)
{
    if(!UTisstring(file))
        return UsdPrim();

    SdfPath sdfPrimPath;
    if(!_CreateSdfPath(primPath, sdfPrimPath, err))
        return UsdPrim();

    if(sdfPrimPath.ContainsPrimVariantSelection()) {
        SdfPath sdfPrimPathNoVariants(sdfPrimPath.StripAllVariantSelections());
        GusdVariantSelectionVec variantVec;
        // Parse path for variant selections.
        SdfPath p(sdfPrimPath);
        while( p != SdfPath::EmptyPath() ) {
            if( p.IsPrimVariantSelectionPath() ) {
                variantVec.push_back(std::make_pair(
                            p.StripAllVariantSelections(),
                            GusdVariantSelection()));
                variantVec.back().second.push_back(p.GetVariantSelection());
            }
            
            p = p.GetParentPath();
        }

        if(UsdStageRefPtr stage = GusdUT_GetStage(file, variantVec, err)) {
            return GusdUT_GetPrim(stage, sdfPrimPathNoVariants, err);
        }
    }
    else {
        if(UsdStageRefPtr stage = GusdUT_GetStage(file, err)) {
            return GusdUT_GetPrim(stage, sdfPrimPath, err);
        }
    }
    return UsdPrim();
}


void 
GusdUT_GetInheritedPrimInfo(
    const UsdPrim& prim,
    bool& active,
    TfToken& purpose)
{
    active = prim.IsActive();
    purpose = UsdGeomImageable(prim).ComputePurpose();
}


PXR_NAMESPACE_CLOSE_SCOPE
