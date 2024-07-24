//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_COMMAND_BUFFER_H
#define PXR_IMAGING_HD_ST_COMMAND_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hdSt/drawItemInstance.h"

#include "pxr/base/gf/matrix4d.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderIndex;
class HdStDrawItem;
class HdStDrawItemInstance;
class Hgi;
class HgiGraphicsCmds;

using HdStRenderPassStateSharedPtr = std::shared_ptr<class HdStRenderPassState>;
using HdStResourceRegistrySharedPtr = 
        std::shared_ptr<class HdStResourceRegistry>;

using HdDrawItemConstPtrVector = std::vector<class HdDrawItem const*>;
using HdDrawItemConstPtrVectorSharedPtr
    = std::shared_ptr<HdDrawItemConstPtrVector>;

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
    void PrepareDraw(HgiGraphicsCmds *gfxCmds,
                     HdStRenderPassStateSharedPtr const &renderPassState,
                     HdRenderIndex *renderIndex);

    /// Execute the command buffer
    HDST_API
    void ExecuteDraw(HgiGraphicsCmds *gfxCmds,
                     HdStRenderPassStateSharedPtr const &renderPassState,
                     HdStResourceRegistrySharedPtr const &resourceRegistry);

    /// Sync visibility state from RprimSharedState to DrawItemInstances.
    HDST_API
    void SyncDrawItemVisibility(unsigned visChangeCount);
 
    /// Sets the draw items to use for batching.
    /// If the shared pointer or version is different, batches are rebuilt and
    /// the batch version is updated.
    HDST_API
    void SetDrawItems(HdDrawItemConstPtrVectorSharedPtr const &drawItems,
                      unsigned currentBatchVersion,
                      Hgi const *hgi);

    /// Rebuild all draw batches if any underlying buffer array is invalidated.
    HDST_API
    void RebuildDrawBatchesIfNeeded(unsigned currentBatchVersion,
                                    Hgi const *hgi);

    /// Returns the total number of draw items, including culled items.
    size_t GetTotalSize() const {
        if (_drawItems) return _drawItems->size();
        return 0;
    }

    /// Returns the number of draw items, excluding culled items.
    size_t GetVisibleSize() const { return _visibleSize; }

    /// Returns the number of culled draw items.
    size_t GetCulledSize() const {
        if (_drawItems) {
            return _drawItems->size() - _visibleSize; 
        }
        return 0;
    }

    HDST_API
    void SetEnableTinyPrimCulling(bool tinyPrimCulling);

private:
    void _RebuildDrawBatches(Hgi const *hgi);
    
    /// Cull drawItemInstances based on view frustum cull matrix
    void _FrustumCullCPU(GfMatrix4d const &cullMatrix);

    HdDrawItemConstPtrVectorSharedPtr _drawItems;
    std::vector<HdStDrawItemInstance> _drawItemInstances;
    HdSt_DrawBatchSharedPtrVector _drawBatches;
    size_t _visibleSize;
    unsigned int _visChangeCount;
    unsigned int _drawBatchesVersion;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_COMMAND_BUFFER_H
