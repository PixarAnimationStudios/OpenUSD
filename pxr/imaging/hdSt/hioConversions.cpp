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

struct _FormatDesc {
    HdFormat hdFormat;
    HioFormat hioFormat;
};

static const _FormatDesc FORMAT_DESC[] =
{
    {HdFormatUNorm8,         HioFormatUNorm8},
    {HdFormatUNorm8Vec2,     HioFormatUNorm8Vec2},
    {HdFormatUNorm8Vec3,     HioFormatUNorm8Vec3},
    {HdFormatUNorm8Vec4,     HioFormatUNorm8Vec4},

    {HdFormatSNorm8,         HioFormatSNorm8},
    {HdFormatSNorm8Vec2,     HioFormatSNorm8Vec2},
    {HdFormatSNorm8Vec3,     HioFormatSNorm8Vec3},
    {HdFormatSNorm8Vec4,     HioFormatSNorm8Vec4},

    {HdFormatFloat16,        HioFormatFloat16},
    {HdFormatFloat16Vec2,    HioFormatFloat16Vec2},
    {HdFormatFloat16Vec3,    HioFormatFloat16Vec3},
    {HdFormatFloat16Vec4,    HioFormatFloat16Vec4},

    {HdFormatFloat32,        HioFormatFloat32},
    {HdFormatFloat32Vec2,    HioFormatFloat32Vec2},
    {HdFormatFloat32Vec3,    HioFormatFloat32Vec3},
    {HdFormatFloat32Vec4,    HioFormatFloat32Vec4},

    {HdFormatInt16,          HioFormatInt16},
    {HdFormatInt16Vec2,      HioFormatInt16Vec2},
    {HdFormatInt16Vec3,      HioFormatInt16Vec3},
    {HdFormatInt16Vec4,      HioFormatInt16Vec4},

    {HdFormatUInt16,         HioFormatUInt16},
    {HdFormatUInt16Vec2,     HioFormatUInt16Vec2},
    {HdFormatUInt16Vec3,     HioFormatUInt16Vec3},
    {HdFormatUInt16Vec4,     HioFormatUInt16Vec4},

    {HdFormatInt32,          HioFormatInt32},
    {HdFormatInt32Vec2,      HioFormatInt32Vec2},
    {HdFormatInt32Vec3,      HioFormatInt32Vec3},
    {HdFormatInt32Vec4,      HioFormatInt32Vec4},

    {HdFormatFloat32UInt8,   HioFormatFloat32}
};
static_assert(TfArraySize(FORMAT_DESC) == HdFormatCount,
        "_FormatDesc array out of sync with HdFormat/HioFormat enum");


HioFormat
HdStHioConversions::GetHioFormat(HdFormat hdFormat)
{
    if ((hdFormat < 0) || (hdFormat >= HdFormatCount)) {
        TF_CODING_ERROR("Unexpected HdFormat %d", hdFormat);
        return HioFormatInvalid;
    }
    return FORMAT_DESC[hdFormat].hioFormat;
}

HdFormat
HdStHioConversions::GetHdFormat(HioFormat hioFormat)
{
    if ((hioFormat < 0) || (hioFormat >= HioFormatCount)) {
        TF_CODING_ERROR("Unexpected HioFormat %d", hioFormat);
        return HdFormatInvalid;
    }

    for (size_t i = 0; i < HdFormatCount; i++) {
        if (FORMAT_DESC[i].hioFormat == hioFormat) {
            return FORMAT_DESC[i].hdFormat;
        }
    }

    TF_CODING_ERROR("Unmapped HioFormat %d", hioFormat);
    return HdFormatInvalid;
}

PXR_NAMESPACE_CLOSE_SCOPE
