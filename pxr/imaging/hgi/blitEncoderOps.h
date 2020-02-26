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


//// \struct HgiTextureGpuToCpuOp
///
/// Describes the properties needed to copy texture data from GPU to CPU.
///
/// It is the responsibility of the caller to:
///   - ensure the destination buffer is large enough to receive the data
///     (keep in mind the destinationByteOffset, mipLevel, numLayers, etc).
///   - ensure the source texture and destination buffer are valid at the time
///     the command is executed.
///   - insert the appropriate barriers in the command buffer prior to
///     reading/writing to/from the resources.
///
/// <ul>
/// <li>gpuSourceTexture:
///   The gpu texture to copy pixels from.</li>
/// <li>sourceTexelOffset:
///   The texel offset (width, height, depth) of where to start copying.</li>
/// <li>mipLevel:
///   Mip level to copy from.</li>
/// <li>startLayer:
///   The first layer to start copying from.</li>
/// <li>numLayers:
///   The number of layers to copy.</li>
/// <li>cpuDestinationBuffer:
///   The copy destination cpu buffer.</li>
/// <li>destinationByteOffset:
///   The byte offset in destination buffer where to start copying the data to.</li>
/// <li>destinationBufferByteSize:
///   Size of the destination buffer (in bytes)</li>
/// </ul>
///
struct HgiTextureGpuToCpuOp
{
    HgiTextureGpuToCpuOp()
    : gpuSourceTexture()
    , sourceTexelOffset(GfVec3i(0))
    , mipLevel(0)
    , startLayer(0)
    , numLayers(1)
    , cpuDestinationBuffer(nullptr)
    , destinationByteOffset(0)
    , destinationBufferByteSize(0)
    {}

    HgiTextureHandle gpuSourceTexture;
    GfVec3i sourceTexelOffset;
    uint32_t mipLevel;
    uint32_t startLayer;
    uint32_t numLayers;
    void* cpuDestinationBuffer;
    size_t destinationByteOffset;
    size_t destinationBufferByteSize;
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
struct HgiResolveImageOp
{
    HgiResolveImageOp()
    : usage(HgiTextureUsageBitsColorTarget)
    , sourceRegion(0)
    , source()
    , destinationRegion(0)
    , destination()
    {}

    HgiTextureUsageBits usage;
    GfVec4i sourceRegion;
    HgiTextureHandle source;
    GfVec4i destinationRegion;
    HgiTextureHandle destination;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
