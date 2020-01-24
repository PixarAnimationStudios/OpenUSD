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
#ifndef PXR_IMAGING_HGI_BLIT_ENCODER_H
#define PXR_IMAGING_HGI_BLIT_ENCODER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"

PXR_NAMESPACE_OPEN_SCOPE

struct HgiCopyResourceOp;
struct HgiResolveImageOp;


/// \class HgiBlitEncoder
///
/// A graphics API independent abstraction of resource copy commands.
/// HgiBlitEncoder is a lightweight object that cannot be re-used after
/// EndEncoding. A new encoder should be acquired from CommandBuffer each frame.
///
/// The API provided by this encoder should be agnostic to whether the
/// encoder operates via immediate or deferred command buffers.
///
class HgiBlitEncoder
{
public:
    HGI_API
    HgiBlitEncoder();

    HGI_API
    virtual ~HgiBlitEncoder();

    /// Finish recording of commands. No further commands can be recorded.
    HGI_API
    virtual void EndEncoding() = 0;

    /// Push a debug marker onto the encoder.
    HGI_API
    virtual void PushDebugGroup(const char* label) = 0;

    /// Pop the lastest debug marker off encoder.
    HGI_API
    virtual void PopDebugGroup() = 0;

    /// Copy a texture resource from GPU to CPU.
    HGI_API
    virtual void CopyTextureGpuToCpu(HgiCopyResourceOp const& copyOp) = 0;

    /// Resolve a multi-sample texture (MSAA) so it can be read from.
    HGI_API
    virtual void ResolveImage(HgiResolveImageOp const& resolveOp)= 0;

private:
    HgiBlitEncoder & operator=(const HgiBlitEncoder&) = delete;
    HgiBlitEncoder(const HgiBlitEncoder&) = delete;
};



PXR_NAMESPACE_CLOSE_SCOPE

#endif
