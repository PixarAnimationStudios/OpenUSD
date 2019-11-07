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
#ifndef PXR_IMAGING_HGI_BLIT_ENCODER_OPS_H
#define PXR_IMAGING_HGI_BLIT_ENCODER_OPS_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/hgi/types.h"

#include <stddef.h>

PXR_NAMESPACE_OPEN_SCOPE


/// \struct HgiCopyResourceOp
///
/// Describes the properties needed to copy resource data to/from GPU/CPU.
///
/// It is the responsibility of the caller to:
///   - ensure the destination buffer is large enough to receive the data 
//      (keep in mind the destinationByteOffset).
///   - ensure the source and destination buffers are valid by the time the 
//      command is executed.
///   - insert the appropriate barriers in the command buffer prior to 
///     reading/writing to/from the buffers.
///
/// <ul>
/// <li>format:
///   The data-type of one element in the source buffer</li>
/// <li>usage:
///   For some platforms knowing the format may not be enough and needs to know
///   if the source/destination is used for e.g. Depth.
/// <li>dimensions:
///   Size of data (in element count) to copy from source to destination.</li>
/// <li>sourceByteOffset:
///   The offset in source buffer where to start copying the data from.
///   For a 2 or 3 dimensionaly buffer you can supply offset[1] and offset[2].</li>
/// <li>sourceBuffer:
///   Where to copy the data from (gpu or cpu)</li>
///
/// <li>destinationByteOffset:
///   The offset in destination buffer where to start copying the data to.
///   For a 2 or 3 dimensionaly buffer you can supply offset[1] and offset[2].</li>
/// <li>destinationBufferByteSize:
///   Size of the destination buffer (in bytes)</li>
/// <li>destinationBuffer:
///   Where to copy the data to (gpu or cpu)</li>
/// </ul>
///
struct HgiCopyResourceOp {
    HgiCopyResourceOp()
    : format(HgiFormatInvalid)
    , usage(HgiTextureUsageBitsColorTarget)
    , dimensions(0)
    , sourceByteOffset(0)
    , cpuSourceBuffer(nullptr)
    , destinationByteOffset(0)
    , destinationBufferByteSize(0)
    , cpuDestinationBuffer(nullptr)
    {}

    // Source
    HgiFormat format;
    HgiTextureUsageBits usage;
    GfVec3i dimensions;
    GfVec3i sourceByteOffset;

    union {
        // XXX HgiBufferHandle gpuSourceBuffer;
        HgiTextureHandle gpuSourceTexture;
        void* cpuSourceBuffer;
    };

    // Destination
    GfVec3i destinationByteOffset;
    size_t destinationBufferByteSize;

    union {
        // XXX HgiBufferHandle gpuDistinationBuffer;
        HgiTextureHandle gpuDestinationTexture;
        void* cpuDestinationBuffer;
    };
};


/// \struct HgiResolveImageOp
///
/// Properties needed to resolve a multi-sample texture into a regular texture.
///
/// <ul>
/// <li>usage:
///   Describes how the texture is intended to be used (depth or color).</li>
/// <li>sourceRegion:
///   Source rectangle (x,y,w,h) to copy from </li>
/// <li>source:
///   The multi-sample source texture</li>
///
/// <li>destinationRegion:
///   Destination rectangle (x,y,w,h) to copy to </li>
/// <li>destination:
///   The non-multi-sample color destination texture</li>
/// </ul>
///
struct HgiResolveImageOp {
    HgiResolveImageOp()
    : usage(HgiTextureUsageBitsColorTarget)
    , sourceRegion(0)
    , source(nullptr)
    , destinationRegion(0)
    , destination(nullptr)
    {}

    HgiTextureUsageBits usage;
    GfVec4i sourceRegion;
    HgiTextureHandle source;
    GfVec4i destinationRegion;
    HgiTextureHandle destination;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
