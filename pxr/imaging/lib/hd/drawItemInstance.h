//
// Copyright 2016 Pixar
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
#ifndef _HD_DRAW_ITEM_INSTANCE_H_
#define _HD_DRAW_ITEM_INSTANCE_H_

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "boost/shared_ptr.hpp"

PXR_NAMESPACE_OPEN_SCOPE

class Hd_DrawBatch;
class HdDrawItem;
class HdDrawItemInstance;

typedef boost::shared_ptr<Hd_DrawBatch> HdDrawBatchSharedPtr;
typedef boost::shared_ptr<HdDrawItem> HdDrawItemSharedPtr;

/// \class HdDrawItemInstance
///
/// A container to store instance state for a drawitem.
///
/// During culling, the visiblity state will be set. If the instance
/// has a batch, the batch will get a DrawItemInstanceChanged
/// callback.
///
/// The Batch is responsible for calling SetBatch and SetBatchIndex
/// when adding / appending the instance. If the batch does not require
/// the DrawItemInstanceChanged callback, then this step can be skipped
///
class HdDrawItemInstance
{
public:
    HD_API
    HdDrawItemInstance(HdDrawItem const *drawItem);
    HD_API
    ~HdDrawItemInstance();

    /// Set visibility state
    HD_API
    void SetVisible(bool visible);

    /// Query visibility state
    bool IsVisible() const { return _visible; }

    /// Set index into batch list. Can be used by
    /// batch during DrawItemInstanceChanged callback
    HD_API
    void SetBatchIndex(size_t batchIndex);

    /// Query batch index
    size_t GetBatchIndex() const { return _batchIndex; }

    /// Set the batch that will receive the DrawItemInstanceChanged
    /// callback when visibility is updated. Setting batch to NULL
    /// will disable this callback.
    // HD_API
    void SetBatch(Hd_DrawBatch *batch);

    /// Return a const pointer to draw item
    HdDrawItem const *GetDrawItem() const { return _drawItem; }

private:
    HdDrawItemInstance();

    Hd_DrawBatch * _batch;
    HdDrawItem const * _drawItem;
    size_t _batchIndex;
    bool _visible;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // _HD_DRAW_ITEM_INSTANCE_H_

