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

#include "usdMaya/stageCache.h"

#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/usd/stageCache.h"
#include "pxr/usd/usdGeom/tokens.h"

#include <maya/MFileIO.h>
#include <maya/MSceneMessage.h>

#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>


PXR_NAMESPACE_OPEN_SCOPE


std::map<std::string, SdfLayerRefPtr> _sharedSessionLayers;
std::mutex _sharedSessionLayersMutex;

static
void
_OnMayaNewOrOpenSceneCallback(void* clientData)
{
    if (MFileIO::isImportingFile() || MFileIO::isReferencingFile()) {
        return;
    }

    TF_STATUS("Clearing USD Stage Cache");
    UsdMayaStageCache::Clear();

    std::lock_guard<std::mutex> lock(_sharedSessionLayersMutex);
    _sharedSessionLayers.clear();
}

/* static */
UsdStageCache&
UsdMayaStageCache::Get(const bool forcePopulate)
{
    static UsdStageCache theCacheForcePopulate;
    static UsdStageCache theCache;

    // We clear the USD stage cache when changing scenes (either by switching
    // to a new empty scene or by opening a different scene). We do not listen
    // for kSceneUpdate messages since those are also emitted after a SaveAs
    // operation, in which case we do not want to clear the stage cache.
    // Note also that we listen for kBeforeFileRead messages because those fire
    // at the right time (after any existing scene has been closed but before
    // the new scene has been opened). However, they are also emitted when a
    // file is imported or referenced, so we check for that and do *not* clear
    // the stage cache in those cases. The Maya/Hydra batch renderer follows a
    // similar listener/reset pattern.
    static MCallbackId afterNewCallbackId = 0;
    if (afterNewCallbackId == 0) {
        afterNewCallbackId =
            MSceneMessage::addCallback(MSceneMessage::kAfterNew,
                                       _OnMayaNewOrOpenSceneCallback);
    }

    static MCallbackId beforeFileReadCallbackId = 0;
    if (beforeFileReadCallbackId == 0) {
        beforeFileReadCallbackId =
            MSceneMessage::addCallback(MSceneMessage::kBeforeFileRead,
                                       _OnMayaNewOrOpenSceneCallback);
    }

    return forcePopulate ? theCacheForcePopulate : theCache;
}

/* static */
void
UsdMayaStageCache::Clear()
{
    Get(true).Clear();
    Get(false).Clear();
}

/* static */
size_t
UsdMayaStageCache::EraseAllStagesWithRootLayerPath(const std::string& layerPath)
{
    size_t erasedStages = 0u;

    const SdfLayerHandle rootLayer = SdfLayer::Find(layerPath);
    if (!rootLayer) {
        return erasedStages;
    }

    erasedStages += Get(true).EraseAll(rootLayer);
    erasedStages += Get(false).EraseAll(rootLayer);

    return erasedStages;
}

SdfLayerRefPtr
UsdMayaStageCache::GetSharedSessionLayer(
    const SdfPath& rootPath,
    const std::map<std::string, std::string> variantSelections,
    const TfToken& drawMode)
{
    // Example key: "/Root/Path:modelingVariant=round|shadingVariant=red|:cards"
    std::ostringstream key;
    key << rootPath;
    key << ":";
    for (const auto& pair : variantSelections) {
        key << pair.first << "=" << pair.second << "|";
    }
    key << ":";
    key << drawMode;

    std::string keyString = key.str();
    std::lock_guard<std::mutex> lock(_sharedSessionLayersMutex);
    auto iter = _sharedSessionLayers.find(keyString);
    if (iter == _sharedSessionLayers.end()) {
        SdfLayerRefPtr newLayer = SdfLayer::CreateAnonymous();

        SdfPrimSpecHandle over = SdfCreatePrimInLayer(newLayer, rootPath);
        for (const auto& pair : variantSelections) {
            const std::string& variantSet = pair.first;
            const std::string& variantSelection = pair.second;
            over->GetVariantSelections()[variantSet] = variantSelection;
        }

        if (!drawMode.IsEmpty()) {
            SdfAttributeSpecHandle drawModeAttr = SdfAttributeSpec::New(
                    over,
                    UsdGeomTokens->modelDrawMode,
                    SdfValueTypeNames->Token,
                    SdfVariabilityUniform);
            drawModeAttr->SetDefaultValue(VtValue(drawMode));
            SdfAttributeSpecHandle applyDrawModeAttr = SdfAttributeSpec::New(
                    over,
                    UsdGeomTokens->modelApplyDrawMode,
                    SdfValueTypeNames->Bool,
                    SdfVariabilityUniform);
            applyDrawModeAttr->SetDefaultValue(VtValue(true));
        }

        _sharedSessionLayers[keyString] = newLayer;
        return newLayer;
    }
    else {
        return iter->second;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
