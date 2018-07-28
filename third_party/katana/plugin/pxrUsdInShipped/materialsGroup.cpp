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

#include <FnGeolibServices/FnBuiltInOpArgsUtil.h>

#include <FnAPI/FnAPI.h>

PXR_NAMESPACE_USING_DIRECTIVE

// We previously processed and cached material groups in their entirety.
// Instead the material group just figures out the katana path and defers
// reading of the material data itself to the material locations.
// 
// The caching is also done per material now -- as implemented in material.cpp
// 
// However, the cache key is still generated from the material group and
// therefore the envvar (and terminology) for enabling it remains here.
// 
// We start with a reasonable constant.
TF_DEFINE_ENV_SETTING(
    USD_KATANA_CACHE_MATERIALGROUPS, true,
    "Toggle inclusion of a cache key representing this scope " \
    "(respected by PxrUsdInCore_LookOp)");


FnKat::Attribute _GetCacheKey(const PxrUsdKatanaUsdInPrivateData& privateData) {
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
        
    return cacheKey;
}



PXRUSDKATANA_USDIN_PLUGIN_DEFINE(
    PxrUsdInCore_LooksGroupOp, privateData, opArgs, interface)
{
    // leave for debugging purposes
    // TfStopwatch timer_loadUsdMaterialGroup;
    // timer_loadUsdMaterialGroup.Start();

    
    FnKat::Attribute cacheKeyAttr;
    if (TfGetEnvSetting(USD_KATANA_CACHE_MATERIALGROUPS))
    {
        // check for an explicit cache key from above
        cacheKeyAttr = opArgs.getChildByName("sharedLooksCacheKey");
        
        if (!cacheKeyAttr.isValid())
        {
            // check locally also
            UsdAttribute keyAttr = privateData.GetUsdPrim().GetAttribute(
                    TfToken("sharedLooksCacheKey"));
            if (keyAttr.IsValid())
            {
                std::string cacheKey;
                keyAttr.Get(&cacheKey);
                
                cacheKeyAttr = FnKat::StringAttribute(cacheKey);
            }
        }
        
        if (!cacheKeyAttr.isValid())
        {
            cacheKeyAttr = _GetCacheKey(privateData);
        }
    }
    
    
    struct usdPrimInfo
    {
        std::vector<std::string> usdPrimPathValues;
        std::vector<std::string> usdPrimNameValues;
    };
    
    std::map<std::string, usdPrimInfo> primInfoPerLocation;

    UsdPrim prim = privateData.GetUsdPrim();
    FnKat::GroupBuilder gb;
    const std::string& rootLocation = interface.getRootLocationPath();

    for (UsdPrim child : prim.GetChildren()) {
        
        std::string materialLocation = 
            PxrUsdKatanaUtils::ConvertUsdMaterialPathToKatLocation(
                child.GetPath(), privateData);
        
        
        std::string relativeMaterialLocation =
                materialLocation.substr(rootLocation.size()+1);

        auto lastSlash = relativeMaterialLocation.rfind('/');
        
        std::string parentPath;
        std::string baseName;
        
        
        if (lastSlash != std::string::npos)
        {
            parentPath = relativeMaterialLocation.substr(0, lastSlash);
            baseName = relativeMaterialLocation.substr(lastSlash + 1);
        }
        else
        {
            baseName = relativeMaterialLocation;
        }
        
        auto & locationEntry = primInfoPerLocation[parentPath];
        

        locationEntry.usdPrimPathValues.push_back(
                child.GetPath().GetText());
        locationEntry.usdPrimNameValues.push_back(baseName);
        

    }

    FnGeolibServices::StaticSceneCreateOpArgsBuilder sscb(false);
    
    for (const auto & I : primInfoPerLocation)
    {
        const auto & locationParent = I.first;
        const auto & entry = I.second;
        
        sscb.setAttrAtLocation(locationParent, "usdPrimPath",
                FnKat::StringAttribute(entry.usdPrimPathValues));
        sscb.setAttrAtLocation(locationParent, "usdPrimName",
                FnKat::StringAttribute(entry.usdPrimNameValues));
    }
    
    // TODO consider caching this? Observed performance doesn't appear to
    //      warrant caching it.
    
    auto args = sscb.build();
    
    
    interface.execOp("PxrUsdIn.BuildIntermediate",
                FnKat::GroupBuilder()
                        .update(interface.getOpArg())
                        .set("staticScene", args)
                        .set("looksGroupLocation",
                                FnAttribute::StringAttribute(
                                        interface.getOutputLocationPath()))
                        .set("forceFlattenLooks",
                                FnAttribute::IntAttribute(0))
                        .set("looksCacheKeyPrefixAttr",
                                cacheKeyAttr)
                        .build()

// katana 2.x doesn't allow private data to be overridden in execOp as it'll
// automatically use that of the calling op while katana 3.x does allow it
// to be overridden but requires it to be specified even if unchanged (as here).
#if KATANA_VERSION_MAJOR >= 3
                ,
                new PxrUsdKatanaUsdInPrivateData(
                            privateData.GetUsdInArgs()->GetRootPrim(),
                            privateData.GetUsdInArgs(), &privateData),
                PxrUsdKatanaUsdInPrivateData::Delete
#endif
                );


    // leave for debugging purposes
    // timer_loadUsdMaterialGroup.Stop();
    // int64_t materialGroupTime = timer_loadUsdMaterialGroup.GetMicroseconds();
    // std::cerr << "whole material group time: " << 
    //     interface.getOutputLocationPath()
    //     << " " << materialGroupTime << "\n";


    interface.setAttr("type", FnKat::StringAttribute("materialgroup"));

    // This is an optimization to reduce the RIB size. Since material
    // assignments will resolve into actual material attributes at the
    // geometry locations, there is no need for the Looks scope to be emitted.
    //
    interface.setAttr("pruneRenderTraversal", FnKat::IntAttribute(1));
}
