//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
               .SetStartTimeCode(
                   HdRetainedTypedSampledDataSource<double>::New(
                       _stage->GetStartTimeCode()))
               .SetEndTimeCode(
                   HdRetainedTypedSampledDataSource<double>::New(
                       _stage->GetEndTimeCode()))
               .Build();
    }
    return nullptr;
}

UsdImagingDataSourceStage::UsdImagingDataSourceStage(UsdStageRefPtr stage)
    : _stage(std::move(stage))
{
}

PXR_NAMESPACE_CLOSE_SCOPE
