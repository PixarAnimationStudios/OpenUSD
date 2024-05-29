//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/renderPass.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

HdRenderPass::HdRenderPass(HdRenderIndex *index,
                           HdRprimCollection const& collection)
    : _renderIndex(index)
{
    SetRprimCollection(collection);
}

HdRenderPass::~HdRenderPass()
{
    /*NOTHING*/
}

void 
HdRenderPass::SetRprimCollection(HdRprimCollection const& col)
{
    if (col == _collection) {
        return;
    }

    _collection = col; 

    // Mark the collection dirty in derived classes.
    _MarkCollectionDirty();
}

void
HdRenderPass::Execute(HdRenderPassStateSharedPtr const &renderPassState,
                      TfTokenVector const &renderTags)
{
    _Execute(renderPassState, renderTags);
}

void
HdRenderPass::Sync()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Enqueue the collection to be synced during Hydra Sync.
    // XXX: This is currently necessary for Hydra's core to know the reprs to
    // sync for the dirty rprims.
    // The collection paths are ignored.
    _renderIndex->EnqueueCollectionToSync(_collection);

    // Give derived classes a chance to sync.
    _Sync();
}

PXR_NAMESPACE_CLOSE_SCOPE
