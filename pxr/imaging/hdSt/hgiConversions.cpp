//
// Copyright 2016 Pixar
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
#include "pxr/imaging/hdSt/hgiConversions.h"
#include "pxr/base/tf/iterator.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

struct _FormatDesc {
    HdFormat hdFormat;
    HgiFormat hgiFormat;
};

const _FormatDesc FORMAT_DESC[] =
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
    return
        HdFormatCount == 25 &&
        HdFormatUNorm8 == 0 && HgiFormatUNorm8 == 0 &&
        HdFormatFloat16Vec4 == 11 && HgiFormatFloat16Vec4 == 9 &&
        HdFormatFloat32Vec4 == 15 && HgiFormatFloat32Vec4 == 13 &&
        HdFormatUInt16Vec4 == 19 && HgiFormatUInt16Vec4 == 17 &&
        HdFormatInt32Vec4 == 23 && HgiFormatInt32Vec4 == 21;
}

static_assert(_CompileTimeValidateFormatTable(), 
              "_FormatDesc array out of sync with HdFormat/HgiFormat enum");

struct _WrapDesc {
    HdWrap hdWrap;
    HgiSamplerAddressMode hgiSamplerAddressMode;
};

const _WrapDesc WRAP_DESC[] =
{
    {HdWrapClamp,           HgiSamplerAddressModeClampToEdge},
    {HdWrapRepeat,          HgiSamplerAddressModeRepeat},
    {HdWrapBlack,           HgiSamplerAddressModeClampToBorderColor},
    {HdWrapMirror,          HgiSamplerAddressModeMirrorRepeat},
    {HdWrapNoOpinion,       HgiSamplerAddressModeClampToBorderColor},
    {HdWrapLegacyNoOpinionFallbackRepeat, HgiSamplerAddressModeRepeat}
};

constexpr bool _CompileTimeValidateWrapTable() {
    return
        HdWrapClamp == 0 &&
        HdWrapLegacyNoOpinionFallbackRepeat == 5;
}

static_assert(_CompileTimeValidateWrapTable(),
              "_WrapDesc array out of sync with HdWrap/HgiSamplerAddressMode");

struct _MagDesc {
    HdMagFilter hdMagFilter;
    HgiSamplerFilter hgiSamplerFilter;
};

const _MagDesc MAG_DESC[] =
{
    {HdMagFilterNearest, HgiSamplerFilterNearest},
    {HdMagFilterLinear,  HgiSamplerFilterLinear}
};

constexpr bool _CompileTimeValidateMagTable() {
    return
        HdMagFilterNearest == 0 &&
        HdMagFilterLinear == 1;
}

static_assert(_CompileTimeValidateMagTable(),
              "_MagDesc array out of sync with HdMagFilter");

struct _MinDesc {
    HdMinFilter hdMinFilter;
    HgiSamplerFilter hgiSamplerFilter;
    HgiMipFilter hgiMipFilter;
};

const _MinDesc MIN_DESC[] =
{
    {HdMinFilterNearest,
     HgiSamplerFilterNearest, HgiMipFilterNotMipmapped},
    {HdMinFilterLinear,
     HgiSamplerFilterLinear,  HgiMipFilterNotMipmapped},
    {HdMinFilterNearestMipmapNearest,
     HgiSamplerFilterNearest, HgiMipFilterNearest},
    {HdMinFilterLinearMipmapNearest,
     HgiSamplerFilterLinear,  HgiMipFilterNearest},
    {HdMinFilterNearestMipmapLinear,
     HgiSamplerFilterNearest, HgiMipFilterLinear},
    {HdMinFilterLinearMipmapLinear,
     HgiSamplerFilterLinear,  HgiMipFilterLinear}
};

constexpr bool _CompileTimeValidateMinTable() {
    return
        HdMinFilterNearest == 0 &&
        HdMinFilterLinear == 1 &&
        HdMinFilterNearestMipmapNearest == 2 &&
        HdMinFilterLinearMipmapLinear == 5;
}

static_assert(_CompileTimeValidateMinTable(),
              "_MinDesc array out of sync with HdMinFilter");

}

HgiFormat
HdStHgiConversions::GetHgiFormat(const HdFormat hdFormat)
{
    if ((hdFormat < 0) || (hdFormat >= HdFormatCount))
    {
        TF_CODING_ERROR("Unexpected HdFormat %d", hdFormat);
        return HgiFormatInvalid;
    }

    HgiFormat hgiFormat = FORMAT_DESC[hdFormat].hgiFormat;
    if (ARCH_UNLIKELY(hgiFormat == HgiFormatInvalid)) {
        TF_CODING_ERROR("Unsupported format");
    }

    return hgiFormat;
}

HgiSamplerAddressMode
HdStHgiConversions::GetHgiSamplerAddressMode(const HdWrap hdWrap)
{
    if ((hdWrap < 0) || (hdWrap > HdWrapLegacyNoOpinionFallbackRepeat))
    {
        TF_CODING_ERROR("Unexpected HdWrap %d", hdWrap);
        return HgiSamplerAddressModeClampToBorderColor;
    }

    return WRAP_DESC[hdWrap].hgiSamplerAddressMode;
}

HgiSamplerFilter
HdStHgiConversions::GetHgiMagFilter(const HdMagFilter hdMagFilter)
{
    if ((hdMagFilter < 0) || (hdMagFilter > HdMagFilterLinear)) {
        TF_CODING_ERROR("Unexpected HdMagFilter %d", hdMagFilter);
        return HgiSamplerFilterLinear;
    }
    return MAG_DESC[hdMagFilter].hgiSamplerFilter;
}

void
HdStHgiConversions::GetHgiMinAndMipFilter(
    const HdMinFilter hdMinFilter,
    HgiSamplerFilter * const hgiSamplerFilter,
    HgiMipFilter * const hgiMipFilter)
{
    if ((hdMinFilter < 0) || (hdMinFilter > HdMinFilterLinearMipmapLinear)) {
        TF_CODING_ERROR("Unexpected HdMinFilter %d", hdMinFilter);
        *hgiSamplerFilter = HgiSamplerFilterLinear;
        *hgiMipFilter = HgiMipFilterNotMipmapped;
    }
    *hgiSamplerFilter = MIN_DESC[hdMinFilter].hgiSamplerFilter;
    *hgiMipFilter     = MIN_DESC[hdMinFilter].hgiMipFilter;
}

PXR_NAMESPACE_CLOSE_SCOPE
