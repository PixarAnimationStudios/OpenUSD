//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_RENDER_PASS_H
#define PXR_IMAGING_HD_ST_RENDER_PASS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/commandBuffer.h"
#include "pxr/imaging/hd/renderPass.h"

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;

/// \class HdSt_RenderPass
///
/// A single draw pass to a render target/buffer. Stream implementation.
///
class HdSt_RenderPass : public HdRenderPass {
public:
    HDST_API
    HdSt_RenderPass(HdRenderIndex *index, HdRprimCollection const &collection);
    HDST_API
    virtual ~HdSt_RenderPass();

    /// Returns whether the render pass has draw items to be submitted
    /// during _Execute. This may be queried during the Prepare and Execute
    /// phases of the task owning the render pass.
    /// This information is useful to prevent unnecessary resource allocation
    /// and pipeline state changes.
    HDST_API
    bool HasDrawItems(TfTokenVector const &renderTags) const;

protected:
    virtual void _Execute(HdRenderPassStateSharedPtr const &renderPassState,
                          TfTokenVector const &renderTags) override;

    virtual void _MarkCollectionDirty() override;

private:
    void _UpdateDrawItems(TfTokenVector const& renderTags);
    void _UpdateCommandBuffer(TfTokenVector const& renderTags);

    // -----------------------------------------------------------------------
    // Drawing state
    HdStCommandBuffer _cmdBuffer;

    int _lastSettingsVersion;
    bool _useTinyPrimCulling;

    // -----------------------------------------------------------------------
    // Change tracking state.
    // XXX: This is necessary only when not using the draw items cache.

    // The version number of the currently held collection.
    int _collectionVersion;

    // The version number of the render tag opinion of rprims
    int _rprimRenderTagVersion;

    // The version number of the render tags opinion of tasks.
    int _taskRenderTagsVersion;
    
    // The renderTags passed to _Execute(), for comparison next _Execute().
    TfTokenVector _prevRenderTags;

    // The version number of the material tags (of the rprims).
    unsigned int _materialTagsVersion;

    // The version number of the geom subset draw items.
    unsigned int _geomSubsetDrawItemsVersion;

    // A flag indicating that the held collection changed since this renderPass
    // was last drawn.
    //
    // When _collectionChanged is true, it indicates that _collectionVersion is
    // no longer accurate, because _collectionVersion was stored for the
    // previously held collection.
    bool _collectionChanged;

    // -----------------------------------------------------------------------
    // DrawItems that are used to build the draw batches.
    HdDrawItemConstPtrVectorSharedPtr _drawItems;
    size_t _drawItemCount;
    bool _drawItemsChanged;

    Hgi* _hgi;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_RENDER_PASS_H
