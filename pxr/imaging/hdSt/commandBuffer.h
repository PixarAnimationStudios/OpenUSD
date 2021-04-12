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
#ifndef PXR_IMAGING_HD_ST_COMMAND_BUFFER_H
#define PXR_IMAGING_HD_ST_COMMAND_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hdSt/drawItemInstance.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class HdStDrawItem;
class HdStDrawItemInstance;

using HdStRenderPassStateSharedPtr = std::shared_ptr<class HdStRenderPassState>;
using HdStResourceRegistrySharedPtr = 
        std::shared_ptr<class HdStResourceRegistry>;

using HdSt_DrawBatchSharedPtr = std::shared_ptr<class HdSt_DrawBatch>;
using HdSt_DrawBatchSharedPtrVector = std::vector<HdSt_DrawBatchSharedPtr>;

/// \class HdStCommandBuffer
///
/// A buffer of commands (HdStDrawItem or HdComputeItem objects) to be executed.
///
/// The HdStCommandBuffer is responsible for accumulating draw items and sorting
/// them for correctness (e.g. alpha transparency) and efficiency (e.g. the
/// fewest number of GPU state changes).
///
class HdStCommandBuffer {
public:
    HDST_API
    HdStCommandBuffer();
    HDST_API
    ~HdStCommandBuffer();

    /// Prepare the command buffer for draw
    HDST_API
    void PrepareDraw(HdStRenderPassStateSharedPtr const &renderPassState,
                     HdStResourceRegistrySharedPtr const &resourceRegistry);

    /// Execute the command buffer
    HDST_API
    void ExecuteDraw(HdStRenderPassStateSharedPtr const &renderPassState,
                     HdStResourceRegistrySharedPtr const &resourceRegistry);

    /// Cull drawItemInstances based on passed in combined view and projection matrix
    HDST_API
    void FrustumCull(GfMatrix4d const &cullMatrix);

    /// Sync visibility state from RprimSharedState to DrawItemInstances.
    HDST_API
    void SyncDrawItemVisibility(unsigned visChangeCount);

    /// Destructively swaps the contents of \p items with the internal list of
    /// all draw items. Culling state is reset, with no items visible.
    HDST_API
    void SwapDrawItems(std::vector<HdStDrawItem const*>* items,
                       unsigned currentBatchVersion);

    /// Rebuild all draw batches if any underlying buffer array is invalidated.
    HDST_API
    void RebuildDrawBatchesIfNeeded(unsigned currentBatchVersion);

    /// Returns the total number of draw items, including culled items.
    size_t GetTotalSize() const { return _drawItems.size(); }

    /// Returns the number of draw items, excluding culled items.
    size_t GetVisibleSize() const { return _visibleSize; }

    /// Returns the number of culled draw items.
    size_t GetCulledSize() const { 
        return _drawItems.size() - _visibleSize; 
    }

    HDST_API
    void SetEnableTinyPrimCulling(bool tinyPrimCulling);

private:
    void _RebuildDrawBatches();

    std::vector<HdStDrawItem const*> _drawItems;
    std::vector<HdStDrawItemInstance> _drawItemInstances;
    HdSt_DrawBatchSharedPtrVector _drawBatches;
    size_t _visibleSize;
    unsigned int _visChangeCount;
    unsigned int _drawBatchesVersion;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_COMMAND_BUFFER_H
