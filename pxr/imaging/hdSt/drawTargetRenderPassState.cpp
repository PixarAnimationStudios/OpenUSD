//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/drawTargetRenderPassState.h"
#include "pxr/base/vt/value.h"
#include "pxr/imaging/hd/aov.h"

PXR_NAMESPACE_OPEN_SCOPE


HdStDrawTargetRenderPassState::HdStDrawTargetRenderPassState()
 : _depthPriority(HdDepthPriorityNearest)
 , _cameraId()
 , _rprimCollection()
 , _rprimCollectionVersion(1) // Clients start at 0
{
}

HdStDrawTargetRenderPassState::~HdStDrawTargetRenderPassState() = default;

void
HdStDrawTargetRenderPassState::SetDepthPriority(HdDepthPriority priority)
{
    _depthPriority = priority;
}

void
HdStDrawTargetRenderPassState::SetCamera(const SdfPath &cameraId)
{
    _cameraId = cameraId;
}


void HdStDrawTargetRenderPassState::SetRprimCollection(
                                                   HdRprimCollection const& col)
{
    _rprimCollection = col;
    ++_rprimCollectionVersion;
}

void HdStDrawTargetRenderPassState::SetAovBindings(
    const HdRenderPassAovBindingVector &aovBindings)
{
    _aovBindings = aovBindings;
}

PXR_NAMESPACE_CLOSE_SCOPE

