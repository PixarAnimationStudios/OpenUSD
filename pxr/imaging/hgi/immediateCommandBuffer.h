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
#ifndef PXR_IMAGING_HGI_IMMEDIATE_COMMAND_BUFFER_H
#define PXR_IMAGING_HGI_IMMEDIATE_COMMAND_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include <memory>


PXR_NAMESPACE_OPEN_SCOPE

struct HgiGraphicsEncoderDesc;

typedef std::unique_ptr<class HgiGraphicsEncoder> HgiGraphicsEncoderUniquePtr;
typedef std::unique_ptr<class HgiBlitEncoder> HgiBlitEncoderUniquePtr;


/// \class HgiImmediateCommandBuffer
///
/// The encoders created from this command buffer are assumed to execute the
/// commands immediately in the rendering backend. There is no list of commands
/// that is recorded and submitted later.
///
/// XXX HgiImmediateCommandBuffer assists in removing OpenGL from HdSt and Tasks
/// while preserving the 'immediate' nature of how HdSt and Tasks currently 
/// expect commands to be executed. In the future code may switch over to
/// deferred command buffers that group together and delay executing commands.
///
class HgiImmediateCommandBuffer
{
public:
    HGI_API
    HgiImmediateCommandBuffer();

    HGI_API
    virtual ~HgiImmediateCommandBuffer();

    /// Returns a graphics encoder for temporary use that is ready to 
    /// execute draw commands. GraphicsEncoder is a lightweight object that
    /// should be re-acquired each frame (don't hold onto it after EndEncoding).
    HGI_API
    virtual HgiGraphicsEncoderUniquePtr CreateGraphicsEncoder(
        HgiGraphicsEncoderDesc const& desc) = 0;

    /// Returns a blit encoder for temporary use that is ready to execute 
    /// resource copy commands. BlitEncoder is a lightweight object that
    /// should be re-acquired each frame (don't hold onto it after EndEncoding).
    HGI_API
    virtual HgiBlitEncoderUniquePtr CreateBlitEncoder() = 0;

private:
    HgiImmediateCommandBuffer & operator=
        (const HgiImmediateCommandBuffer&) = delete;
    HgiImmediateCommandBuffer(const HgiImmediateCommandBuffer&) = delete;
};



PXR_NAMESPACE_CLOSE_SCOPE

#endif
