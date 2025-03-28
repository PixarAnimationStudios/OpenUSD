//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_METAL_RESOURCEBINDINGS_H
#define PXR_IMAGING_HGI_METAL_RESOURCEBINDINGS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgiMetal/api.h"

#include <Metal/Metal.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Fixed indexes within the argument buffer for resource types.
/// Chosen to be at the top of the range of indexs to not interfere
/// with the vertex attributes.
enum HgiMetalArgumentIndex {
    HgiMetalArgumentIndexICB = 26,
    HgiMetalArgumentIndexConstants = 27,
    HgiMetalArgumentIndexSamplers = 28,
    HgiMetalArgumentIndexTextures = 29,
    HgiMetalArgumentIndexBuffers = 30,
};

enum HgiMetalArgumentOffset {
    HgiMetalArgumentOffsetBufferVS = 0,
    HgiMetalArgumentOffsetBufferFS = 512,
    HgiMetalArgumentOffsetSamplerVS = 1024,
    HgiMetalArgumentOffsetSamplerFS = 1536,
    HgiMetalArgumentOffsetTextureVS = 2048,
    HgiMetalArgumentOffsetTextureFS = 2560,

    HgiMetalArgumentOffsetBufferCS = 0,
    HgiMetalArgumentOffsetSamplerCS = 1024,
    HgiMetalArgumentOffsetTextureCS = 2048,
    
    HgiMetalArgumentOffsetConstants = 3072,
    
    HgiMetalArgumentOffsetSize = 4096
};


class HgiMetal;

///
/// \class HgiMetalResourceBindings
///
/// Metal implementation of HgiResourceBindings.
///
///
class HgiMetalResourceBindings final : public HgiResourceBindings
{
public:
    HGIMETAL_API
    HgiMetalResourceBindings(HgiResourceBindingsDesc const& desc);

    HGIMETAL_API
    ~HgiMetalResourceBindings() override;

    /// Binds the resources to GPU.
    HGIMETAL_API
    void BindResources(HgiMetal *hgi,
                       id<MTLRenderCommandEncoder> renderEncoder,
                       id<MTLBuffer> argBuffer);

    HGIMETAL_API
    void BindResources(HgiMetal *hgi,
                       id<MTLComputeCommandEncoder> computeEncoder,
                       id<MTLBuffer> argBuffer);
    
    HGIMETAL_API
    static void SetConstantValues(
        id<MTLBuffer> argumentBuffer,
        HgiShaderStage stages,
        uint32_t bindIndex,
        uint32_t byteSize,
        const void* data);

private:
    HgiMetalResourceBindings() = delete;
    HgiMetalResourceBindings & operator=(const HgiMetalResourceBindings&) = delete;
    HgiMetalResourceBindings(const HgiMetalResourceBindings&) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
