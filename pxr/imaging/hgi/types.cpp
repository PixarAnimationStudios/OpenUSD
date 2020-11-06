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
#include "pxr/pxr.h"
#include "pxr/imaging/hgi/types.h"
#include "pxr/base/tf/diagnostic.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

size_t
HgiGetComponentCount(const HgiFormat f)
{
    switch (f) {
    case HgiFormatUNorm8:
    case HgiFormatSNorm8:
    case HgiFormatFloat16:
    case HgiFormatFloat32:
    case HgiFormatUInt16:
    case HgiFormatInt32:
    case HgiFormatFloat32UInt8: // treat as a single component
        return 1;
    case HgiFormatUNorm8Vec2:
    case HgiFormatSNorm8Vec2:
    case HgiFormatFloat16Vec2:
    case HgiFormatFloat32Vec2:
    case HgiFormatUInt16Vec2:
    case HgiFormatInt32Vec2:
        return 2;
    // case HgiFormatUNorm8Vec3: // Unsupported Metal (MTLPixelFormat)
    // case HgiFormatSNorm8Vec3: // Unsupported Metal (MTLPixelFormat)
    case HgiFormatFloat16Vec3:
    case HgiFormatFloat32Vec3:
    case HgiFormatUInt16Vec3:
    case HgiFormatInt32Vec3:
    case HgiFormatBC6FloatVec3:
    case HgiFormatBC6UFloatVec3:
        return 3;
    case HgiFormatUNorm8Vec4:
    case HgiFormatSNorm8Vec4:
    case HgiFormatFloat16Vec4:
    case HgiFormatFloat32Vec4:
    case HgiFormatUInt16Vec4:
    case HgiFormatInt32Vec4:
    case HgiFormatBC7UNorm8Vec4:
    case HgiFormatBC7UNorm8Vec4srgb:
    case HgiFormatUNorm8Vec4srgb:
    case HgiFormatBC1UNorm8Vec4:
    case HgiFormatBC3UNorm8Vec4:
        return 4;
    case HgiFormatCount:
    case HgiFormatInvalid:
        TF_CODING_ERROR("Invalid Format");
        return 0;
    }
    TF_CODING_ERROR("Missing Format");
    return 0;
}

size_t
HgiGetDataSizeOfFormat(
    const HgiFormat f,
    size_t * const blockWidth,
    size_t * const blockHeight)
{
    if (blockWidth) {
        *blockWidth = 1;
    }
    if (blockHeight) {
        *blockHeight = 1;
    }

    switch(f) {
    case HgiFormatUNorm8:
    case HgiFormatSNorm8:
        return 1;
    case HgiFormatUNorm8Vec2:
    case HgiFormatSNorm8Vec2:
        return 2;
    // case HgiFormatUNorm8Vec3: // Unsupported Metal (MTLPixelFormat)
    // case HgiFormatSNorm8Vec3: // Unsupported Metal (MTLPixelFormat)
    //     return 3;
    case HgiFormatUNorm8Vec4:
    case HgiFormatSNorm8Vec4:
    case HgiFormatUNorm8Vec4srgb:
        return 4;
    case HgiFormatFloat16:
    case HgiFormatUInt16:
        return 2;
    case HgiFormatFloat16Vec2:
    case HgiFormatUInt16Vec2:
        return 4;
    case HgiFormatFloat16Vec3:
    case HgiFormatUInt16Vec3:
        return 6;
    case HgiFormatFloat16Vec4:
    case HgiFormatUInt16Vec4:
        return 8;
    case HgiFormatFloat32:
    case HgiFormatInt32:
        return 4;
    case HgiFormatFloat32Vec2:
    case HgiFormatInt32Vec2:
    case HgiFormatFloat32UInt8: // XXX: implementation dependent
        return 8;
    case HgiFormatFloat32Vec3:
    case HgiFormatInt32Vec3:
        return 12;
    case HgiFormatFloat32Vec4:
    case HgiFormatInt32Vec4:
        return 16;
    case HgiFormatBC6FloatVec3:
    case HgiFormatBC6UFloatVec3:
    case HgiFormatBC7UNorm8Vec4:
    case HgiFormatBC7UNorm8Vec4srgb:
    case HgiFormatBC1UNorm8Vec4:
    case HgiFormatBC3UNorm8Vec4:
        if (blockWidth) {
            *blockWidth = 4;
        }
        if (blockHeight) {
            *blockHeight = 4;
        }
        return 16;
    case HgiFormatCount:
    case HgiFormatInvalid:
        TF_CODING_ERROR("Invalid Format");
        return 0;
    }
    TF_CODING_ERROR("Missing Format");
    return 0;
}

bool
HgiIsCompressed(const HgiFormat f)
{
    switch(f) {
    case HgiFormatBC6FloatVec3:
    case HgiFormatBC6UFloatVec3:
    case HgiFormatBC7UNorm8Vec4:
    case HgiFormatBC7UNorm8Vec4srgb:
    case HgiFormatBC1UNorm8Vec4:
    case HgiFormatBC3UNorm8Vec4:
        return true;
    default:
        return false;
    }
}

size_t
HgiGetDataSize(
    const HgiFormat format,
    const GfVec3i &dimensions)
{
    size_t blockWidth, blockHeight;
    const size_t bpt =
        HgiGetDataSizeOfFormat(format, &blockWidth, &blockHeight);
    return
        ((dimensions[0] + blockWidth  - 1) / blockWidth ) *
        ((dimensions[1] + blockHeight - 1) / blockHeight) *
        std::max(1, dimensions[2]) *
        bpt;
}

uint16_t
_ComputeNumMipLevels(const GfVec3i &dimensions)
{
    const int dim = std::max({dimensions[0], dimensions[1], dimensions[2]});
    
    for (uint16_t i = 1; i < 8 * sizeof(int) - 1; i++) {
        const int powerTwo = 1 << i;
        if (powerTwo > dim) {
            return i;
        }
    }
    
    // Can never be reached, but compiler doesn't know that.
    return 1;
}

std::vector<HgiMipInfo>
HgiGetMipInfos(
    const HgiFormat format,
    const GfVec3i& dimensions,
    const size_t layerCount,
    const size_t dataByteSize)
{
    const bool is2DArray = layerCount > 1;
    if (is2DArray && dimensions[2] != 1) {
        TF_CODING_ERROR("An array of 3D textures is invalid");
    }

    const uint16_t numMips = _ComputeNumMipLevels(dimensions);

    std::vector<HgiMipInfo> result;
    result.reserve(numMips);

    size_t byteOffset = 0;
    GfVec3i size = dimensions;

    for (uint16_t mipLevel = 0; mipLevel < numMips; mipLevel++) {
        const size_t byteSize = HgiGetDataSize(format, size);

        result.push_back({ byteOffset, size, byteSize });

        byteOffset += byteSize * layerCount;
        if (byteOffset >= dataByteSize) {
            break;
        }

        size[0] = std::max(size[0] / 2, 1);
        size[1] = std::max(size[1] / 2, 1);
        size[2] = std::max(size[2] / 2, 1);
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
