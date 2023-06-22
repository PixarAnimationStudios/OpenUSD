
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

#pragma once

#include "pxr/pxr.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgi/graphicsPipeline.h"
#include "pxr/imaging/hgiDX/api.h"
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE

class HgiDXDevice;


/// \class HgiDXPipeline
///
/// DirectX implementation of HgiGraphicsPipeline.
///
class HgiDXGraphicsPipeline final : public HgiGraphicsPipeline
{
public:
   HGIDX_API
   ~HgiDXGraphicsPipeline() override;

   /// Apply pipeline state
   HGIDX_API
   void BindPipeline();

   ID3D12CommandSignature* GetIndirectCommandSignature(uint32_t stride);

protected:
   friend class HgiDX;

   HGIDX_API
   HgiDXGraphicsPipeline(HgiDXDevice* device, HgiGraphicsPipelineDesc const& desc);

private:
   HgiDXGraphicsPipeline() = delete;
   HgiDXGraphicsPipeline& operator=(const HgiDXGraphicsPipeline&) = delete;
   HgiDXGraphicsPipeline(const HgiDXGraphicsPipeline&) = delete;

   D3D12_PRIMITIVE_TOPOLOGY_TYPE _GetTopologyType();
   D3D12_PRIMITIVE_TOPOLOGY _GetTopology();
   D3D12_COMPARISON_FUNC _GetDepthCompareFc(HgiCompareFunction fc);

private:

   HgiDXDevice* _device;

   // Root signature
   Microsoft::WRL::ComPtr<ID3D12RootSignature> _rootSignature;

   // Pipeline state object.
   Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineState;

   //
   // Indirect Command signature
   uint32_t _indirectArgumentStride;
   Microsoft::WRL::ComPtr<ID3D12CommandSignature> _indirectCommandSignature;
};


PXR_NAMESPACE_CLOSE_SCOPE
