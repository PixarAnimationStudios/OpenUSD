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
#include "usdKatana/cache.h"
#include "usdKatana/locks.h"
#include "usdKatana/debugCodes.h"

#include "pxr/usd/ar/resolver.h"

#include "pxr/usdImaging/usdImagingGL/gl.h"

#include "pxr/usd/usdUtils/stageCache.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tracelite/trace.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stageCacheContext.h"

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/tf/instantiateSingleton.h"

#include <set>
#include <regex.h>

#include <pystring/pystring.h>

TF_INSTANTIATE_SINGLETON(UsdKatanaCache);

SdfLayerRefPtr&
UsdKatanaCache::_FindOrCreateSessionLayer(
        FnAttribute::GroupAttribute sessionAttr, std::string rootLocation)
{
    // Grab a reader lock for reading the _sessionKeyCache
    boost::upgrade_lock<boost::upgrade_mutex>
                readerLock(UsdKatanaGetSessionCacheLock());
    
    
    std::string cacheKey = FnAttribute::GroupAttribute("s", sessionAttr,
            "r", FnAttribute::StringAttribute(rootLocation),
                    true).getHash().str();
    
    // Open the usd stage
    SdfLayerRefPtr sessionLayer;
    
    if (_sessionKeyCache.find(cacheKey) == _sessionKeyCache.end())
    {
        boost::upgrade_to_unique_lock<boost::upgrade_mutex>
                    writerLock(readerLock);
        sessionLayer = SdfLayer::CreateAnonymous();
        _sessionKeyCache[cacheKey] = sessionLayer;
        
        
        std::string rootLocationPlusSlash = rootLocation + "/";
        
        
        FnAttribute::GroupAttribute variantsAttr =
                sessionAttr.getChildByName("variants");
        for (int64_t i = 0, e = variantsAttr.getNumberOfChildren(); i != e;
                ++i)
        {
            std::string entryName = FnAttribute::DelimiterDecode(
                    variantsAttr.getChildName(i));
            
            FnAttribute::GroupAttribute entryVariantSets =
                    variantsAttr.getChildByIndex(i);
            
            if (entryVariantSets.getNumberOfChildren() == 0)
            {
                continue;
            }
            
            if (!pystring::startswith(entryName, rootLocationPlusSlash))
            {
                continue;
            }
            
            
            std::string primPath = pystring::slice(entryName,
                    rootLocation.size());
            
            for (int64_t i = 0, e = entryVariantSets.getNumberOfChildren();
                    i != e; ++i)
            {
                std::string variantSetName = entryVariantSets.getChildName(i);
                
                FnAttribute::StringAttribute variantValueAttr =
                        entryVariantSets.getChildByIndex(i);
                if (!variantValueAttr.isValid())
                {
                    continue;
                }
                
                std::string variantSetSelection =
                        variantValueAttr.getValue("", false);
                
                SdfPath varSelPath(primPath);
                
                
                SdfPrimSpecHandle spec = SdfCreatePrimInLayer(
                        sessionLayer, varSelPath.GetPrimPath());
                if (spec)
                {
                    std::pair<std::string, std::string> sel = 
                            varSelPath.GetVariantSelection();
                    spec->SetVariantSelection(variantSetName,
                            variantSetSelection);
                }
            }
        }
        
        
        FnAttribute::GroupAttribute activationsAttr =
                sessionAttr.getChildByName("activations");
        for (int64_t i = 0, e = activationsAttr.getNumberOfChildren(); i != e;
                ++i)
        {
            std::string entryName = FnAttribute::DelimiterDecode(
                    activationsAttr.getChildName(i));
            
            FnAttribute::IntAttribute stateAttr =
                    activationsAttr.getChildByIndex(i);
            
            if (stateAttr.getNumberOfValues() != 1)
            {
                continue;
            }
            
            if (!pystring::startswith(entryName, rootLocationPlusSlash))
            {
                continue;
            }
            
            std::string primPath = pystring::slice(entryName,
                    rootLocation.size());
            
            SdfPath varSelPath(primPath);
            
            SdfPrimSpecHandle spec = SdfCreatePrimInLayer(
                        sessionLayer, varSelPath.GetPrimPath());
            spec->SetActive(stateAttr.getValue());
        }
        
    }
    
    return _sessionKeyCache[cacheKey];
    
}


