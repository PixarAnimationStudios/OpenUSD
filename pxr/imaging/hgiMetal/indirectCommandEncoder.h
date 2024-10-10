//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_METAL_INDIRECT_COMMAND_ENCODER_H
#define PXR_IMAGING_HGI_METAL_INDIRECT_COMMAND_ENCODER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/indirectCommandEncoder.h"
#include "pxr/imaging/hgiMetal/api.h"
#include "pxr/imaging/hgiMetal/stepFunctions.h"

#import <Metal/Metal.h>

#include <map>
#include <mutex>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;
struct HgiIndirectCommands;
class HgiMetal;
class HgiMetalGraphicsPipeline;
class HgiMetalResourceBindings;
class HgiMetalVertexBufferBindings;

/// \class HgiMetalIndirectCommandEncoder
///
/// Metal implementation of Indirect Command Buffers.
///
class HgiMetalIndirectCommandEncoder final : public HgiIndirectCommandEncoder
{
public:
    HGIMETAL_API
    HgiMetalIndirectCommandEncoder(Hgi *hgi);
    
    HGIMETAL_API
    ~HgiMetalIndirectCommandEncoder() override;

    HGIMETAL_API
    HgiIndirectCommandsUniquePtr EncodeDraw(
        HgiComputeCmds * computeCmds,
        HgiGraphicsPipelineHandle const& pipeline,
        HgiResourceBindingsHandle const& resourceBindings,
        HgiVertexBufferBindingVector const& vertexBindings,
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferByteOffset,
        uint32_t drawCount,
        uint32_t stride) override;

    HGIMETAL_API
    HgiIndirectCommandsUniquePtr EncodeDrawIndexed(
        HgiComputeCmds * computeCmds,
        HgiGraphicsPipelineHandle const& pipeline,
        HgiResourceBindingsHandle const& resourceBindings,
        HgiVertexBufferBindingVector const& vertexBindings,
        HgiBufferHandle const& indexBuffer,
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferByteOffset,
        uint32_t drawCount,
        uint32_t stride,
        uint32_t patchBaseVertexByteOffset) override;

    HGIMETAL_API
    void ExecuteDraw(
        HgiGraphicsCmds * gfxCmds,
        HgiIndirectCommands const* commands) override;

private:
    HgiMetalIndirectCommandEncoder & operator=(const HgiMetalIndirectCommandEncoder&) = delete;
    HgiMetalIndirectCommandEncoder(const HgiMetalIndirectCommandEncoder&) = delete;

    struct FunctionState {
        id<MTLFunction> function;
        id<MTLComputePipelineState> pipelineState;
        id<MTLArgumentEncoder> argumentEncoder;
    };

    HGIMETAL_API
    FunctionState _GetFunction(HgiGraphicsPipelineDesc const& pipelineDesc,
                               bool isIndexed);

    HGIMETAL_API
    HgiIndirectCommandsUniquePtr _EncodeDraw(
        HgiComputeCmds * computeCmds,
        HgiGraphicsPipelineHandle const& pipeline,
        HgiResourceBindingsHandle const& resourceBindings,
        HgiVertexBufferBindingVector const &bindings,
        HgiBufferHandle const &indexBuffer,
        uint32_t drawBufferByteOffset,
        uint32_t drawCount,
        uint32_t stride,
        uint32_t patchBaseVertexByteOffset);

    HGIMETAL_API
    id<MTLIndirectCommandBuffer> _AllocateCommandBuffer(uint32_t drawCount);

    HGIMETAL_API
    id<MTLBuffer> _AllocateArgumentBuffer(uint32_t encodedLength);

    HgiMetal *_hgi;
    id<MTLDevice> _device;
    id<MTLLibrary> _library;
    std::vector<FunctionState> _functions;
    MTLResourceOptions _bufferStorageMode;
    id<MTLBuffer> _triangleTessFactors;
    id<MTLBuffer> _quadTessFactors;
    
    using FreeCommandBuffers =
        std::multimap<uint32_t, id<MTLIndirectCommandBuffer>>;
    using FreeArgumentBuffers =
        std::multimap<uint32_t, id<MTLBuffer>>;
    
    std::mutex _poolMutex;
    FreeCommandBuffers _commandBufferPool;
    FreeArgumentBuffers _argumentBufferPool;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
