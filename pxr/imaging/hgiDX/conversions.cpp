
//
// Copyright 2023 Pixar
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

#include "pch.h"
#include "pxr/imaging/hgiDX/conversions.h"

PXR_NAMESPACE_OPEN_SCOPE
using namespace Microsoft::WRL;

static const uint32_t
_FormatTable[HgiFormatCount][2] =
{
   // HGI FORMAT              VK FORMAT
   {HgiFormatUNorm8,         DXGI_FORMAT_R8_UNORM},
   {HgiFormatUNorm8Vec2,     DXGI_FORMAT_R8G8_UNORM},
   {HgiFormatUNorm8Vec4,     DXGI_FORMAT_R8G8B8A8_UNORM},
   {HgiFormatSNorm8,         DXGI_FORMAT_R8_SNORM},
   {HgiFormatSNorm8Vec2,     DXGI_FORMAT_R8G8_SNORM},
   {HgiFormatSNorm8Vec4,     DXGI_FORMAT_R8G8B8A8_SNORM},
   {HgiFormatFloat16,        DXGI_FORMAT_R16_FLOAT},
   {HgiFormatFloat16Vec2,    DXGI_FORMAT_R16G16_FLOAT},
   {HgiFormatFloat16Vec3,    DXGI_FORMAT_UNKNOWN}, // DirectX does not seem to have something like this
   {HgiFormatFloat16Vec4,    DXGI_FORMAT_R16G16B16A16_FLOAT},
   {HgiFormatFloat32,        DXGI_FORMAT_D32_FLOAT}, // It may be that when we convert we need to use more info 
   {HgiFormatFloat32Vec2,    DXGI_FORMAT_R32G32_FLOAT},
   {HgiFormatFloat32Vec3,    DXGI_FORMAT_R32G32B32_FLOAT},
   {HgiFormatFloat32Vec4,    DXGI_FORMAT_R32G32B32A32_FLOAT},
   {HgiFormatInt16,          DXGI_FORMAT_R16_SINT},
   {HgiFormatInt16Vec2,      DXGI_FORMAT_R16G16_SINT},
   {HgiFormatInt16Vec3,      DXGI_FORMAT_UNKNOWN}, // DirectX does not seem to have something like this
   {HgiFormatInt16Vec4,      DXGI_FORMAT_R16G16B16A16_SINT},
   {HgiFormatUInt16,         DXGI_FORMAT_R16_UINT},
   {HgiFormatUInt16Vec2,     DXGI_FORMAT_R16G16_UINT},
   {HgiFormatUInt16Vec3,     DXGI_FORMAT_UNKNOWN}, // DirectX does not seem to have something like this
   {HgiFormatUInt16Vec4,     DXGI_FORMAT_R16G16B16A16_UINT},
   {HgiFormatInt32,          DXGI_FORMAT_R32_SINT},
   {HgiFormatInt32Vec2,      DXGI_FORMAT_R32G32_SINT},
   {HgiFormatInt32Vec3,      DXGI_FORMAT_R32G32B32_SINT},
   {HgiFormatInt32Vec4,      DXGI_FORMAT_R32G32B32A32_SINT},
   {HgiFormatUNorm8Vec4srgb, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB},
   {HgiFormatBC6FloatVec3,   DXGI_FORMAT_UNKNOWN}, // DXGI_FORMAT_BC6H_SF16 ??
   {HgiFormatBC6UFloatVec3,  DXGI_FORMAT_UNKNOWN}, // DXGI_FORMAT_BC6H_UF16 ??
   {HgiFormatBC7UNorm8Vec4,  DXGI_FORMAT_UNKNOWN},
   {HgiFormatBC7UNorm8Vec4srgb, DXGI_FORMAT_BC7_UNORM_SRGB},
   {HgiFormatBC1UNorm8Vec4,  DXGI_FORMAT_BC1_UNORM_SRGB},
   {HgiFormatBC3UNorm8Vec4,  DXGI_FORMAT_BC3_UNORM_SRGB},
   {HgiFormatFloat32UInt8,   DXGI_FORMAT_D32_FLOAT_S8X24_UINT},
   {HgiFormatPackedInt1010102, DXGI_FORMAT_R10G10B10A2_UINT},
};


DXGI_FORMAT
HgiDXConversions::GetTextureFormat(HgiFormat hgiFormat)
{
   DXGI_FORMAT ret = DXGI_FORMAT_UNKNOWN;

   if (!TF_VERIFY(hgiFormat != HgiFormatInvalid)) {
      return DXGI_FORMAT_UNKNOWN;
   }

   if (!TF_VERIFY(hgiFormat != HgiFormatFloat16Vec3)) {
      return DXGI_FORMAT_UNKNOWN;
   }

   if (!TF_VERIFY(hgiFormat != HgiFormatUInt16Vec3)) {
      return DXGI_FORMAT_UNKNOWN;
   }

   if (!TF_VERIFY(hgiFormat != HgiFormatBC6FloatVec3)) {
      return DXGI_FORMAT_UNKNOWN;
   }

   if (!TF_VERIFY(hgiFormat != HgiFormatBC6UFloatVec3)) {
      return DXGI_FORMAT_UNKNOWN;
   }

   if (!TF_VERIFY(hgiFormat != HgiFormatBC7UNorm8Vec4)) {
      return DXGI_FORMAT_UNKNOWN;
   }

   return DXGI_FORMAT(_FormatTable[hgiFormat][1]);
}

std::wstring 
HgiDXConversions::GetWideString(const std::string& str)
{
   std::wstring ret;
   size_t size = str.length();

   wchar_t* pwc = new wchar_t[size + 1];
   mbstowcs_s(&size, pwc, size + 1, str.c_str(), size);
   ret = pwc;
   delete pwc;

   return ret;
}

namespace {
   std::map<std::string, DXGI_FORMAT> type2DXFormat{
      { "vec4", DXGI_FORMAT_R32G32B32A32_FLOAT},
      { "ivec4", DXGI_FORMAT_R32G32B32A32_SINT },
      { "uvec4", DXGI_FORMAT_R32G32B32A32_UINT },
      { "vec3", DXGI_FORMAT_R32G32B32_FLOAT },
      { "ivec3", DXGI_FORMAT_R32G32B32_SINT },
      { "uvec3", DXGI_FORMAT_R32G32B32_UINT },
      { "vec2", DXGI_FORMAT_R32G32_FLOAT },
      { "ivec2", DXGI_FORMAT_R32G32_SINT },
      { "int", DXGI_FORMAT_R32_SINT },
      { "uint", DXGI_FORMAT_R32_UINT },
      { "float", DXGI_FORMAT_R32_FLOAT },
      { "bool", DXGI_FORMAT_R8_UINT },
   };
}

DXGI_FORMAT
HgiDXConversions::ParamType2DXFormat(std::string strParamType)
{
   DXGI_FORMAT ret = DXGI_FORMAT_UNKNOWN;

   if (type2DXFormat.find(strParamType) == type2DXFormat.end())
   {
      TF_WARN("Failed to translate input parameter type to DX type: %s", strParamType.c_str());
   }
   else
      ret = type2DXFormat[strParamType];

   return ret;
}


PXR_NAMESPACE_CLOSE_SCOPE