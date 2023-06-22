
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
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"

#include "pxr/imaging/hgiDX/capabilities.h"
#include "pxr/imaging/hgiDX/device.h"


PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HGI_DX_INDIRECT_DRAW, true, "Enable indirect draw.");

HgiDXCapabilities::HgiDXCapabilities(HgiDXDevice* device)
    : supportsTimeStamps(false)
   , _d3dMinFeatureLevel(D3D_FEATURE_LEVEL_11_0)
{

   // Determine maximum supported feature level for this device
   static const D3D_FEATURE_LEVEL s_featureLevels[] =
   {
#if defined(NTDDI_WIN10_FE) || defined(USING_D3D12_AGILITY_SDK)
        D3D_FEATURE_LEVEL_12_2,
#endif
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
   };

   D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels =
   {
       static_cast<UINT>(std::size(s_featureLevels)), s_featureLevels, D3D_FEATURE_LEVEL_11_0
   };

   HRESULT hr = device->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
   if (SUCCEEDED(hr))
   {
      _d3dFeatureLevel = featLevels.MaxSupportedFeatureLevel;
   }
   else
   {
      _d3dFeatureLevel = _d3dMinFeatureLevel;
   }

   //
   // TODO: check what each of the following do, what their effects are and set them 
   // to match both the reality and our needs...
   // For the moment I set them according to various empirical observations and 
   // mainly according to what was working well for Vulkan Hgi

   //
   // multisample quality levels can tell us the quality range we can use when setting up textures
   // device->GetDevice()->CheckMultisampleQualityLevels()

   //_maxClipDistances = vkDeviceProperties.limits.maxClipDistances;
   
   const bool conservativeRasterEnabled = false;
   
   _SetFlag(HgiDeviceCapabilitiesBitsDepthRangeMinusOnetoOne, false);
   _SetFlag(HgiDeviceCapabilitiesBitsConservativeRaster, conservativeRasterEnabled);
   _SetFlag(HgiDeviceCapabilitiesBitsStencilReadback, true);

   //
   // TODO: for the moment I want to have this disabled,
   // but I need to learn more about it and see if DX can do something better in some context
   _SetFlag(HgiDeviceCapabilitiesBitsBuiltinBarycentrics, false);


   //
   // I want to be able to control this from outside easily so I can test one way or the other...
   bool bIndirectDraw = TfGetEnvSetting(HGI_DX_INDIRECT_DRAW);
   _SetFlag(HgiDeviceCapabilitiesBitsMultiDrawIndirect, bIndirectDraw);

   //
   // This flag seems to make things worse for DX, so let's disable it
   _SetFlag(HgiDeviceCapabilitiesBitsCppShaderPadding, false);

   //
   // TODO: review this data and make sure it is correct, maybe even optimal
   // also, what else could we setup here that would impact workflows and / or performance?
   _maxUniformBlockSize = 64 * 1024;
   _maxShaderStorageBlockSize = 1 * 1024 * 1024 * 1024;

   _uniformBufferOffsetAlignment = 256;
   _pageSizeAlignment = 4096;
}

HgiDXCapabilities::~HgiDXCapabilities() = default;

int
HgiDXCapabilities::GetAPIVersion() const
{
	//
	// TODO: review what this means to HdSt
   // I could easily set a good value here, but what is it used for?
   return 0;
}

int
HgiDXCapabilities::GetShaderVersion() const
{
   //
   // TODO: review what this means to HdSt
   // I could easily set a good value here, but what is it used for?
   return 450;
}

PXR_NAMESPACE_CLOSE_SCOPE
