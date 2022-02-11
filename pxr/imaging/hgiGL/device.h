//
// Copyright 2020 Pixar
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
#ifndef PXR_IMAGING_HGIGL_DEVICE_H
#define PXR_IMAGING_HGIGL_DEVICE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/hgiGL/contextArena.h"
#include "pxr/imaging/hgiGL/hgi.h"

#include <fstream>
#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

class HgiGraphicsCmdDesc;

/// \class HgiGLDevice
///
/// OpenGL implementation of GPU device.
/// Note: HgiGL does not concern itself with GL context management.
///       See notes in hgiGL/hgi.h
///
class HgiGLDevice final {
public:
    HGIGL_API
    HgiGLDevice();

    HGIGL_API
    ~HgiGLDevice();

    /// Execute the provided functions / ops. This will emit the GL calls.
    HGIGL_API
    void SubmitOps(HgiGLOpsVector const& ops);

    /// Sets the active arena to use when submitting commands. This is used
    /// for management of resources tied to a GL context such as FBOs.
    /// The default arena is used if a valid handle isn't provided.
    HGIGL_API
    void SetCurrentArena(HgiGLContextArenaHandle const& arenaHandle);
 
    /// Returns a framebuffer object id that is managed by the active arena.
    HGIGL_API
    uint32_t AcquireFramebuffer(
        HgiGraphicsCmdsDesc const& desc,
        bool resolved = false);
    
    /// Garbage collect resources in the active arena.
    HGIGL_API
    void GarbageCollect();

private:
    HgiGLContextArena const * _GetArena() const;
    HgiGLContextArena * _GetArena();

    HgiGLDevice & operator=(const HgiGLDevice&) = delete;
    HgiGLDevice(const HgiGLDevice&) = delete;

    friend std::ofstream& operator<<(
        std::ofstream& out,
        const HgiGLDevice& dev);

    // The default arena is used in the absence of a user provided arena.
    HgiGLContextArena _defaultArena;
    HgiGLContextArena *_activeArena;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
