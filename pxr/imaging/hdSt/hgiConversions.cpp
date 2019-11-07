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

struct _FormatDesc {
    HdFormat hdFormat;
    HgiFormat hgiFormat;
};

static const _FormatDesc FORMAT_DESC[] =
{
    {HdFormatUNorm8,     HgiFormatUNorm8}, 
    {HdFormatUNorm8Vec2, HgiFormatUNorm8Vec2}, 
    {HdFormatUNorm8Vec3, HgiFormatUNorm8Vec3}, 
    {HdFormatUNorm8Vec4, HgiFormatUNorm8Vec4}, 

    {HdFormatSNorm8,     HgiFormatSNorm8}, 
    {HdFormatSNorm8Vec2, HgiFormatSNorm8Vec2}, 
    {HdFormatSNorm8Vec3, HgiFormatSNorm8Vec3}, 
    {HdFormatSNorm8Vec4, HgiFormatSNorm8Vec4}, 

    {HdFormatFloat16,     HgiFormatFloat16}, 
    {HdFormatFloat16Vec2, HgiFormatFloat16Vec2}, 
    {HdFormatFloat16Vec3, HgiFormatFloat16Vec3}, 
    {HdFormatFloat16Vec4, HgiFormatFloat16Vec4}, 

    {HdFormatFloat32,     HgiFormatFloat32}, 
    {HdFormatFloat32Vec2, HgiFormatFloat32Vec2}, 
    {HdFormatFloat32Vec3, HgiFormatFloat32Vec3}, 
    {HdFormatFloat32Vec4, HgiFormatFloat32Vec4}, 

    {HdFormatInt32,     HgiFormatInt32}, 
    {HdFormatInt32Vec2, HgiFormatInt32Vec2}, 
    {HdFormatInt32Vec3, HgiFormatInt32Vec3}, 
    {HdFormatInt32Vec4, HgiFormatInt32Vec4}, 
};

constexpr bool _CompileTimeValidateFormatTable() {
    return (TfArraySize(FORMAT_DESC) == HdFormatCount &&
            TfArraySize(FORMAT_DESC) == HgiFormatCount &&
            HdFormatUNorm8 == 0 && HgiFormatUNorm8 == 0 &&
            HdFormatFloat16Vec4 == 11 && HgiFormatFloat16Vec4 == 11 &&
            HdFormatFloat32Vec4 == 15 && HgiFormatFloat32Vec4 == 15 &&
            HdFormatInt32Vec4 == 19 && HgiFormatInt32Vec4 == 19) ? true : false;
}

static_assert(_CompileTimeValidateFormatTable(), 
              "_FormatDesc array out of sync with HdFormat/HgiFormat enum");

HgiFormat
HdStHgiConversions::GetHgiFormat(HdFormat hdFormat)
{
    if ((hdFormat < 0) || (hdFormat >= HdFormatCount))
    {
        TF_CODING_ERROR("Unexpected HdFormat %d", hdFormat);
        return HgiFormatInvalid;
    }

    return FORMAT_DESC[hdFormat].hgiFormat;
}

PXR_NAMESPACE_CLOSE_SCOPE
