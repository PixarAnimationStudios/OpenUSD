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
#include "pxr/imaging/hdSt/hioConversions.h"
#include "pxr/base/tf/iterator.h"

PXR_NAMESPACE_OPEN_SCOPE


static const HioFormat FORMAT_DESC[] =
{
    // HioFormat            // HdFormat
    HioFormatUNorm8,        // HdFormatUNorm8,
    HioFormatUNorm8Vec2,    // HdFormatUNorm8Vec2,
    HioFormatUNorm8Vec3,    // HdFormatUNorm8Vec3,
    HioFormatUNorm8Vec4,    // HdFormatUNorm8Vec4,

    HioFormatSNorm8,        // HdFormatSNorm8,
    HioFormatSNorm8Vec2,    // HdFormatSNorm8Vec2,
    HioFormatSNorm8Vec3,    // HdFormatSNorm8Vec3,
    HioFormatSNorm8Vec4,    // HdFormatSNorm8Vec4,

    HioFormatFloat16,       // HdFormatFloat16,
    HioFormatFloat16Vec2,   // HdFormatFloat16Vec2,
    HioFormatFloat16Vec3,   // HdFormatFloat16Vec3,
    HioFormatFloat16Vec4,   // HdFormatFloat16Vec4,

    HioFormatFloat32,       // HdFormatFloat32,
    HioFormatFloat32Vec2,   // HdFormatFloat32Vec2,
    HioFormatFloat32Vec3,   // HdFormatFloat32Vec3,
    HioFormatFloat32Vec4,   // HdFormatFloat32Vec4,

    HioFormatUInt16,        // HdFormatUInt16,
    HioFormatUInt16Vec2,    // HdFormatUInt16Vec2,
    HioFormatUInt16Vec3,    // HdFormatUInt16Vec3,
    HioFormatUInt16Vec4,    // HdFormatUInt16Vec4,

    HioFormatInt32,         // HdFormatInt32,
    HioFormatInt32Vec2,     // HdFormatInt32Vec2,
    HioFormatInt32Vec3,     // HdFormatInt32Vec3,
    HioFormatInt32Vec4,     // HdFormatInt32Vec4,

    HioFormatFloat32,       // HdFormatFloat32UInt8
};
static_assert(TfArraySize(FORMAT_DESC) ==
        HdFormatCount, "hioConversion FORMAT_DESC to HdFormat enum mismatch");


HioFormat
HdStHioConversions::GetHioFormat(HdFormat inFormat)
{
    if ((inFormat < 0) || (inFormat >= HdFormatCount)) {
        TF_CODING_ERROR("Unexpected HdFormat %d", inFormat);
        return HioFormatUNorm8Vec4;
    }
    return FORMAT_DESC[inFormat];
}

PXR_NAMESPACE_CLOSE_SCOPE
