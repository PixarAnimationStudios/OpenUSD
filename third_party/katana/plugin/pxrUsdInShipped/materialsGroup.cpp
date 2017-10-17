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

#include <list>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "pxr/pxr.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/stopwatch.h"     // profiling
#include "pxr/usd/usdShade/material.h"

#include "usdKatana/blindDataObject.h"
#include "usdKatana/readBlindData.h"
#include "usdKatana/readMaterial.h"
#include "usdKatana/utils.h"

#include "pxrUsdInShipped/declareCoreOps.h"


PXR_NAMESPACE_USING_DIRECTIVE

// Small attribute cache:
// The problem we're trying to solve is that Katana reads these groups typically
// twice, because of it's access pattern on typicall assets.
// After the second cook, it realizes it shouldn't evict this group.
// This is pretty expensive however, as all the looks are cooked at once. We 
// want to cache the result. Since there are multiple threads cooking 
// potentially different materialGroups, we want about one slot per thread.
// We start with a reasonable constant.
TF_DEFINE_ENV_SETTING(
    USD_KATANA_CACHE_MATERIALGROUPS, true,
    "Toggle a small cache for repeated access of the same materialGroups.");
#define MAX_CACHED_MATERIALGROUPS 20

namespace
{
    class MaterialGroupCache {
    public: 
        void add(const FnKat::Hash& key, const FnKat::GroupAttribute& attr) {

            boost::upgrade_lock<boost::upgrade_mutex> lock(
                cachedMaterialGroupsMutex);

            _cache.push_front(std::pair<FnKat::Hash, FnKat::GroupAttribute>(
                key, attr));
            if (_cache.size() > MAX_CACHED_MATERIALGROUPS) {
                _cache.pop_back();
            }
        }

        FnKat::GroupAttribute get(const FnKat::Hash& key) {
            boost::upgrade_lock<boost::upgrade_mutex> lock(
                cachedMaterialGroupsMutex);
            for (auto it = _cache.begin(); it!=_cache.end(); ++it) {
                auto p = *it;
                if (p.first == key) {
                    auto retval = p.second;
                    _cache.erase(it);
                    _cache.push_front(p);

                    return retval;
                }
            }

            return FnKat::GroupAttribute();
        }

        void clear() {
            boost::upgrade_lock<boost::upgrade_mutex> lock(
                cachedMaterialGroupsMutex);
            _cache.clear();
        }

    private:
        boost::upgrade_mutex cachedMaterialGroupsMutex;
        std::list<std::pair<FnKat::Hash, FnKat::GroupAttribute> > _cache;
    };
    
    MaterialGroupCache g_materialGroupCache;
    
    void FlushMaterialGroupCache()
    {
        g_materialGroupCache.clear();
    }
}


FnKat::Hash _GetCacheKey(const PxrUsdKatanaUsdInPrivateData& privateData) {
    PxrUsdKatanaUsdInArgsRefPtr args = privateData.GetUsdInArgs();

    std::string location;
    UsdPrim prim = privateData.GetUsdPrim();
    if (prim.IsInstanceProxy()) {
        location = prim.GetPrimInMaster().GetPath().GetString();
    }
    else {
        location = prim.GetPath().GetString();
    }
    
    auto cacheKey = FnKat::GroupAttribute(
        "file", FnKat::StringAttribute(args->GetFileName()),
        "session", args->GetSessionAttr(),
        "time", FnKat::DoubleAttribute(args->GetCurrentTime()),
        "location", FnKat::StringAttribute(location),
        false);
        
    return cacheKey.getHash();
}



PXRUSDKATANA_USDIN_PLUGIN_DEFINE_WITH_FLUSH(
    PxrUsdInCore_LooksGroupOp, privateData, opArgs, interface, FlushMaterialGroupCache)
{
    // leave for debugging purposes
    // TfStopwatch timer_loadUsdMaterialGroup;
    // timer_loadUsdMaterialGroup.Start();

    //
    // Construct the group attribute argument for the StaticSceneCreate
    // op which will construct the materials scenegraph branch.
    //
    FnKat::Hash cacheKey;
    FnKat::GroupAttribute sscArgs;

    if (TfGetEnvSetting(USD_KATANA_CACHE_MATERIALGROUPS)) {
        cacheKey = _GetCacheKey(privateData);
        sscArgs = g_materialGroupCache.get(cacheKey);
    }
    
    if (!sscArgs.isValid())
    {
        UsdPrim prim = privateData.GetUsdPrim();
        FnKat::GroupBuilder gb;
        const std::string& rootLocation = interface.getRootLocationPath();

        for (UsdPrim child : prim.GetChildren()) {
            UsdShadeMaterial materialSchema(child);
            if (!materialSchema) {
                continue;
            }

            // do not flatten child material (specialize arcs) 
            // if we have any.
            bool flatten = !materialSchema.HasBaseMaterial();

            std::string location = 
                PxrUsdKatanaUtils::ConvertUsdMaterialPathToKatLocation(
                    child.GetPath(), privateData);

            PxrUsdKatanaAttrMap attrs;
            PxrUsdKatanaReadMaterial(materialSchema, flatten, 
                privateData, attrs, rootLocation);

            // Read blind data.
            PxrUsdKatanaReadBlindData(
                UsdKatanaBlindDataObject(materialSchema), attrs);

            // location is "/root/world/geo/Model/Wood/Walnut/Aged"
            // where rootLocation is "/root/world/geo/Model/"
            // want to get, "c.Wood.c.Walnut.c.Aged"

            std::string childPath = "c.";
            childPath += TfStringReplace(
                    location.substr(rootLocation.size()+1), "/", ".c.");
            childPath += ".a";

            gb.set(childPath, attrs.build());
        }

        sscArgs = gb.build();
        if (TfGetEnvSetting(USD_KATANA_CACHE_MATERIALGROUPS)) {
            g_materialGroupCache.add(cacheKey, sscArgs);
        }
    }

    // leave for debugging purposes
    // timer_loadUsdMaterialGroup.Stop();
    // int64_t materialGroupTime = timer_loadUsdMaterialGroup.GetMicroseconds();
    // std::cerr << "whole material group time: " << 
    //     interface.getOutputLocationPath()
    //     << " " << materialGroupTime << "\n";

    interface.execOp("StaticSceneCreate", sscArgs);

    interface.setAttr("type", FnKat::StringAttribute("materialgroup"));

    // This is an optimization to reduce the RIB size. Since material
    // assignments will resolve into actual material attributes at the
    // geometry locations, there is no need for the Looks scope to be emitted.
    //
    interface.setAttr("pruneRenderTraversal", FnKat::IntAttribute(1));
}
