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
#ifndef USD_KATANA_CACHE_H
#define USD_KATANA_CACHE_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usdImaging/usdImagingGL/engine.h"
#include "pxr/base/tf/singleton.h"

#include <boost/shared_ptr.hpp>

#include <map>
#include <string>

#include <FnAttribute/FnAttribute.h>

PXR_NAMESPACE_OPEN_SCOPE


// Forward declare pointers.
SDF_DECLARE_HANDLES(SdfLayer);
typedef TfRefPtr<class UsdStage> UsdStageRefPtr;
typedef boost::shared_ptr<class UsdImagingGLEngine> UsdImagingGLEngineSharedPtr;
class SdfPath;
class UsdPrim;

/*
 * Custom cache singleton class for katana. Hold the usd stage and renderer.
 * The stage returned by this cache helper is meant to be read only. The
 * session layer will be locked for editing.
 */
class UsdKatanaCache : public TfSingleton<UsdKatanaCache> 
{
    friend class TfSingleton<UsdKatanaCache>;

    UsdKatanaCache();

    /// Construct a session layer from the groupAttr encoding of variants
    /// and deactivations -- or return a previously created one
    SdfLayerRefPtr& _FindOrCreateSessionLayer(
            FnAttribute::GroupAttribute sessionAttr, 
            const std::string& rootLocation);


    /// Mute layers by name
    static void _SetMutedLayers(
        const UsdStageRefPtr &stage, const std::string &layerRegex);

    typedef std::map<std::string, UsdImagingGLEngineSharedPtr> _RendererCache;

    std::string _ComputeCacheKey(FnAttribute::GroupAttribute sessionAttr, 
        const std::string& rootLocation);

    std::map<std::string, SdfLayerRefPtr> _sessionKeyCache;
    _RendererCache _rendererCache;

public:

    static UsdKatanaCache& GetInstance() {
        return TfSingleton<UsdKatanaCache>::GetInstance();
    }

    /// Clear all caches
    void Flush();

    
    /// Get (or create) a cached usd stage with a sessionLayer containing
    /// variant selections and activations (so far)
    UsdStageRefPtr GetStage(std::string const& fileName, 
                            FnAttribute::GroupAttribute sessionAttr,
                            const std::string & sessionRootLocation,
                            std::string const& ignoreLayerRegex,
                            bool forcePopulate);
    
    // Equivalent to GetStage above but without caching
    UsdStageRefPtr GetUncachedStage(std::string const& fileName, 
                            FnAttribute::GroupAttribute sessionAttr,
                            const std::string & sessionRootLocation,
                            std::string const& ignoreLayerRegex,
                            bool forcePopulate);

    /// Flushes an individual stage if present in the cache
    void FlushStage(const UsdStageRefPtr & stage);




    /// Get (or create) a cached renderer for a given prim path.
    UsdImagingGLEngineSharedPtr const& GetRenderer(UsdStageRefPtr const& stage,
                                             UsdPrim const& root,
                                             std::string const& sessionKey);

    /// \brief Find a cached session layer if it exists.  Does NOT create.
    SdfLayerRefPtr FindSessionLayer(
        FnAttribute::GroupAttribute sessionAttr, 
        const std::string& rootLocation);


    SdfLayerRefPtr FindSessionLayer(
        const std::string& cacheKey) ;
    
    
    SdfLayerRefPtr FindOrCreateSessionLayer(
        const std::string& sessionAttrXML,
        const std::string& rootLocation);

    

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_KATANA_CACHE_H
