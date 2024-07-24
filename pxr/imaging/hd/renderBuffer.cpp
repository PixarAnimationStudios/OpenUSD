//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/sceneDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

HdRenderBuffer::HdRenderBuffer(SdfPath const& id)
    : HdBprim(id)
{
}

HdRenderBuffer::~HdRenderBuffer() = default;

/*virtual*/
HdDirtyBits
HdRenderBuffer::GetInitialDirtyBitsMask() const
{
    return HdRenderBuffer::AllDirty;
}

/*virtual*/
void
HdRenderBuffer::Sync(HdSceneDelegate *sceneDelegate,
                     HdRenderParam *renderParam,
                     HdDirtyBits *dirtyBits)
{
    if (*dirtyBits & DirtyDescription) {
        const HdRenderBufferDescriptor desc =
            sceneDelegate->GetRenderBufferDescriptor(GetId());

        if (!(desc.dimensions[0] >= 0 &&
              desc.dimensions[1] >= 0 &&
              desc.dimensions[2] >= 0)) {
            TF_CODING_ERROR("Bad dimensions for render buffer %s",
                            GetId().GetText());
            return;
        }

        Allocate(desc.dimensions, desc.format, desc.multiSampled);
    }
    *dirtyBits &= ~AllDirty;
}

/*virtual*/
void
HdRenderBuffer::Finalize(HdRenderParam *renderParam)
{
    _Deallocate();
}

PXR_NAMESPACE_CLOSE_SCOPE
