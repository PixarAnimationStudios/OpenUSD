//
// Copyright 2023 Pixar
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
#include "pxr/usdImaging/usdImaging/sceneIndices.h"

#include "pxr/usdImaging/usdImaging/drawModeSceneIndex.h"
#include "pxr/usdImaging/usdImaging/extentResolvingSceneIndex.h"
#include "pxr/usdImaging/usdImaging/flattenedDataSourceProviders.h"
#include "pxr/usdImaging/usdImaging/niPrototypePropagatingSceneIndex.h"
#include "pxr/usdImaging/usdImaging/piPrototypePropagatingSceneIndex.h"
#include "pxr/usdImaging/usdImaging/renderSettingsFlatteningSceneIndex.h"
#include "pxr/usdImaging/usdImaging/selectionSceneIndex.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"
#include "pxr/usdImaging/usdImaging/unloadedDrawModeSceneIndex.h"

#include "pxr/imaging/hd/flatteningSceneIndex.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

static
HdContainerDataSourceHandle
_AdditionalStageSceneIndexInputArgs(
    const bool displayUnloadedPrimsWithBounds)
{
    if (!displayUnloadedPrimsWithBounds) {
        return nullptr;
    }
    static HdContainerDataSourceHandle const ds =
        HdRetainedContainerDataSource::New(
            UsdImagingStageSceneIndexTokens->includeUnloadedPrims,
            HdRetainedTypedSampledDataSource<bool>::New(true));
    return ds;
}

// Use extentsHint (of models) for purpose geometry
static
HdContainerDataSourceHandle
_ExtentResolvingSceneIndexInputArgs()
{
    HdDataSourceBaseHandle const purposeDataSources[] = {
        HdRetainedTypedSampledDataSource<TfToken>::New(
            HdTokens->geometry) };

    return
        HdRetainedContainerDataSource::New(
            UsdImagingExtentResolvingSceneIndexTokens->purposes,
            HdRetainedSmallVectorDataSource::New(
                TfArraySize(purposeDataSources),
                purposeDataSources));
}

UsdImagingSceneIndices
UsdImagingInstantiateSceneIndices(
    const UsdImagingSceneIndicesCreateInfo &createInfo)
{
    UsdImagingSceneIndices result;

    HdSceneIndexBaseRefPtr sceneIndex;
    
    sceneIndex = result.stageSceneIndex =
        UsdImagingStageSceneIndex::New(
            HdOverlayContainerDataSource::OverlayedContainerDataSources(
                _AdditionalStageSceneIndexInputArgs(
                    createInfo.displayUnloadedPrimsWithBounds),
                createInfo.stageSceneIndexInputArgs));

    result.stageSceneIndex->SetStage(createInfo.stage);
    
    if (createInfo.overridesSceneIndexCallback) {
        sceneIndex =
            createInfo.overridesSceneIndexCallback(sceneIndex);
    }

    if (createInfo.displayUnloadedPrimsWithBounds) {
        sceneIndex =
            UsdImagingUnloadedDrawModeSceneIndex::New(sceneIndex);
    }
    
    sceneIndex =
        UsdImagingExtentResolvingSceneIndex::New(
            sceneIndex, _ExtentResolvingSceneIndexInputArgs());

    sceneIndex =
        UsdImagingPiPrototypePropagatingSceneIndex::New(sceneIndex);

    sceneIndex =
        UsdImagingNiPrototypePropagatingSceneIndex::New(sceneIndex);

    sceneIndex = result.selectionSceneIndex =
        UsdImagingSelectionSceneIndex::New(sceneIndex);
    
    sceneIndex =
        UsdImagingRenderSettingsFlatteningSceneIndex::New(sceneIndex);

    sceneIndex =
        HdFlatteningSceneIndex::New(
            sceneIndex, UsdImagingFlattenedDataSourceProviders());

    if (createInfo.addDrawModeSceneIndex) {
        sceneIndex =
            UsdImagingDrawModeSceneIndex::New(sceneIndex,
                                              /* inputArgs = */ nullptr);
    }

    result.finalSceneIndex = sceneIndex;

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
