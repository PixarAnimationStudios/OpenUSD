//
// Copyright 2020 benmalartre
//
// Unlicensed
// 
#include "pxr/imaging/plugin/LoFi/drawTargetRenderPassState.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE


LoFiDrawTargetRenderPassState::LoFiDrawTargetRenderPassState()
 : _colorClearValues()
 , _depthClearValue(1.0f)
 , _depthPriority(HdDepthPriorityNearest)
 , _cameraId()
 , _rprimCollection()
 , _rprimCollectionVersion(1) // Clients start at 0
{

}

LoFiDrawTargetRenderPassState::~LoFiDrawTargetRenderPassState()
{
}


void
LoFiDrawTargetRenderPassState::SetNumColorAttachments(size_t numAttachments)
{
    _colorClearValues.resize(numAttachments);
}

void
LoFiDrawTargetRenderPassState::SetColorClearValue(size_t attachmentIdx,
                                           const VtValue &clearValue)
{
    TF_DEV_AXIOM(attachmentIdx < _colorClearValues.size());
    _colorClearValues[attachmentIdx] = clearValue;
}

void
LoFiDrawTargetRenderPassState::SetDepthClearValue(float clearValue)
{
    _depthClearValue = clearValue;
}

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

PXR_NAMESPACE_CLOSE_SCOPE

