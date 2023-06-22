
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
#include "pxr/imaging/hgi/blitCmds.h"

PXR_NAMESPACE_OPEN_SCOPE

class HgiDX;
class HgiDXCommandBuffer;


/// \class HgiDXBlitCmds
///
/// DirectX implementation of HgiBlitCmds.
///
class HgiDXBlitCmds final : public HgiBlitCmds
{
public:
   HGIDX_API
   ~HgiDXBlitCmds() override;

   HGIDX_API
   void PushDebugGroup(const char* label) override;

   HGIDX_API
   void PopDebugGroup() override;

   HGIDX_API
   void CopyTextureGpuToCpu(HgiTextureGpuToCpuOp const& copyOp) override;

   HGIDX_API
   void CopyTextureCpuToGpu(HgiTextureCpuToGpuOp const& copyOp) override;

   HGIDX_API
    void CopyBufferGpuToGpu(HgiBufferGpuToGpuOp const& copyOp) override;

   HGIDX_API
   void CopyBufferCpuToGpu(HgiBufferCpuToGpuOp const& copyOp) override;

   HGIDX_API
   void CopyBufferGpuToCpu(HgiBufferGpuToCpuOp const& copyOp) override;

   HGIDX_API
   void CopyTextureToBuffer(HgiTextureToBufferOp const& copyOp) override;

   HGIDX_API
   void CopyBufferToTexture(HgiBufferToTextureOp const& copyOp) override;

   HGIDX_API
   void GenerateMipMaps(HgiTextureHandle const& texture) override;

   HGIDX_API
   void FillBuffer(HgiBufferHandle const& buffer, uint8_t value) override;

   HGIDX_API
   void InsertMemoryBarrier(HgiMemoryBarrier barrier) override;

protected:
   friend class HgiDX;

   HGIDX_API
   HgiDXBlitCmds(HgiDX* hgi);
   
   bool _Submit(Hgi* hgi, HgiSubmitWaitType wait) override;
   

private:
   HgiDXBlitCmds& operator=(const HgiDXBlitCmds&) = delete;
   HgiDXBlitCmds(const HgiDXBlitCmds&) = delete;

   HgiDX* _hgi;
};

PXR_NAMESPACE_CLOSE_SCOPE