/* static */
void
UsdKatanaCache::_SetMutedLayers(
    const UsdStageRefPtr &stage, const std::string &layerRegex) 
{
    // Trace this function to track its performance
    TRACE_FUNCTION();

    // Unmute layers that are currently muted, but not requested to be muted
    SdfLayerHandleVector stageLayers = stage->GetUsedLayers();

    bool regexIsEmpty = layerRegex == "" || layerRegex == "^$";
    
    // use a better regex library?
    regex_t regex;
    regcomp(&regex, layerRegex.c_str(), REG_EXTENDED);
    regmatch_t* rmatch = 0;

    TF_FOR_ALL(stageLayer, stageLayers)
    {
        SdfLayerHandle layer = *stageLayer;
        if (!layer) {
            continue;
        }
        std::string layerPath = layer->GetRepositoryPath();
        const std::string layerIdentifier = layer->GetIdentifier();

        bool match = false;
        
        if (!regexIsEmpty)
        {
            if (layer && !regexec(
                &regex, 
                layerIdentifier.c_str(), 
                0, rmatch, 0))
            {
                match = true;
            }
        }
        
        if (!match && stage->IsLayerMuted(layerIdentifier)) {
            TF_DEBUG(USDKATANA_CACHE_RENDERER).Msg("{USD RENDER CACHE} "
                                "Unmuting Layer: '%s'\n",
                                layerIdentifier.c_str());
            stage->UnmuteLayer(layerIdentifier);
        }

        if (match && !stage->IsLayerMuted(layerIdentifier)) {
            TF_DEBUG(USDKATANA_CACHE_RENDERER).Msg("{USD RENDER CACHE} "
                    "Muting Layer: '%s'\n",
                    layerIdentifier.c_str());
            stage->MuteLayer(layerIdentifier);
        }
    }
    regfree(&regex);
}

UsdKatanaCache::UsdKatanaCache() 
{
}

void
UsdKatanaCache::Flush()
{
    // Flushing is writing, grab writer locks for the caches.
    boost::unique_lock<boost::upgrade_mutex>
                rendererWriterLock(UsdKatanaGetRendererCacheLock());
    boost::unique_lock<boost::upgrade_mutex>
                sessionWriterLock(UsdKatanaGetSessionCacheLock());

    UsdUtilsStageCache::Get().Clear();
    _sessionKeyCache.clear();
    _rendererCache.clear();
}


static std::string
_ResolvePath(const std::string& path)
{
    return ArGetResolver().Resolve(path);
}

UsdStageRefPtr UsdKatanaCache::GetStage(
        std::string const& fileName, 
        FnAttribute::GroupAttribute sessionAttr,
        const std::string & sessionRootLocation,
        std::string const& ignoreLayerRegex,
        bool forcePopulate)
{
    bool givenAbsPath = TfStringStartsWith(fileName, "/");
    const std::string contextPath = givenAbsPath ? 
                                    TfGetPathName(fileName) : ArchGetCwd();

    std::string path = !givenAbsPath ? _ResolvePath(fileName) : fileName;

    TF_DEBUG(USDKATANA_CACHE_STAGE).Msg(
            "{USD STAGE CACHE} Creating and caching UsdStage for "
            "given filePath @%s@, which resolves to @%s@\n", 
            fileName.c_str(), path.c_str());

    if (SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(path)) {
        SdfLayerRefPtr& sessionLayer =
                _FindOrCreateSessionLayer(sessionAttr, sessionRootLocation);

        UsdStageCache& stageCache = UsdUtilsStageCache::Get();
        UsdStageCacheContext cacheCtx(stageCache);
        const UsdStage::InitialLoadSet load = 
            (forcePopulate ? UsdStage::LoadAll : UsdStage::LoadNone);
        UsdStageRefPtr const& stage = UsdStage::Open(rootLayer, sessionLayer, 
                ArGetResolver().GetCurrentContext(),
                load);

        // TF_DEBUG(USDKATANA_CACHE_STAGE).Msg("{USD STAGE CACHE} Fetched stage "
        //                             "(%s, %s, forcePopulate=%s) "
        //                             "with UsdStage address '%lx'\n",
        //                             fileName.c_str(),
        //                             variantSelections.c_str(),
        //                             forcePopulate?"true":"false",
        //                             (size_t)stage.operator->());

        // Mute layers according to a regex.
        _SetMutedLayers(stage, ignoreLayerRegex);

        return stage;

    }
    
    static UsdStageRefPtr NULL_STAGE;
    return NULL_STAGE;
}


