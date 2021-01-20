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
#include "pxr/imaging/hio/types.h"
#include "pxr/base/gf/half.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/tf/iterator.h"

PXR_NAMESPACE_OPEN_SCOPE


// A few random format validations to make sure the HioFormat switch stays
// aligned with the HioFormat table.
constexpr bool _CompileTimeValidateHioFormatSwitch() {
    return (HioFormatCount == 46 &&
            HioFormatUNorm8 == 0 &&
            HioFormatFloat32 == 12 &&
            HioFormatUInt32 == 28 &&
            HioFormatBC6FloatVec3 == 40 &&
            HioFormatBC1UNorm8Vec4 == 44) ? true : false;
}

static_assert(_CompileTimeValidateHioFormatSwitch(),
              "switch in HioTypes out of sync with HioFormat enum");

static HioFormat _hioFormats[][4] = {
    { HioFormatUNorm8, HioFormatUNorm8Vec2,
      HioFormatUNorm8Vec3, HioFormatUNorm8Vec4 },
    { HioFormatUNorm8srgb, HioFormatUNorm8Vec2srgb,
      HioFormatUNorm8Vec3srgb, HioFormatUNorm8Vec4srgb },
    { HioFormatSNorm8, HioFormatSNorm8Vec2,
      HioFormatSNorm8Vec3, HioFormatSNorm8Vec4 },
    { HioFormatUInt16, HioFormatUInt16Vec2,
      HioFormatUInt16Vec3, HioFormatUInt16Vec4 },
    { HioFormatInt16, HioFormatInt16Vec2,
      HioFormatInt16Vec3, HioFormatInt16Vec4 },
    { HioFormatUInt32, HioFormatUInt32Vec2,
      HioFormatUInt32Vec3, HioFormatUInt32Vec4 },
    { HioFormatInt32, HioFormatInt32Vec2,
      HioFormatInt32Vec3, HioFormatInt32Vec4 },
    { HioFormatFloat16, HioFormatFloat16Vec2,
      HioFormatFloat16Vec3, HioFormatFloat16Vec4 },
    { HioFormatFloat32, HioFormatFloat32Vec2,
      HioFormatFloat32Vec3, HioFormatFloat32Vec4 },
    { HioFormatDouble64, HioFormatDouble64Vec2,
      HioFormatDouble64Vec3, HioFormatDouble64Vec4 },
};

static_assert(
    TfArraySize(_hioFormats) == HioTypeCount,
    "_hioFormats array in HioUtils out of sync with "
    "HioColorChannelType enum");

HioFormat
HioGetFormat(uint32_t nchannels,
             HioType type,
             bool isSRGB)
{
    if (type >= HioTypeCount) {
        TF_CODING_ERROR("Invalid type");
        return HioFormatInvalid;
    }

    if (nchannels == 0 || nchannels > 4) {
        TF_CODING_ERROR("Invalid channel count");
        return HioFormatInvalid;
    }
    
    if (isSRGB && type == HioTypeUnsignedByte) {
        type = HioTypeUnsignedByteSRGB;
    }

    return _hioFormats[type][nchannels - 1];
}

