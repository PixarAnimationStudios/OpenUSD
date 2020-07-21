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

PXR_NAMESPACE_OPEN_SCOPE

size_t HgiGetComponentCount(const HgiFormat f)
{
    switch (f) {
    case HgiFormatUNorm8:
    case HgiFormatSNorm8:
    case HgiFormatFloat16:
    case HgiFormatFloat32:
    case HgiFormatInt32:
    case HgiFormatFloat32UInt8: // treat as a single component
        return 1;
    case HgiFormatUNorm8Vec2:
    case HgiFormatSNorm8Vec2:
    case HgiFormatFloat16Vec2:
    case HgiFormatFloat32Vec2:
    case HgiFormatInt32Vec2:
        return 2;
    // case HgiFormatUNorm8Vec3: // Unsupported Metal (MTLPixelFormat)
    // case HgiFormatSNorm8Vec3: // Unsupported Metal (MTLPixelFormat)
    case HgiFormatFloat16Vec3:
    case HgiFormatFloat32Vec3:
    case HgiFormatInt32Vec3:
    case HgiFormatBC6FloatVec3:
    case HgiFormatBC6UFloatVec3:
        return 3;
    case HgiFormatUNorm8Vec4:
    case HgiFormatSNorm8Vec4:
    case HgiFormatFloat16Vec4:
    case HgiFormatFloat32Vec4:
    case HgiFormatInt32Vec4:
    case HgiFormatUNorm8Vec4srgb:
        return 4;
    case HgiFormatCount:
    case HgiFormatInvalid:
        TF_CODING_ERROR("Invalid Format");
        return 0;
    }
    TF_CODING_ERROR("Missing Format");
    return 0;
}

size_t HgiDataSizeOfFormat(const HgiFormat f)
{
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
        return 2;
    case HgiFormatFloat16Vec2:
        return 4;
    case HgiFormatFloat16Vec3:
        return 6;
    case HgiFormatFloat16Vec4:
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
        return 1;
    case HgiFormatCount:
    case HgiFormatInvalid:
        TF_CODING_ERROR("Invalid Format");
        return 0;
    }
    TF_CODING_ERROR("Missing Format");
    return 0;
}

bool HgiIsCompressed(const HgiFormat f)
{
    switch(f) {
    case HgiFormatBC6FloatVec3:
    case HgiFormatBC6UFloatVec3:
        return true;
    default:
        return false;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
