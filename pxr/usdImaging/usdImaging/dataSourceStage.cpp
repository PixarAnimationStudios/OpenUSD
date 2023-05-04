//
// Copyright 2022 Pixar
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
#include "pxr/usdImaging/usdImaging/dataSourceStage.h"

#include "pxr/imaging/hdar/systemSchema.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"
#include "pxr/imaging/hd/systemSchema.h"

#include "pxr/usd/ar/resolverContext.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdRender/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TfTokenVector
UsdImagingDataSourceStage::GetNames()
{
    return {
        HdSystemSchema::GetSchemaToken(),
        HdSceneGlobalsSchema::GetSchemaToken()
    };
}

HdDataSourceBaseHandle
UsdImagingDataSourceStage::Get(const TfToken& name)
{
    if (name == HdSystemSchema::GetSchemaToken()) {
        return HdRetainedContainerDataSource::New(
            HdarSystemSchemaTokens->assetResolution,
            HdarSystemSchema::Builder()
                .SetResolverContext(
                    HdRetainedTypedSampledDataSource<ArResolverContext>::New(
                        _stage->GetPathResolverContext()))
                .Build());
    }
    if (name == HdSceneGlobalsSchema::GetSchemaToken()) {
        // Update the sceneGlobals locator if we have stage metadata for the
        // the render settings prim to use for rendering.
        std::string pathStr;
        if (_stage->HasAuthoredMetadata(
                UsdRenderTokens->renderSettingsPrimPath)) {
            _stage->GetMetadata(
                UsdRenderTokens->renderSettingsPrimPath, &pathStr);
        }

        return HdSceneGlobalsSchema::Builder()
                .SetActiveRenderSettingsPrim(
                    pathStr.empty()
                        ? nullptr
                        : HdRetainedTypedSampledDataSource<SdfPath>::New(
                            SdfPath(pathStr)))
                .Build();
    }
    return nullptr;
}

UsdImagingDataSourceStage::UsdImagingDataSourceStage(UsdStageRefPtr stage)
    : _stage(std::move(stage))
{
}

PXR_NAMESPACE_CLOSE_SCOPE