HioType 
HioGetHioType(HioFormat format)
{
    switch (format) {
        case HioFormatUNorm8:
        case HioFormatUNorm8Vec2:
        case HioFormatUNorm8Vec3:
        case HioFormatUNorm8Vec4:

        case HioFormatUNorm8srgb:
        case HioFormatUNorm8Vec2srgb:
        case HioFormatUNorm8Vec3srgb:
        case HioFormatUNorm8Vec4srgb:
        
        case HioFormatBC7UNorm8Vec4:
        case HioFormatBC7UNorm8Vec4srgb:
        case HioFormatBC1UNorm8Vec4:
        case HioFormatBC3UNorm8Vec4:
            return HioTypeUnsignedByte;

        case HioFormatSNorm8:
        case HioFormatSNorm8Vec2:
        case HioFormatSNorm8Vec3:
        case HioFormatSNorm8Vec4:
            return HioTypeSignedByte;

        case HioFormatFloat16:
        case HioFormatFloat16Vec2:
        case HioFormatFloat16Vec3:
        case HioFormatFloat16Vec4:
            return HioTypeHalfFloat;

        case HioFormatFloat32:
        case HioFormatFloat32Vec2:
        case HioFormatFloat32Vec3:
        case HioFormatFloat32Vec4:

        case HioFormatBC6FloatVec3:
        case HioFormatBC6UFloatVec3:
            return HioTypeFloat;

        case HioFormatDouble64:
        case HioFormatDouble64Vec2:
        case HioFormatDouble64Vec3:
        case HioFormatDouble64Vec4:
            return HioTypeDouble;

        case HioFormatUInt16:
        case HioFormatUInt16Vec2:
        case HioFormatUInt16Vec3:
        case HioFormatUInt16Vec4:
            return HioTypeUnsignedShort;

        case HioFormatInt16:
        case HioFormatInt16Vec2:
        case HioFormatInt16Vec3:
        case HioFormatInt16Vec4:
            return HioTypeSignedShort;

        case HioFormatUInt32:
        case HioFormatUInt32Vec2:
        case HioFormatUInt32Vec3:
        case HioFormatUInt32Vec4:
            return HioTypeUnsignedInt;

        case HioFormatInt32:
        case HioFormatInt32Vec2:
        case HioFormatInt32Vec3:
        case HioFormatInt32Vec4:
            return HioTypeInt;

        case HioFormatInvalid:
        case HioFormatCount:
            TF_CODING_ERROR("Unsupported HioFormat");
            return HioTypeUnsignedByte;
    }
    TF_CODING_ERROR("Missing Format");
    return HioTypeUnsignedByte;
}

int
HioGetComponentCount(HioFormat format)
{
    switch (format) {
        case HioFormatUNorm8:
        case HioFormatSNorm8:
        case HioFormatFloat16:
        case HioFormatFloat32:
        case HioFormatDouble64:
        case HioFormatUInt16:
        case HioFormatInt16:
        case HioFormatUInt32:
        case HioFormatInt32:
        case HioFormatUNorm8srgb:
            return 1;
        case HioFormatUNorm8Vec2:
        case HioFormatSNorm8Vec2:
        case HioFormatFloat16Vec2:
        case HioFormatFloat32Vec2:
        case HioFormatDouble64Vec2:
        case HioFormatUInt16Vec2:
        case HioFormatInt16Vec2:
        case HioFormatUInt32Vec2:
        case HioFormatInt32Vec2:
        case HioFormatUNorm8Vec2srgb:
            return 2;
        case HioFormatUNorm8Vec3:
        case HioFormatSNorm8Vec3:
        case HioFormatFloat16Vec3:
        case HioFormatFloat32Vec3:
        case HioFormatDouble64Vec3:
        case HioFormatUInt16Vec3:
        case HioFormatInt16Vec3:
        case HioFormatUInt32Vec3:
        case HioFormatInt32Vec3:
        case HioFormatUNorm8Vec3srgb:
        case HioFormatBC6FloatVec3:
        case HioFormatBC6UFloatVec3:
            return 3;
        case HioFormatUNorm8Vec4:
        case HioFormatSNorm8Vec4:
        case HioFormatFloat16Vec4:
        case HioFormatFloat32Vec4:
        case HioFormatDouble64Vec4:
        case HioFormatUInt16Vec4:
        case HioFormatInt16Vec4:
        case HioFormatUInt32Vec4:
        case HioFormatInt32Vec4:
        case HioFormatUNorm8Vec4srgb:
        case HioFormatBC7UNorm8Vec4:
        case HioFormatBC7UNorm8Vec4srgb:
        case HioFormatBC1UNorm8Vec4:
        case HioFormatBC3UNorm8Vec4:
            return 4;
        case HioFormatInvalid:
        case HioFormatCount:
            TF_CODING_ERROR("Unsupported format");
            return 1;
    }
    TF_CODING_ERROR("Missing Format");
    return 1;
}

size_t
HioGetDataSizeOfType(HioType type)
{
    switch (type) {
        case HioTypeCount:
            return 0;
        case HioTypeUnsignedByte:
        case HioTypeSignedByte:
        case HioTypeUnsignedByteSRGB:
            return 1;
        case HioTypeUnsignedShort:
        case HioTypeSignedShort:
        case HioTypeHalfFloat:
            return 2;
        case HioTypeUnsignedInt:
        case HioTypeInt:
        case HioTypeFloat:
            return 4;
        case HioTypeDouble:
            return 8;
    }
    TF_CODING_ERROR("Missing Format");
    return 1;
}

