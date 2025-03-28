//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/drawItemInstance.h"
#include "pxr/imaging/hdSt/drawBatch.h"
#include "pxr/imaging/hdSt/drawItem.h"

PXR_NAMESPACE_OPEN_SCOPE


HdStDrawItemInstance::HdStDrawItemInstance(HdStDrawItem const* drawItem)
    : _batch(nullptr)
    , _drawItem(drawItem)
    , _batchIndex(0)
    , _visible(drawItem->GetVisible())
{
}

HdStDrawItemInstance::~HdStDrawItemInstance()
{
}

void
HdStDrawItemInstance::SetVisible(bool visible)
{
    if (_visible != visible) {
        _visible = visible;
        if (_batch) {
            _batch->DrawItemInstanceChanged(this);
        }
    }
}

void
HdStDrawItemInstance::SetBatchIndex(size_t batchIndex)
{
    _batchIndex = batchIndex;
}

void
HdStDrawItemInstance::SetBatch(HdSt_DrawBatch *batch)
{
    _batch = batch;
}

PXR_NAMESPACE_CLOSE_SCOPE

