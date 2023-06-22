
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
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgiDX/api.h"


PXR_NAMESPACE_OPEN_SCOPE

class HgiDXDevice;
class HgiDXShaderProgram;


///
/// \class HgiDXResourceBindings
///
/// DirectX implementation of HgiResourceBindings.
///
///
class HgiDXResourceBindings final : public HgiResourceBindings
{
public:
    HGIDX_API
    ~HgiDXResourceBindings() override;

    /// Binds the resources to GPU.
    HGIDX_API
    void BindResources();

    /// Returns the device used to create this object.
    HGIDX_API
    HgiDXDevice* GetDevice() const;

    HGIDX_API
    HgiBufferBindDesc GetBufferDesc(int nBindingIndex) const;

    HGIDX_API
    static void BindRootParams(ID3D12GraphicsCommandList* pCmdList,
                               HgiDXShaderProgram* pShaderProgram, 
                               const HgiBufferBindDescVector& bindBuffersDescs,
                               bool bCompute);

    //
    // This is never called at this time, and I am not convinced I ever need to call it.
    // TODO: review & call or delete.
    HGIDX_API
    static void UnBindRootParams(ID3D12GraphicsCommandList* pCmdList,
                                 HgiDXShaderProgram* pShaderProgram,
                                 const HgiBufferBindDescVector& bindBuffersDescs,
                                 bool bCompute);

protected:
    friend class HgiDX;

    HGIDX_API
    HgiDXResourceBindings(
        HgiDXDevice* device,
        HgiResourceBindingsDesc const& desc);

private:
    HgiDXResourceBindings() = delete;
    HgiDXResourceBindings & operator=(const HgiDXResourceBindings&) = delete;
    HgiDXResourceBindings(const HgiDXResourceBindings&) = delete;

    HgiDXDevice* _device;
};


PXR_NAMESPACE_CLOSE_SCOPE