size_t
HioGetDataSizeOfType(HioFormat format)
{
    return HioGetDataSizeOfType(HioGetHioType(format));
}

size_t
HioGetDataSizeOfFormat(HioFormat format,
                       size_t * const blockWidth,
                       size_t * const blockHeight)
{
    if (blockWidth) {
        *blockWidth = 1;
    }
    if (blockHeight) {
        *blockHeight = 1;
    }

    switch (format) {
        case HioFormatUNorm8:
        case HioFormatSNorm8:
        case HioFormatUNorm8srgb:
            return 1;
        case HioFormatUNorm8Vec2:
        case HioFormatSNorm8Vec2:
        case HioFormatUNorm8Vec2srgb:
            return 2;
        case HioFormatUNorm8Vec3:
        case HioFormatSNorm8Vec3:
        case HioFormatUNorm8Vec3srgb:
            return 3;
        case HioFormatUNorm8Vec4:
        case HioFormatSNorm8Vec4:
        case HioFormatUNorm8Vec4srgb:
            return 4;

        case HioFormatFloat16:
        case HioFormatUInt16:
        case HioFormatInt16:
            return 2;
        case HioFormatFloat16Vec2:
        case HioFormatUInt16Vec2:
        case HioFormatInt16Vec2:
            return 4;
        case HioFormatFloat16Vec3:
        case HioFormatUInt16Vec3:
        case HioFormatInt16Vec3:
            return 6;
        case HioFormatFloat16Vec4:
        case HioFormatUInt16Vec4:
        case HioFormatInt16Vec4:
            return 8;

        case HioFormatFloat32:
        case HioFormatUInt32:
        case HioFormatInt32:
            return 4;
        case HioFormatFloat32Vec2:
        case HioFormatUInt32Vec2:
        case HioFormatInt32Vec2:
            return 8;
        case HioFormatFloat32Vec3:
        case HioFormatUInt32Vec3:
        case HioFormatInt32Vec3:
            return 12;
        case HioFormatFloat32Vec4:
        case HioFormatUInt32Vec4:
        case HioFormatInt32Vec4:
            return 16;

        case HioFormatDouble64:
            return 8;
        case HioFormatDouble64Vec2:
            return 16;
        case HioFormatDouble64Vec3:
            return 24;
        case HioFormatDouble64Vec4:
            return 32;

        case HioFormatBC6FloatVec3:
        case HioFormatBC6UFloatVec3:
        case HioFormatBC7UNorm8Vec4:
        case HioFormatBC7UNorm8Vec4srgb:
        case HioFormatBC1UNorm8Vec4:
        case HioFormatBC3UNorm8Vec4:
            if (blockWidth) {
                *blockWidth = 4;
            }
            if (blockHeight) {
                *blockHeight = 4;
            }
            return 16;
        case HioFormatInvalid:
        case HioFormatCount:
            TF_CODING_ERROR("Unsupported format");
            return 0;
    }
    TF_CODING_ERROR("Missing Format");
    return 0;
}

bool 
HioIsCompressed(HioFormat format) 
{
    switch(format) {
        case HioFormatBC6FloatVec3:
        case HioFormatBC6UFloatVec3:
        case HioFormatBC7UNorm8Vec4:
        case HioFormatBC7UNorm8Vec4srgb:
        case HioFormatBC1UNorm8Vec4:
        case HioFormatBC3UNorm8Vec4:
            return true;
        default:
            return false;
    }
}

size_t
HioGetDataSize(const HioFormat hioFormat, const GfVec3i &dimensions)
{
    size_t blockWidth, blockHeight;
    const size_t bytesPerPixel = HioGetDataSizeOfFormat(hioFormat, &blockWidth, 
                                                                &blockHeight);

    size_t numPixels = ((dimensions[0] + blockWidth  - 1) / blockWidth ) *
                       ((dimensions[1] + blockHeight - 1) / blockHeight);
    return numPixels * bytesPerPixel * std::max(1, dimensions[2]);
}

PXR_NAMESPACE_CLOSE_SCOPE
