//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_DRAW_ITEM_INSTANCE_H
#define PXR_IMAGING_HD_ST_DRAW_ITEM_INSTANCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/drawItem.h"

PXR_NAMESPACE_OPEN_SCOPE

using HdSt_DrawBatchSharedPtr = std::shared_ptr<class HdSt_DrawBatch>;

/// \class HdStDrawItemInstance
///
/// A container to store instance state for a drawitem.
///
/// During culling, the visibility state will be set. If the instance
/// has a batch, the batch will get a DrawItemInstanceChanged
/// callback.
///
/// The Batch is responsible for calling SetBatch and SetBatchIndex
/// when adding / appending the instance. If the batch does not require
/// the DrawItemInstanceChanged callback, then this step can be skipped
///
class HdStDrawItemInstance
{
public:
    HDST_API
    HdStDrawItemInstance(HdStDrawItem const *drawItem);
    HDST_API
    ~HdStDrawItemInstance();

    /// Set visibility state
    HDST_API
    void SetVisible(bool visible);

    /// Query visibility state
    bool IsVisible() const { return _visible; }

    /// Set index into batch list. Can be used by
    /// batch during DrawItemInstanceChanged callback
    HDST_API
    void SetBatchIndex(size_t batchIndex);

    /// Query batch index
    size_t GetBatchIndex() const { return _batchIndex; }

    /// Set the batch that will receive the DrawItemInstanceChanged
    /// callback when visibility is updated. Setting batch to NULL
    /// will disable this callback.
    // HDST_API
    void SetBatch(HdSt_DrawBatch *batch);

    /// Return a const pointer to draw item
    HdStDrawItem const *GetDrawItem() const { return _drawItem; }

private:
    HdStDrawItemInstance();

    HdSt_DrawBatch * _batch;
    HdStDrawItem const * _drawItem;
    size_t _batchIndex;
    bool _visible;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_DRAW_ITEM_INSTANCE_H

