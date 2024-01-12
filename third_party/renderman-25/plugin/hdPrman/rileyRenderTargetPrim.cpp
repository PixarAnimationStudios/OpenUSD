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

#include "hdPrman/rileyRenderTargetPrim.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "hdPrman/rileyPrimUtil.h"
#include "hdPrman/rileyRenderOutputPrim.h"
#include "hdPrman/rileyRenderTargetSchema.h"
#include "hdPrman/utils.h"

#include "pxr/imaging/hd/sceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

RtUString
_GetString(HdTokenDataSourceHandle const &ds)
{
    if (!ds) {
        return RtUString();
    }
    return RtUString(ds->GetTypedValue(0.0f).GetText());
}

riley::Extent
_GetExtent(HdVec3iDataSourceHandle const &ds)
{
    if (!ds) {
        return { 1, 1, 0 };
    }
    
    const GfVec3i v = ds->GetTypedValue(0.0f);
    return { static_cast<uint32_t>(std::max(v[0], 0)),
             static_cast<uint32_t>(std::max(v[1], 0)),
             static_cast<uint32_t>(std::max(v[2], 0)) };
}

float
_GetFloat(HdFloatDataSourceHandle const &ds)
{
    if (!ds) {
        return 1.0f;
    }
    return ds->GetTypedValue(0.0f);
}

}

HdPrman_RileyRenderTargetPrim::HdPrman_RileyRenderTargetPrim(
    HdContainerDataSourceHandle const &primSource,
    const HdsiPrimManagingSceneIndexObserver * const observer,
    HdPrman_RenderParam * const renderParam)
  : HdPrman_RileyPrimBase(renderParam)
{
    HdPrmanRileyRenderTargetSchema schema =
        HdPrmanRileyRenderTargetSchema::GetFromParent(primSource);

    HdPrman_RileyPrimArray<HdPrman_RileyRenderOutputPrim>
        renderOutputs(observer, schema.GetRenderOutputs());

    _rileyId = _AcquireRiley()->CreateRenderTarget(
        riley::UserId(),
        renderOutputs.rileyObject,
        _GetExtent(schema.GetExtent()),
        _GetString(schema.GetFilterMode()),
        _GetFloat(schema.GetPixelVariance()),
        HdPrman_Utils::ParamsFromDataSource(
            schema.GetParams()));

    _renderOutputPrims = std::move(renderOutputs.prims);
}

void
HdPrman_RileyRenderTargetPrim::_Dirty(
    const HdSceneIndexObserver::DirtiedPrimEntry &entry,
    const HdsiPrimManagingSceneIndexObserver * const observer)
{
    HdPrmanRileyRenderTargetSchema schema =
        HdPrmanRileyRenderTargetSchema::GetFromParent(
            observer->GetSceneIndex()->GetPrim(entry.primPath).dataSource);

    std::optional<HdPrman_RileyPrimArray<HdPrman_RileyRenderOutputPrim>>
        renderOutputs;
    if (entry.dirtyLocators.Intersects(
            HdPrmanRileyRenderTargetSchema::GetRenderOutputsLocator())) {
        renderOutputs =
            HdPrman_RileyPrimArray<HdPrman_RileyRenderOutputPrim>(
                observer, schema.GetRenderOutputs());
    }

    std::optional<riley::Extent> extent;
    if (entry.dirtyLocators.Intersects(
            HdPrmanRileyRenderTargetSchema::GetExtentLocator())) {
        extent = _GetExtent(schema.GetExtent());
    }

    std::optional<RtUString> filterMode;
    if (entry.dirtyLocators.Intersects(
            HdPrmanRileyRenderTargetSchema::GetFilterModeLocator())) {
        filterMode = _GetString(schema.GetFilterMode());
    }

    std::optional<float> pixelVariance;
    if (entry.dirtyLocators.Intersects(
            HdPrmanRileyRenderTargetSchema::GetPixelVarianceLocator())) {
        pixelVariance = _GetFloat(schema.GetPixelVariance());
    }

    std::optional<RtParamList> params;
    if (entry.dirtyLocators.Intersects(
            HdPrmanRileyRenderTargetSchema::GetParamsLocator())) {
        params = HdPrman_Utils::ParamsFromDataSource(schema.GetParams());
    }

    _AcquireRiley()->ModifyRenderTarget(
        _rileyId,
        HdPrman_GetPtrRileyObject(renderOutputs),
        HdPrman_GetPtr(extent),
        HdPrman_GetPtr(filterMode),
        HdPrman_GetPtr(pixelVariance),
        HdPrman_GetPtr(params));

    // Now that the render target is using the new render outputs, we
    // can release the handles to the old render outputs.
    if (renderOutputs) {
        _renderOutputPrims = std::move(renderOutputs->prims);
    }
}

HdPrman_RileyRenderTargetPrim::~HdPrman_RileyRenderTargetPrim()
{
    _AcquireRiley()->DeleteRenderTarget(_rileyId);
    
    // _renderOutputPrims get dropped after the render target was
    // deleted.
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
