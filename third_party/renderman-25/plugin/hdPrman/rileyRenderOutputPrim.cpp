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

#include "hdPrman/rileyRenderOutputPrim.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "hdPrman/rileyRenderOutputSchema.h"
#include "hdPrman/utils.h"

#include "pxr/imaging/hd/sceneIndex.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

static
riley::RenderOutputType
_GetRenderOutputType(HdTokenDataSourceHandle const &ds)
{
    if (!ds) {
        return riley::RenderOutputType::k_Float;
    }
    const TfToken t = ds->GetTypedValue(0.0f);
    if (t == HdPrmanRileyRenderOutputSchemaTokens->typeFloat) {
        return riley::RenderOutputType::k_Float;
    }
    if (t == HdPrmanRileyRenderOutputSchemaTokens->typeInteger) {
        return riley::RenderOutputType::k_Integer;
    }
    if (t == HdPrmanRileyRenderOutputSchemaTokens->typeColor) {
        return riley::RenderOutputType::k_Color;
    }
    if (t == HdPrmanRileyRenderOutputSchemaTokens->typeVector) {
        return riley::RenderOutputType::k_Vector;
    }
    return riley::RenderOutputType::k_Float;
}

static
RtUString
_GetString(HdTokenDataSourceHandle const &ds)
{
    if (!ds) {
        return RtUString();
    }
    return RtUString(ds->GetTypedValue(0.0f).GetText());
}

static
riley::FilterSize
_GetFilterSize(HdVec2fDataSourceHandle const &ds)
{
    if (!ds) {
        return { 1.0f, 1.0f };
    }
    const GfVec2f v = ds->GetTypedValue(0.0f);
    return { v[0], v[1] };
}

static
float
_GetFloat(HdFloatDataSourceHandle const &ds)
{
    if (!ds) {
        return 1.0f;
    }
    return ds->GetTypedValue(0.0f);
}

template<typename T>
static
T * _GetPtr(std::optional<T> &v)
{
    if (v) {
        return &(*v);
    } else {
        return nullptr;
    }
}


static
riley::RenderOutputId
_CreateRenderOutput(
    riley::Riley * const riley,
    HdPrmanRileyRenderOutputSchema schema)
{
    return
        riley->CreateRenderOutput(
            riley::UserId(),
            _GetString(schema.GetName()),
            _GetRenderOutputType(schema.GetType()),
            _GetString(schema.GetSource()),
            _GetString(schema.GetAccumulationRule()),
            _GetString(schema.GetFilter()),
            _GetFilterSize(schema.GetFilterSize()),
            _GetFloat(schema.GetRelativePixelVariance()),
            HdPrman_Utils::ParamsFromDataSource(
                schema.GetParams()));
}

HdPrman_RileyRenderOutputPrim::HdPrman_RileyRenderOutputPrim(
    HdContainerDataSourceHandle const &primSource,
    const HdsiPrimManagingSceneIndexObserver * const observer,
    HdPrman_RenderParam * const renderParam)
  : HdPrman_RileyPrimBase(renderParam)
  , _rileyId(
      _CreateRenderOutput(
          _AcquireRiley(),
          HdPrmanRileyRenderOutputSchema::GetFromParent(primSource)))
{
}

void
HdPrman_RileyRenderOutputPrim::_Dirty(
    const HdSceneIndexObserver::DirtiedPrimEntry &entry,
    const HdsiPrimManagingSceneIndexObserver * const observer)
{
    HdPrmanRileyRenderOutputSchema schema =
        HdPrmanRileyRenderOutputSchema::GetFromParent(
            observer->GetSceneIndex()->GetPrim(entry.primPath).dataSource);

    std::optional<RtUString> name;
    if (entry.dirtyLocators.Intersects(
            HdPrmanRileyRenderOutputSchema::GetNameLocator())) {
        name = _GetString(schema.GetName());
    }

    std::optional<riley::RenderOutputType> type;
    if (entry.dirtyLocators.Intersects(
            HdPrmanRileyRenderOutputSchema::GetTypeLocator())) {
        type = _GetRenderOutputType(schema.GetType());
    }

    std::optional<RtUString> source;
    if (entry.dirtyLocators.Intersects(
            HdPrmanRileyRenderOutputSchema::GetSourceLocator())) {
        source = _GetString(schema.GetSource());
    }

    std::optional<RtUString> accumulationRule;
    if (entry.dirtyLocators.Intersects(
            HdPrmanRileyRenderOutputSchema::GetAccumulationRuleLocator())) {
        accumulationRule = _GetString(schema.GetAccumulationRule());
    }

    std::optional<RtUString> filter;
    if (entry.dirtyLocators.Intersects(
            HdPrmanRileyRenderOutputSchema::GetFilterLocator())) {
        filter = _GetString(schema.GetFilter());
    }

    std::optional<riley::FilterSize> filterSize;
    if (entry.dirtyLocators.Intersects(
            HdPrmanRileyRenderOutputSchema::GetFilterSizeLocator())) {
        filterSize = _GetFilterSize(schema.GetFilterSize());
    }

    std::optional<float> relativePixelVariance;
    if (entry.dirtyLocators.Intersects(
            HdPrmanRileyRenderOutputSchema::GetRelativePixelVarianceLocator())) {
        relativePixelVariance = _GetFloat(schema.GetRelativePixelVariance());
    }

    std::optional<RtParamList> params;
    if (entry.dirtyLocators.Intersects(
            HdPrmanRileyRenderOutputSchema::GetParamsLocator())) {
        params = HdPrman_Utils::ParamsFromDataSource(schema.GetParams());
    }

    _AcquireRiley()->ModifyRenderOutput(
        _rileyId,
        _GetPtr(name),
        _GetPtr(type),
        _GetPtr(source),
        _GetPtr(accumulationRule),
        _GetPtr(filter),
        _GetPtr(filterSize),
        _GetPtr(relativePixelVariance),
        _GetPtr(params));

}

HdPrman_RileyRenderOutputPrim::~HdPrman_RileyRenderOutputPrim()
{
    _AcquireRiley()->DeleteRenderOutput(_rileyId);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
