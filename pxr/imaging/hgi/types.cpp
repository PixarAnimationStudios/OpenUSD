//
// Copyright 2018 Pixar
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

PXR_NAMESPACE_OPEN_SCOPE

size_t HgiGetComponentCount(HgiFormat f)
{
    switch (f) {
    case HgiFormatUNorm8Vec2:
    case HgiFormatSNorm8Vec2:
    case HgiFormatFloat16Vec2:
    case HgiFormatFloat32Vec2:
    case HgiFormatInt32Vec2:
        return 2;
    case HgiFormatUNorm8Vec3:
    case HgiFormatSNorm8Vec3:
    case HgiFormatFloat16Vec3:
    case HgiFormatFloat32Vec3:
    case HgiFormatInt32Vec3:
        return 3;
    case HgiFormatUNorm8Vec4:
    case HgiFormatSNorm8Vec4:
    case HgiFormatFloat16Vec4:
    case HgiFormatFloat32Vec4:
    case HgiFormatInt32Vec4:
        return 4;
    default:
        return 1;
    }
}

size_t HgiDataSizeOfFormat(HgiFormat f)
{
    switch(f) {
    case HgiFormatUNorm8:
    case HgiFormatSNorm8:
        return 1;
    case HgiFormatUNorm8Vec2:
    case HgiFormatSNorm8Vec2:
        return 2;
    case HgiFormatUNorm8Vec3:
    case HgiFormatSNorm8Vec3:
        return 3;
    case HgiFormatUNorm8Vec4:
    case HgiFormatSNorm8Vec4:
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
        return 8;
    case HgiFormatFloat32Vec3:
    case HgiFormatInt32Vec3:
        return 12;
    case HgiFormatFloat32Vec4:
    case HgiFormatInt32Vec4:
        return 16;
    default:
        return 0;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
