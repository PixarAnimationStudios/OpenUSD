//
// Copyright 2019 Pixar
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
#ifndef PXR_IMAGING_HDX_OIT_BUFFER_ACCESSOR_H
#define PXR_IMAGING_HDX_OIT_BUFFER_ACCESSOR_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"

#include "pxr/imaging/hd/task.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

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
    void InitializeOitBuffersIfNecessary();

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