UsdStageRefPtr
UsdKatanaCache::GetUncachedStage(std::string const& fileName, 
                            FnAttribute::GroupAttribute sessionAttr,
                            const std::string & sessionRootLocation,
                            std::string const& ignoreLayerRegex,
                            bool forcePopulate)
{
    bool givenAbsPath = TfStringStartsWith(fileName, "/");
    const std::string contextPath = givenAbsPath ? 
                                    TfGetPathName(fileName) : ArchGetCwd();

    std::string path = !givenAbsPath ? _ResolvePath(fileName) : fileName;

    TF_DEBUG(USDKATANA_CACHE_STAGE).Msg(
            "{USD STAGE CACHE} Creating UsdStage for "
            "given filePath @%s@, which resolves to @%s@\n", 
            fileName.c_str(), path.c_str());

    if (SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(path)) {
        SdfLayerRefPtr& sessionLayer =
                _FindOrCreateSessionLayer(sessionAttr, sessionRootLocation);
        
        // No cache!
        //UsdStageCache stageCache;
        //UsdStageCacheContext cacheCtx(stageCache);
        
        const UsdStage::InitialLoadSet load = 
            (forcePopulate ? UsdStage::LoadAll : UsdStage::LoadNone);
        UsdStageRefPtr const stage = UsdStage::Open(rootLayer, sessionLayer, 
                ArGetResolver().GetCurrentContext(),
                load);

        // TF_DEBUG(USDKATANA_CACHE_STAGE).Msg("{USD STAGE CACHE} Fetched uncached stage "
        //                             "(%s, %s, forcePopulate=%s) "
        //                             "with UsdStage address '%lx'\n",
        //                             fileName.c_str(),
        //                             variantSelections.c_str(),
        //                             forcePopulate?"true":"false",
        //                             (size_t)stage.operator->());

        // Mute layers according to a regex.
        _SetMutedLayers(stage, ignoreLayerRegex);

        return stage;

    }
    
    return UsdStageRefPtr();
    
    
}





UsdImagingGLSharedPtr const& 
UsdKatanaCache::GetRenderer(UsdStageRefPtr const& stage,
                            UsdPrim const& root,
                            std::string const& sessionKey)
{
    // Grab a reader lock for reading the _rendererCache
    boost::upgrade_lock<boost::upgrade_mutex>
                readerLock(UsdKatanaGetRendererCacheLock());

    // First look for a parent renderer object first.
    std::string const prefix = stage->GetRootLayer()->GetIdentifier() 
                             + "::" + sessionKey + "::";

    std::string key = prefix + root.GetPath().GetString();
    {
        _RendererCache::const_iterator it = _rendererCache.find(key);
        if (it != _rendererCache.end())
            return it->second;
    }
    
    // May 2015: In the future, we might want to look for a renderer
    // cached at the parent prim that we can reuse to render this prim. This
    // would save some time by not creating a new renderer for every prim.
    //
    // UsdImaging does not currently support recycling renderers in this way,
    // so we can't do it yet. It's a non-issue at the moment because we only
    // render proxies at components, not at every prim.
    //
    // For future reference, here is some example code for re-using the renderer
    // from the parent prim:
    //
    // Look for a renderer cached at the parent.
    // std::string parentKey = prefix + root.GetParent().GetPath().GetString();
    // _RendererCache::const_iterator it = _rendererCache.find(parentKey);
    // if (it != _rendererCache.end()) {
    //     TF_DEBUG(USDKATANA_CACHE_RENDERER).Msg("{USD RENDER CACHE} "
    //                         "Inherited renderer '%s' from parent '%s'\n",
    //                                 key.c_str(),
    //                                 parentKey.c_str());
    //     // Protect the _rendererCache for write
    //     boost::upgrade_to_unique_lock<boost::upgrade_mutex>
    //                 writerLock(readerLock);

    //     // Chain the child to the parent;
    //     _rendererCache.insert(std::make_pair(key, it->second));
    //     return it->second;
    // } 

    TF_DEBUG(USDKATANA_CACHE_RENDERER).Msg("{USD RENDER CACHE} "
                                    "New renderer created with key '%s'\n",
                                    key.c_str());

    // Protect the _rendererCache for write
    boost::upgrade_to_unique_lock<boost::upgrade_mutex> writerLock(readerLock);

    // Make a new renderer at the requested path
    SdfPathVector excludedPaths;
    std::pair<_RendererCache::iterator,bool> res  = 
        _rendererCache.insert(std::make_pair(key, 
                   UsdImagingGLSharedPtr(new UsdImagingGL(root.GetPath(), 
                                                          excludedPaths))));
    return res.first->second;
}

