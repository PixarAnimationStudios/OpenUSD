//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/hgiConversions.h"
#include "pxr/base/tf/iterator.h"

PXR_NAMESPACE_OPEN_SCOPE

struct _FormatDesc {
    HdFormat hdFormat;
    HgiFormat hgiFormat;
};

static const _FormatDesc FORMAT_DESC[] =
{
    {HdFormatUNorm8,     HgiFormatUNorm8}, 
    {HdFormatUNorm8Vec2, HgiFormatUNorm8Vec2}, 
    {HdFormatUNorm8Vec3, HgiFormatInvalid}, // Unsupported by HgiFormat 
    {HdFormatUNorm8Vec4, HgiFormatUNorm8Vec4}, 

    {HdFormatSNorm8,     HgiFormatSNorm8}, 
    {HdFormatSNorm8Vec2, HgiFormatSNorm8Vec2}, 
    {HdFormatSNorm8Vec3, HgiFormatInvalid}, // Unsupported by HgiFormat 
    {HdFormatSNorm8Vec4, HgiFormatSNorm8Vec4}, 

    {HdFormatFloat16,     HgiFormatFloat16}, 
    {HdFormatFloat16Vec2, HgiFormatFloat16Vec2}, 
    {HdFormatFloat16Vec3, HgiFormatFloat16Vec3}, 
    {HdFormatFloat16Vec4, HgiFormatFloat16Vec4}, 

    {HdFormatFloat32,     HgiFormatFloat32}, 
    {HdFormatFloat32Vec2, HgiFormatFloat32Vec2}, 
    {HdFormatFloat32Vec3, HgiFormatFloat32Vec3}, 
    {HdFormatFloat32Vec4, HgiFormatFloat32Vec4}, 

    {HdFormatInt16,      HgiFormatInt16},
    {HdFormatInt16Vec2,  HgiFormatInt16Vec2},
    {HdFormatInt16Vec3,  HgiFormatInt16Vec3},
    {HdFormatInt16Vec4,  HgiFormatInt16Vec4},

    {HdFormatUInt16,     HgiFormatUInt16},
    {HdFormatUInt16Vec2, HgiFormatUInt16Vec2},
    {HdFormatUInt16Vec3, HgiFormatUInt16Vec3},
    {HdFormatUInt16Vec4, HgiFormatUInt16Vec4},

    {HdFormatInt32,     HgiFormatInt32}, 
    {HdFormatInt32Vec2, HgiFormatInt32Vec2}, 
    {HdFormatInt32Vec3, HgiFormatInt32Vec3}, 
    {HdFormatInt32Vec4, HgiFormatInt32Vec4},

    {HdFormatFloat32UInt8, HgiFormatFloat32UInt8},
};

// A few random format validations to make sure that the format conversion
// table stays up-to-date with changes to HdFormat and HgiFormat.
constexpr bool _CompileTimeValidateFormatTable() {
    return (HdFormatCount == 29 &&
            HdFormatUNorm8 == 0 && HgiFormatUNorm8 == 0 &&
            HdFormatFloat16Vec4 == 11 && HgiFormatFloat16Vec4 == 9 &&
            HdFormatFloat32Vec4 == 15 && HgiFormatFloat32Vec4 == 13 &&
            HdFormatUInt16Vec4 == 23 && HgiFormatUInt16Vec4 == 21 &&
            HdFormatInt32Vec4 == 27 && HgiFormatInt32Vec4 == 25) ? true : false;
}

static_assert(_CompileTimeValidateFormatTable(), 
              "_FormatDesc array out of sync with HdFormat/HgiFormat enum");

HgiFormat
HdxHgiConversions::GetHgiFormat(HdFormat hdFormat)
{
    if ((hdFormat < 0) || (hdFormat >= HdFormatCount))
    {
        TF_CODING_ERROR("Unexpected HdFormat %d", hdFormat);
        return HgiFormatInvalid;
    }

    return FORMAT_DESC[hdFormat].hgiFormat;
}

HdFormat
HdxHgiConversions::GetHdFormat(HgiFormat hgiFormat)
{
    if ((hgiFormat < 0) || (hgiFormat >= HgiFormatCount))
    {
        TF_CODING_ERROR("Unexpected HgiFormat %d", hgiFormat);
        return HdFormatInvalid;
    }

    for (size_t i = 0; i < HdFormatCount; i++) {
        if (FORMAT_DESC[i].hgiFormat == hgiFormat) {
            return HdFormat(i);
        }
    }
    
    TF_CODING_ERROR("Unmapped HgiFormat %d", hgiFormat);
    return HdFormatInvalid;
}

PXR_NAMESPACE_CLOSE_SCOPE
