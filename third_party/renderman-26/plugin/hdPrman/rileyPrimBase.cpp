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

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
