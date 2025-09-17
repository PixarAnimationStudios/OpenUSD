//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_OIT_BUFFER_ACCESSOR_H
#define PXR_IMAGING_HDX_OIT_BUFFER_ACCESSOR_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"

#include "pxr/imaging/hd/task.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;

using HdBufferArrayRangeSharedPtr = 
    std::shared_ptr<class HdBufferArrayRange>;

using HdStRenderPassShaderSharedPtr =
    std::shared_ptr<class HdStRenderPassShader>;

/// Class for OIT render tasks to access the OIT buffers.
class HdxOitBufferAccessor {
public:
    static bool IsOitEnabled();

    HDX_API
    HdxOitBufferAccessor(HdTaskContext *ctx);

    /// Called during Prepare to indicate that OIT buffers are needed.
    HDX_API
    void RequestOitBuffers();

    /// Called during Excecute before writing to OIT buffers.
    HDX_API
    void InitializeOitBuffersIfNecessary(Hgi *hgi);

    /// Called during Execute to add necessary OIT buffer shader bindings.
    ///
    /// Returns false if the OIT buffers were not allocated.
    HDX_API
    bool AddOitBufferBindings(const HdStRenderPassShaderSharedPtr &);

private:
    HdBufferArrayRangeSharedPtr const &_GetBar(const TfToken &);

    HdTaskContext * const _ctx;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
