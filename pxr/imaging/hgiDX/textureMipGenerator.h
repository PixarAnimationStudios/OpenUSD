
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
#include "pxr/imaging/hgiDX/api.h"
#include "pxr/imaging/hgi/handle.h"

PXR_NAMESPACE_OPEN_SCOPE

class HgiDXBuffer;
class HgiShaderFunction;
struct HgiShaderFunctionDesc;
class HgiShaderProgram;
class HgiDXTexture;


/// \class HgiDXComputePipeline
///
/// DirectX implementation of functionality 
/// that generates mips (other than 0).
///
class HgiDXTextureMipGenerator final
{
private:
    HGIDX_API
    HgiDXTextureMipGenerator(class HgiDX* pHgi);
   
public:
    
    HGIDX_API
    ~HgiDXTextureMipGenerator();

    void generate(HgiDXTexture* pTx);
    
private:
   
    void _initialize(DXGI_FORMAT);
    bool _buildPSO();

    HgiDXTextureMipGenerator& operator=(const HgiDXTextureMipGenerator&) = delete;
    HgiDXTextureMipGenerator(const HgiDXTextureMipGenerator&) = delete;


private:
    HgiDX* _pHgi = nullptr;
    // Root signature
    Microsoft::WRL::ComPtr<ID3D12RootSignature> _rootSignature;

    // Pipeline state object.
    Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineState;
};


PXR_NAMESPACE_CLOSE_SCOPE