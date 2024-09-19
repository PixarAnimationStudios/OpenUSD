//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "hdPrman/rileyPrimBase.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "hdPrman/renderParam.h"

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_RileyPrimBase::HdPrman_RileyPrimBase(
    HdPrman_RenderParam * const renderParam)
  : _renderParam(renderParam)
{
}

riley::Riley *
HdPrman_RileyPrimBase::_AcquireRiley()
{
    return _renderParam->AcquireRiley();
}

const GfVec2f &
HdPrman_RileyPrimBase::_GetShutterInterval()
{
    return _renderParam->GetShutterInterval();
}

void
HdPrman_RileyPrimBase::_SetRileyOptions(const RtParamList &params)
{
    // Ideally, all riley options are managed by scene indices and
    // this would just do:
    // _AcquireRiley()->SetOptions(options.rileyObject);

    // But we also need to respect various legacy options in render
    // param:
    _renderParam->SetRileySceneIndexObserverOptions(params);

    // Initialize legacy options from the render settings map.
    _renderParam->UpdateLegacyOptions();

    if (TfGetEnvSetting(HD_PRMAN_EXPERIMENTAL_RILEY_SCENE_INDEX_OBSERVER)) {
        _renderParam->SetRileyOptions();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
