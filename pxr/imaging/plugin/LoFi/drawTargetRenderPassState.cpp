//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/plugin/LoFi/drawTargetRenderPassState.h"
#include "pxr/base/vt/value.h"
#include "pxr/imaging/hd/aov.h"

PXR_NAMESPACE_OPEN_SCOPE


LoFiDrawTargetRenderPassState::LoFiDrawTargetRenderPassState()
 : _depthPriority(HdDepthPriorityNearest)
 , _cameraId()
 , _rprimCollection()
 , _rprimCollectionVersion(1) // Clients start at 0
{
}

LoFiDrawTargetRenderPassState::~LoFiDrawTargetRenderPassState() = default;

void
LoFiDrawTargetRenderPassState::SetDepthPriority(HdDepthPriority priority)
{
    _depthPriority = priority;
}

void
LoFiDrawTargetRenderPassState::SetCamera(const SdfPath &cameraId)
{
    _cameraId = cameraId;
}


void LoFiDrawTargetRenderPassState::SetRprimCollection(
                                                   HdRprimCollection const& col)
{
    _rprimCollection = col;
    ++_rprimCollectionVersion;
}

void LoFiDrawTargetRenderPassState::SetAovBindings(
    const HdRenderPassAovBindingVector &aovBindings)
{
    _aovBindings = aovBindings;
}

PXR_NAMESPACE_CLOSE_SCOPE

