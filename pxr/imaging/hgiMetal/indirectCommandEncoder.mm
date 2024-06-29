//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/arch/defines.h"

#include "pxr/imaging/hgiMetal/indirectCommandEncoder.h"
#include "pxr/imaging/hgiMetal/buffer.h"
#include "pxr/imaging/hgiMetal/capabilities.h"
#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/computeCmds.h"
#include "pxr/imaging/hgiMetal/graphicsCmds.h"
#include "pxr/imaging/hgiMetal/graphicsPipeline.h"
#include "pxr/imaging/hgiMetal/hgi.h"
#include "pxr/imaging/hgiMetal/resourceBindings.h"
#include "pxr/imaging/hgiMetal/stepFunctions.h"

PXR_NAMESPACE_OPEN_SCOPE

static const uint32_t MaxVertexBufferBindings = 64;
static const uint32_t MaxStepFunctions = 16;
static const uint32_t StepFunctionSize = 3;
static const uint32_t StepFunctionsArraySize = MaxStepFunctions * StepFunctionSize;

struct HgiMetalIndirectCommands : public HgiIndirectCommands
{
    HgiMetalIndirectCommands(
        uint32_t drawCount,
        HgiGraphicsPipelineHandle const &graphicsPipeline,
        HgiResourceBindingsHandle const &resourceBindings,
        id<MTLIndirectCommandBuffer> indirectCommandBuffer,
        id<MTLBuffer> argumentBuffer,
        id<MTLBuffer> mainArgumentBuffer)
        : HgiIndirectCommands(drawCount, graphicsPipeline, resourceBindings)
        , indirectCommandBuffer(indirectCommandBuffer)
        , indirectArgumentBuffer(argumentBuffer)
        , mainArgumentBuffer(mainArgumentBuffer)
    {
    }
    
    id<MTLIndirectCommandBuffer> indirectCommandBuffer;
    id<MTLBuffer> indirectArgumentBuffer;
    id<MTLBuffer> mainArgumentBuffer;
};

enum ArgIndex
{
    ArgIndex_ICB,
    ArgIndex_IndexBuffer,
    ArgIndex_MainArgumentBuffer,
    ArgIndex_PatchFactorsBuffer,
    ArgIndex_PrimitiveType,
    ArgIndex_DrawStepFunction,
    ArgIndex_NumPatchStepFuncs = ArgIndex_DrawStepFunction + StepFunctionSize,
    ArgIndex_PatchStepFunctions,
    ArgIndex_NumControlPoints = ArgIndex_PatchStepFunctions + StepFunctionsArraySize,
    ArgIndex_PatchBaseVertexByteOffset,
    ArgIndex_NumBuffers,
    ArgIndex_Buffers,
};

HgiMetalIndirectCommandEncoder::HgiMetalIndirectCommandEncoder(Hgi* hgi)
    : HgiIndirectCommandEncoder()
    , _device(nil)
    , _library(nil)
    , _triangleTessFactors(nil)
    , _quadTessFactors(nil)
{
    _hgi = static_cast<HgiMetal*>(hgi);
    _device = _hgi->GetPrimaryDevice();
    _bufferStorageMode = _hgi->GetCapabilities()->defaultStorageMode;
    
    uint16_t const factorZero =
            reinterpret_cast<uint16_t>(GfHalf(0.0f).bits());
    uint16_t const factorOne =
            reinterpret_cast<uint16_t>(GfHalf(1.0f).bits());

    MTLTriangleTessellationFactorsHalf triangleFactors;
    triangleFactors.insideTessellationFactor = factorZero;
    triangleFactors.edgeTessellationFactor[0] = factorOne;
    triangleFactors.edgeTessellationFactor[1] = factorOne;
    triangleFactors.edgeTessellationFactor[2] = factorOne;
    _triangleTessFactors =
        [_device
            newBufferWithBytes:&triangleFactors
                        length:sizeof(triangleFactors)
                       options:_bufferStorageMode];
    if (_bufferStorageMode != MTLStorageModeShared &&
        [_triangleTessFactors respondsToSelector:@selector(didModifyRange:)]) {
#if defined(ARCH_OS_OSX)
        [_triangleTessFactors didModifyRange:{0, _triangleTessFactors.length}];
#endif
    }

    MTLQuadTessellationFactorsHalf quadFactors;
    quadFactors.insideTessellationFactor[0] = factorZero;
    quadFactors.insideTessellationFactor[1] = factorZero;
    quadFactors.edgeTessellationFactor[0] = factorOne;
    quadFactors.edgeTessellationFactor[1] = factorOne;
    quadFactors.edgeTessellationFactor[2] = factorOne;
    quadFactors.edgeTessellationFactor[3] = factorOne;
    _quadTessFactors =
        [_device
            newBufferWithBytes:&quadFactors
                        length:sizeof(quadFactors)
                       options:_bufferStorageMode];
    if (_bufferStorageMode != MTLStorageModeShared &&
        [_quadTessFactors respondsToSelector:@selector(didModifyRange:)]) {
#if defined(ARCH_OS_OSX)
        [_quadTessFactors didModifyRange:{0, _quadTessFactors.length}];
#endif
    }
}

std::string
_ArgId(ArgIndex index)
{
    return "[[ id(" + std::to_string(index) + ") ]]";
}

std::string
_Buffer(uint32_t index)
{
    return "[[ buffer(" + std::to_string(index) + ") ]]";
}

std::string
_MainArgBuffer(uint32_t offset, uint32_t index)
{
    return "(const device uint8_t*)args->mainArgumentBuffer + "
           + std::to_string(offset) + ", " + std::to_string(index);
}

HgiMetalIndirectCommandEncoder::FunctionState
HgiMetalIndirectCommandEncoder::_GetFunction(
    HgiGraphicsPipelineDesc const& pipelineDesc,
    bool isIndexed)
{
    static const std::string _shaderSource =
        "#include <metal_stdlib>\n"
        "using namespace metal;\n"
    
        "struct StepFunctionDesc\n"
        "{\n"
        "  uint32_t bindingIndex;\n"
        "  uint32_t byteOffset;\n"
        "  uint32_t stride;\n"
        "};\n"

        "struct ArgBuffer\n"
        "{\n"
        "  command_buffer commandBuffer " + _ArgId(ArgIndex_ICB) + ";\n"
        "  const device uint32_t* indexBuffer " + _ArgId(ArgIndex_IndexBuffer) + ";\n"
        "  const device void* mainArgumentBuffer " + _ArgId(ArgIndex_MainArgumentBuffer) + ";\n"
        "  const device void* patchFactorsBuffer " + _ArgId(ArgIndex_PatchFactorsBuffer) + ";\n"
        "  primitive_type type " + _ArgId(ArgIndex_PrimitiveType) + ";\n"
        "  StepFunctionDesc drawStepFunc " + _ArgId(ArgIndex_DrawStepFunction) + ";\n"
        "  uint32_t numPatchStepFuncs " + _ArgId(ArgIndex_NumPatchStepFuncs) + ";\n"
        "  array<StepFunctionDesc, " + std::to_string(MaxStepFunctions) + "> patchStepFuncs " + _ArgId(ArgIndex_PatchStepFunctions) + ";\n"
        "  uint32_t numControlPoints " + _ArgId(ArgIndex_NumControlPoints) + ";\n"
        "  uint32_t patchBaseVertexByteOffset " + _ArgId(ArgIndex_PatchBaseVertexByteOffset) + ";\n"
        "  uint32_t numBuffers " + _ArgId(ArgIndex_NumBuffers) + ";\n"
        "  array<const device void*, " + std::to_string(MaxVertexBufferBindings) + "> buffers " + _ArgId(ArgIndex_Buffers) + ";\n"
        "};\n"
    
        "void SetMainBuffers(\n"
        "  thread render_command& cmd,\n"
        "  const device ArgBuffer* args)\n"
        "{\n"
        "  cmd.set_vertex_buffer(" + _MainArgBuffer(HgiMetalArgumentOffsetBufferVS, HgiMetalArgumentIndexBuffers) + ");\n"
        "  cmd.set_fragment_buffer(" + _MainArgBuffer(HgiMetalArgumentOffsetBufferFS, HgiMetalArgumentIndexBuffers) + ");\n"
        "  cmd.set_vertex_buffer(" + _MainArgBuffer(HgiMetalArgumentOffsetTextureVS, HgiMetalArgumentIndexTextures) + ");\n"
        "  cmd.set_fragment_buffer(" + _MainArgBuffer(HgiMetalArgumentOffsetTextureFS, HgiMetalArgumentIndexTextures) + ");\n"
        "  cmd.set_vertex_buffer(" + _MainArgBuffer(HgiMetalArgumentOffsetSamplerVS, HgiMetalArgumentIndexSamplers) + ");\n"
        "  cmd.set_fragment_buffer(" + _MainArgBuffer(HgiMetalArgumentOffsetSamplerFS, HgiMetalArgumentIndexSamplers) + ");\n"
        "  cmd.set_vertex_buffer(" + _MainArgBuffer(HgiMetalArgumentOffsetConstants, HgiMetalArgumentIndexConstants) + ");\n"
        "  cmd.set_fragment_buffer(" + _MainArgBuffer(HgiMetalArgumentOffsetConstants, HgiMetalArgumentIndexConstants) + ");\n"
        "}\n"
    
        "const device uint8_t* GetDrawCmdPtr(\n"
        "  const device ArgBuffer* args,\n"
        "  uint drawItemIndex)\n"
        "{\n"
        "  return\n"
        "    (const device uint8_t*)args->buffers[args->drawStepFunc.bindingIndex]"
        "    + args->drawStepFunc.byteOffset\n"
        "    + (drawItemIndex * args->drawStepFunc.stride);\n"
        "}\n"
    
        "const device uint8_t* BufferOffset(\n"
        "  const device uint8_t* bufferPtr,"
        "  const device StepFunctionDesc& stepFuncDesc,"
        "  uint32_t base)\n"
        "{\n"
        "  return bufferPtr + stepFuncDesc.byteOffset + (base * stepFuncDesc.stride);\n"
        "}\n"

        "// From _DrawNonIndexedCommand\n"
        "struct DrawPrimitivesCmd\n"
        "{\n"
        "  uint vertexCount;\n"
        "  uint instanceCount;\n"
        "  uint vertexStart;\n"
        "  uint baseInstance;\n"
        "};\n"

        "kernel void\n"
        "MultiDrawPrimitives(uint drawItemIndex [[ thread_position_in_grid ]],\n"
        "device ArgBuffer *args " + _Buffer(HgiMetalArgumentIndexICB) + ")\n"
        "{\n"
        "  render_command cmd(args->commandBuffer, drawItemIndex);\n"
        "  SetMainBuffers(cmd, args);\n"
        "  const device uint8_t* drawCmdU8 = GetDrawCmdPtr(args, drawItemIndex);\n"
        "  const device DrawPrimitivesCmd* drawCmd = "
        "    (const device DrawPrimitivesCmd*)drawCmdU8;\n"
        "  for (uint32_t i = 0; i < args->numBuffers; ++i)\n"
        "  {\n"
        "    const device uint8_t* bufferPtr = (device uint8_t*)args->buffers[i];\n"
        "    if (i == args->drawStepFunc.bindingIndex) {\n"
        "      bufferPtr = BufferOffset(bufferPtr, args->drawStepFunc, drawCmd->baseInstance);\n"
        "    }\n"
        "    cmd.set_vertex_buffer(bufferPtr, i);\n"
        "  }\n"
        "  cmd.draw_primitives(\n"
        "    args->type,\n"
        "    drawCmd->vertexStart,\n"
        "    drawCmd->vertexCount,\n"
        "    drawCmd->instanceCount,\n"
        "    drawCmd->baseInstance);\n"
        "}\n"
        
        "// From _DrawIndexedCommand\n"
        "struct DrawIndexedCmd\n"
        "{\n"
        "  uint32_t indexCount;\n"
        "  uint32_t instanceCount;\n"
        "  uint32_t indexStart;\n"
        "  int32_t  baseVertex;\n"
        "  uint32_t baseInstance;\n"
        "};\n"
        
        "kernel void\n"
        "MultiDrawIndexedPrimitives(uint drawItemIndex [[ thread_position_in_grid ]],\n"
        "device ArgBuffer *args " + _Buffer(HgiMetalArgumentIndexICB) + ")\n"
        "{\n"
        "  render_command cmd(args->commandBuffer, drawItemIndex);\n"
        "  SetMainBuffers(cmd, args);\n"
        "  const device uint8_t* drawCmdU8 = GetDrawCmdPtr(args, drawItemIndex);\n"
        "  const device DrawIndexedCmd* drawCmd = "
        "    (const device DrawIndexedCmd*)drawCmdU8;\n"
        "  for (uint32_t i = 0; i < args->numBuffers; ++i)\n"
        "  {\n"
        "    const device uint8_t* bufferPtr = (device uint8_t*)args->buffers[i];\n"
        "    if (i == args->drawStepFunc.bindingIndex) {\n"
        "      bufferPtr = BufferOffset(bufferPtr, args->drawStepFunc, drawCmd->baseInstance);\n"
        "    }\n"
        "    cmd.set_vertex_buffer(bufferPtr, i);\n"
        "  }\n"
        "  cmd.draw_indexed_primitives(\n"
        "    args->type,\n"
        "    drawCmd->indexCount,\n"
        "    args->indexBuffer + drawCmd->indexStart,\n"
        "    drawCmd->instanceCount,\n"
        "    drawCmd->baseVertex,\n"
        "    drawCmd->baseInstance);\n"
        "}\n"

        "// From MTLDrawPatchIndirectArguments\n"
        "struct DrawPatchCmd\n"
        "{\n"
        "  uint32_t patchCount;\n"
        "  uint32_t instanceCount;\n"
        "  uint32_t patchStart;\n"
        "  uint32_t baseInstance;\n"
        "};\n"
        
        "const device DrawPatchCmd* SetPatchVertexBuffers(\n"
        "  thread render_command& cmd,\n"
        "  const device ArgBuffer* args,\n"
        "  uint drawItemIndex)\n"
        "{\n"
        "  const device uint8_t* drawCmdU8 = GetDrawCmdPtr(args, drawItemIndex);\n"
        "  const device DrawPatchCmd* drawCmd = (const device DrawPatchCmd*)drawCmdU8;\n"
        "  uint32_t baseVertexIndex = \n"
        "    (args->patchBaseVertexByteOffset + drawItemIndex * args->drawStepFunc.stride) / sizeof(uint32_t);\n"
        "  const device uint32_t* drawCmdU32 = (const device uint32_t*)(args->buffers[args->drawStepFunc.bindingIndex]);\n"
        "  uint32_t baseVertex = drawCmdU32[baseVertexIndex];\n"
        "  for (uint32_t i = 0; i < args->numBuffers; ++i)\n"
        "  {\n"
        "    const device uint8_t* bufferPtr = (device uint8_t*)args->buffers[i];\n"
        "    if (i == args->drawStepFunc.bindingIndex) {\n"
        "      bufferPtr = BufferOffset(bufferPtr, args->drawStepFunc, drawCmd->baseInstance);\n"
        "    } else {\n"
        "      for (uint32_t j = 0; j < args->numPatchStepFuncs; ++j) {\n"
        "        if (args->patchStepFuncs[j].bindingIndex == i) {\n"
        "          bufferPtr = BufferOffset(bufferPtr, args->patchStepFuncs[j], baseVertex);\n"
        "        }\n"
        "      }\n"
        "    }\n"
        "    cmd.set_vertex_buffer(bufferPtr, i);\n"
        "  }\n"
        "  return drawCmd;\n"
        "}\n"

        "kernel void\n"
        "MultiDrawTriPatches(uint drawItemIndex [[ thread_position_in_grid ]],\n"
        "device ArgBuffer *args " + _Buffer(HgiMetalArgumentIndexICB) + ")\n"
        "{\n"
        "  render_command cmd(args->commandBuffer, drawItemIndex);\n"
        "  SetMainBuffers(cmd, args);\n"
        "  const device DrawPatchCmd* drawCmd =\n"
        "    SetPatchVertexBuffers(cmd, args, drawItemIndex);\n"
        "  cmd.draw_patches(\n"
        "    args->numControlPoints,\n"
        "    drawCmd->patchStart,\n"
        "    drawCmd->patchCount,\n"
        "    (const device uint*)nullptr,\n"
        "    drawCmd->instanceCount,\n"
        "    drawCmd->baseInstance,\n"
        "    (const device MTLTriangleTessellationFactorsHalf*)args->patchFactorsBuffer);\n"
        "}\n"
        
        "kernel void\n"
        "MultiDrawIndexedTriPatches(uint drawItemIndex [[ thread_position_in_grid ]],\n"
        "device ArgBuffer *args " + _Buffer(HgiMetalArgumentIndexICB) + ")\n"
        "{\n"
        "  render_command cmd(args->commandBuffer, drawItemIndex);\n"
        "  SetMainBuffers(cmd, args);\n"
        "  const device DrawPatchCmd* drawCmd =\n"
        "    SetPatchVertexBuffers(cmd, args, drawItemIndex);\n"
        "  cmd.draw_indexed_patches(\n"
        "    args->numControlPoints,\n"
        "    drawCmd->patchStart,\n"
        "    drawCmd->patchCount,\n"
        "    (const device uint*)nullptr,\n"
        "    args->indexBuffer,\n"
        "    drawCmd->instanceCount,\n"
        "    drawCmd->baseInstance,\n"
        "    (const device MTLTriangleTessellationFactorsHalf*)args->patchFactorsBuffer);\n"
        "}\n"

        "kernel void\n"
        "MultiDrawQuadPatches(uint drawItemIndex [[ thread_position_in_grid ]],\n"
        "device ArgBuffer *args " + _Buffer(HgiMetalArgumentIndexICB) + ")\n"
        "{\n"
        "  render_command cmd(args->commandBuffer, drawItemIndex);\n"
        "  SetMainBuffers(cmd, args);\n"
        "  const device DrawPatchCmd* drawCmd =\n"
        "    SetPatchVertexBuffers(cmd, args, drawItemIndex);\n"
        "  cmd.draw_patches(\n"
        "    args->numControlPoints,\n"
        "    drawCmd->patchStart,\n"
        "    drawCmd->patchCount,\n"
        "    (const device uint*)nullptr,\n"
        "    drawCmd->instanceCount,\n"
        "    drawCmd->baseInstance,\n"
        "    (const device MTLQuadTessellationFactorsHalf*)args->patchFactorsBuffer);\n"
        "}\n"
        
        "kernel void\n"
        "MultiDrawIndexedQuadPatches(uint drawItemIndex [[ thread_position_in_grid ]],\n"
        "device ArgBuffer *args " + _Buffer(HgiMetalArgumentIndexICB) + ")\n"
        "{\n"
        "  render_command cmd(args->commandBuffer, drawItemIndex);\n"
        "  SetMainBuffers(cmd, args);\n"
        "  const device DrawPatchCmd* drawCmd =\n"
        "    SetPatchVertexBuffers(cmd, args, drawItemIndex);\n"
        "  cmd.draw_indexed_patches(\n"
        "    args->numControlPoints,\n"
        "    drawCmd->patchStart,\n"
        "    drawCmd->patchCount,\n"
        "    (const device uint*)nullptr,\n"
        "    args->indexBuffer,\n"
        "    drawCmd->instanceCount,\n"
        "    drawCmd->baseInstance,\n"
        "    (const device MTLQuadTessellationFactorsHalf*)args->patchFactorsBuffer);\n"
        "}\n"
    ;

    if (!_library) {
        NSError *error = NULL;
        MTLCompileOptions *options = [[MTLCompileOptions alloc] init];
        options.fastMathEnabled = YES;
        
        NSString *shaderSource =
            [NSString stringWithUTF8String:_shaderSource.c_str()];

        _library = [_device newLibraryWithSource:shaderSource
                                         options:options
                                           error:&error];
        
        _functions.resize(6);
        [options release];
        options = nil;
        
        if (!_library) {
            NSString *errStr = [error localizedDescription];
            TF_FATAL_CODING_ERROR(
                "Failed to create multidraw pipeline state: %s",
                [errStr UTF8String]);
        }
    }

    static NSString *const _functionNames[] = {
        @"MultiDrawPrimitives",
        @"MultiDrawIndexedPrimitives",
        @"MultiDrawTriPatches",
        @"MultiDrawIndexedTriPatches",
        @"MultiDrawQuadPatches",
        @"MultiDrawIndexedQuadPatches"
    };

    int32_t type = 0;                   
    
    if (pipelineDesc.primitiveType != HgiPrimitiveTypePatchList) {
        type = (!isIndexed) ? 0 : 1;
    } else {
        if (pipelineDesc.tessellationState.patchType ==
            HgiTessellationState::PatchType::Triangle) {
            type = (!isIndexed) ? 2 : 3;
        } else {
            type = (!isIndexed) ? 4 : 5;
        }
    }

    FunctionState state = _functions[type];
    // Create a compute pipeline state for the generation of the ICB.
    NSError *error;
    
    if (!state.function) {
        state.function = [_library newFunctionWithName:_functionNames[type]];
        state.function.label = _functionNames[type];

        state.pipelineState =
            [_device newComputePipelineStateWithFunction:state.function
                                                error:&error];
        state.argumentEncoder =
            [state.function newArgumentEncoderWithBufferIndex:HgiMetalArgumentIndexICB];

        _functions[type] = state;
    }

    return state;
}

static const uint32_t _RoundUpPow2(uint32_t x)
{
    if (x <= 1) return 1;

    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
    
    return x;
}

id<MTLIndirectCommandBuffer>
HgiMetalIndirectCommandEncoder::_AllocateCommandBuffer(uint32_t drawCount)
{
    uint32_t roundedSize = _RoundUpPow2(drawCount);
    id<MTLIndirectCommandBuffer> commandBuffer = nil;
    
    {
        // Search for a buffer of the required size in the free pool.
        std::lock_guard<std::mutex> lock(_poolMutex);
        FreeCommandBuffers::iterator it = _commandBufferPool.find(roundedSize);

        if (it != _commandBufferPool.end()) {
            commandBuffer = it->second;
            _commandBufferPool.erase(it);
        }
    }

    if (!commandBuffer) {
        MTLIndirectCommandBufferDescriptor* descriptor =
            [MTLIndirectCommandBufferDescriptor new];
        
        descriptor.commandTypes = MTLIndirectCommandTypeDraw
                                | MTLIndirectCommandTypeDrawIndexed
                                | MTLIndirectCommandTypeDrawPatches
                                | MTLIndirectCommandTypeDrawIndexedPatches;
        descriptor.inheritBuffers = NO;
        descriptor.inheritPipelineState = YES;
        descriptor.maxVertexBufferBindCount = 31;
        descriptor.maxFragmentBufferBindCount = 31;

        commandBuffer =
            [_device newIndirectCommandBufferWithDescriptor:descriptor
                                            maxCommandCount:roundedSize
                                                    options:MTLResourceStorageModePrivate];
        [descriptor release];
        descriptor = nil;
    }

    return commandBuffer;
}

id<MTLBuffer>
HgiMetalIndirectCommandEncoder::_AllocateArgumentBuffer(uint32_t encodedLength)
{
    id<MTLBuffer> buffer = nil;
    uint32_t roundedSize = _RoundUpPow2(encodedLength);
    
    {
        // Search for a buffer of the required size in the free pool.
        std::lock_guard<std::mutex> lock(_poolMutex);
        FreeArgumentBuffers::iterator it = _argumentBufferPool.find(roundedSize);

        if (it != _argumentBufferPool.end()) {
            buffer = it->second;
            _argumentBufferPool.erase(it);
        }
    }

    if (!buffer) {
        buffer =
            [_device newBufferWithLength:roundedSize
                                 options:_bufferStorageMode];
    }
    
    return buffer;
}

template<typename T>
void _SetArg(
    id<MTLArgumentEncoder> argumentEncoder,
             ArgIndex argumentIndex,
             T value)
{
    *(T*)[argumentEncoder constantDataAtIndex:argumentIndex] = value;
}

HgiIndirectCommandsUniquePtr
HgiMetalIndirectCommandEncoder::_EncodeDraw(
    HgiComputeCmds *computeCmds,
    HgiGraphicsPipelineHandle const& pipeline,
    HgiResourceBindingsHandle const& resourceBindings,
    HgiVertexBufferBindingVector const &bindings,
    HgiBufferHandle const &indexBuffer,
    uint32_t drawBufferByteOffset,
    uint32_t drawCount,
    uint32_t stride,
    uint32_t patchBaseVertexByteOffset)
{

    HgiGraphicsPipelineDesc const &pipelineDesc = pipeline->GetDescriptor();
    FunctionState function = _GetFunction(pipelineDesc, !!indexBuffer);

    // Create an argument buffer to hold all buffers and constants.
    std::unique_ptr<HgiMetalIndirectCommands> commands =
        std::make_unique<HgiMetalIndirectCommands>(
            drawCount,
            pipeline,
            resourceBindings,
            _AllocateCommandBuffer(drawCount),
            _AllocateArgumentBuffer(function.argumentEncoder.encodedLength),
            _AllocateArgumentBuffer(HgiMetalArgumentOffsetSize));

    [function.argumentEncoder setArgumentBuffer:commands->indirectArgumentBuffer
                                         offset:0];

    HgiMetalComputeCmds* metalComputeCmds = static_cast<HgiMetalComputeCmds*>(computeCmds);
    id<MTLComputeCommandEncoder> encoder = metalComputeCmds->GetEncoder();

    // Create the ICB and add it to the argument buffer.
    [function.argumentEncoder setIndirectCommandBuffer:commands->indirectCommandBuffer
                                                atIndex:ArgIndex_ICB];
    [encoder useResource:commands->indirectCommandBuffer
                   usage:(MTLResourceUsageRead | MTLResourceUsageWrite)];
    
    // Pass the main argument buffer through so the resources can be bound.
    [function.argumentEncoder setBuffer:commands->mainArgumentBuffer
                                 offset:0
                                atIndex:ArgIndex_MainArgumentBuffer];
    [encoder useResource:commands->mainArgumentBuffer
                   usage:(MTLResourceUsageRead | MTLResourceUsageWrite)];

    // Add the constants to argument buffer.
    MTLPrimitiveType mtlPrimitiveType =
        HgiMetalConversions::GetPrimitiveType(pipelineDesc.primitiveType);

    HgiMetalStepFunctions stepFunctions(pipelineDesc, bindings);

    HgiMetalStepFunctionDesc drawCommandStep
        {stepFunctions.GetDrawBufferIndex(), drawBufferByteOffset, stride};

    TF_VERIFY(stepFunctions.GetPatchBaseDescs().size() <= MaxStepFunctions);
    
    uint32_t controlPointCount =
        pipelineDesc.tessellationState.primitiveIndexSize;

    _SetArg(function.argumentEncoder, ArgIndex_PrimitiveType, mtlPrimitiveType);
    _SetArg(function.argumentEncoder, ArgIndex_DrawStepFunction, drawCommandStep);
    _SetArg(function.argumentEncoder, ArgIndex_NumControlPoints, controlPointCount);
    _SetArg(function.argumentEncoder, ArgIndex_PatchBaseVertexByteOffset,
        patchBaseVertexByteOffset);
    _SetArg(function.argumentEncoder, ArgIndex_NumBuffers, bindings.size());
    _SetArg(function.argumentEncoder, ArgIndex_NumPatchStepFuncs,
        (uint32_t)stepFunctions.GetPatchBaseDescs().size());
    HgiMetalStepFunctionDesc* argPatchStepDescs = (HgiMetalStepFunctionDesc*)
        [function.argumentEncoder constantDataAtIndex:ArgIndex_PatchStepFunctions];
    for (auto const& stepFuncDesc : stepFunctions.GetPatchBaseDescs()) {
        *argPatchStepDescs = stepFuncDesc;
        ++argPatchStepDescs;
    }

    bool usedExplicitTessFactorBuffer = false;
    for (const HgiBufferBindDesc &buffer :
         resourceBindings->GetDescriptor().buffers) {
        if (buffer.resourceType == HgiBindResourceTypeTessFactors) {
            if (buffer.buffers[0]) {
                HgiMetalBuffer* mtlBuffer =
                    static_cast<HgiMetalBuffer*>(buffer.buffers[0].Get());

                [function.argumentEncoder setBuffer:mtlBuffer->GetBufferId()
                                             offset:buffer.offsets[0]
                                            atIndex:ArgIndex_PatchFactorsBuffer];
                usedExplicitTessFactorBuffer = true;
            }
        }
    }
    if (pipelineDesc.primitiveType == HgiPrimitiveTypePatchList &&
            !usedExplicitTessFactorBuffer) {
        id<MTLBuffer> patchFactorsBuffer = _triangleTessFactors;
        if (pipelineDesc.tessellationState.patchType ==
            HgiTessellationState::PatchType::Quad) {
            patchFactorsBuffer = _quadTessFactors;
        }
        [function.argumentEncoder setBuffer:patchFactorsBuffer
                                     offset:0
                                    atIndex:ArgIndex_PatchFactorsBuffer];
        [encoder useResource:patchFactorsBuffer
                       usage:(MTLResourceUsageRead | MTLResourceUsageWrite)];
    }

    // Add the index buffer to the argument buffer.
    if (indexBuffer) {
        HgiMetalBuffer* mtlIndexBuffer =
            static_cast<HgiMetalBuffer*>(indexBuffer.Get());
        [function.argumentEncoder setBuffer:mtlIndexBuffer->GetBufferId()
                                     offset:0
                                    atIndex:ArgIndex_IndexBuffer];
        [encoder useResource:mtlIndexBuffer->GetBufferId()
                       usage:(MTLResourceUsageRead | MTLResourceUsageWrite)];
    }

    // Add the vertex buffers to the argument buffer so they can be bound.
    TF_VERIFY(bindings.size() < MaxVertexBufferBindings);
    uint32_t index = 0;

    for (auto const& binding : bindings) {
        if (binding.buffer) {
            HgiMetalBuffer* mtlBuffer =
                static_cast<HgiMetalBuffer*>(binding.buffer.Get());
            [function.argumentEncoder setBuffer:mtlBuffer->GetBufferId()
                                         offset:binding.byteOffset
                                        atIndex:ArgIndex_Buffers + index];
            [encoder useResource:mtlBuffer->GetBufferId()
                           usage:(MTLResourceUsageRead | MTLResourceUsageWrite)];
        }
        index++;
    }

    if (_bufferStorageMode != MTLStorageModeShared &&
        [commands->indirectArgumentBuffer
            respondsToSelector:@selector(didModifyRange:)]) {
#if defined(ARCH_OS_OSX)
        [commands->indirectArgumentBuffer
            didModifyRange:{0, commands->indirectArgumentBuffer.length}];
#endif
    }

    // Set pipeline state on the encoder and dispatch to populate the ICB
    [encoder setComputePipelineState:function.pipelineState];
    [encoder setBuffer:commands->indirectArgumentBuffer
                offset:0
               atIndex:HgiMetalArgumentIndexICB];

    NSUInteger threadExecutionWidth = function.pipelineState.threadExecutionWidth;

    [encoder dispatchThreads:MTLSizeMake(drawCount, 1, 1)
       threadsPerThreadgroup:MTLSizeMake(threadExecutionWidth, 1, 1)];

    return commands;
}

void
HgiMetalIndirectCommandEncoder::ExecuteDraw(
    HgiGraphicsCmds * gfxCmds,
    HgiIndirectCommands const* commands)
{
    HgiMetalGraphicsCmds *metalGfxCmds = static_cast<HgiMetalGraphicsCmds*>(gfxCmds);
    id<MTLRenderCommandEncoder> encoder = metalGfxCmds->GetEncoder();

    id<MTLCommandBuffer> commandBuffer =
        _hgi->GetPrimaryCommandBuffer(this, false);
    HgiMetalIndirectCommands const* metalCommands =
        static_cast<HgiMetalIndirectCommands const*>(commands);

    // Bind the encoder pipeline and draw everything in the ICB
    HgiMetalGraphicsPipeline* graphicsPipeline =
        static_cast<HgiMetalGraphicsPipeline*>(metalCommands->graphicsPipeline.Get());
    graphicsPipeline->BindPipeline(encoder);

    // Bind the resources.
    id<MTLBuffer> mainArgumentBuffer = metalCommands->mainArgumentBuffer;
    HgiMetalResourceBindings* resourceBindings =
        static_cast<HgiMetalResourceBindings*>(metalCommands->resourceBindings.Get());
    resourceBindings->BindResources(_hgi,
                                    encoder,
                                    mainArgumentBuffer);
    
    // Ensure the the main argument buffer is updated on managed hardware.
    if (mainArgumentBuffer.storageMode != MTLStorageModeShared &&
        [mainArgumentBuffer respondsToSelector:@selector(didModifyRange:)]) {
#if defined(ARCH_OS_OSX)
        [mainArgumentBuffer didModifyRange:{0, mainArgumentBuffer.length}];
#endif
    }
    
    id<MTLIndirectCommandBuffer> indirectCommandBuffer =
        metalCommands->indirectCommandBuffer;
    id<MTLBuffer> argumentBuffer = metalCommands->indirectArgumentBuffer;
    [encoder setVertexBuffer:argumentBuffer
                      offset:0
                     atIndex:HgiMetalArgumentIndexICB];

    [encoder
        executeCommandsInBuffer:indirectCommandBuffer
                      withRange:{0, metalCommands->drawCount}];

    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> cmdBuffer)
    {
        std::lock_guard<std::mutex> lock(_poolMutex);
        
        _argumentBufferPool.insert({mainArgumentBuffer.length, mainArgumentBuffer});
        _argumentBufferPool.insert({argumentBuffer.length, argumentBuffer});
        _commandBufferPool.insert({indirectCommandBuffer.size, indirectCommandBuffer});
    }];
}

HgiIndirectCommandsUniquePtr
HgiMetalIndirectCommandEncoder::EncodeDraw(
    HgiComputeCmds * computeCmds,
    HgiGraphicsPipelineHandle const& pipeline,
    HgiResourceBindingsHandle const& resourceBindings,
    HgiVertexBufferBindingVector const& vertexBindings,
    HgiBufferHandle const& drawParameterBuffer,
    uint32_t drawBufferByteOffset,
    uint32_t drawCount,
    uint32_t stride)
{
    HgiIndirectCommandsUniquePtr commands =
        _EncodeDraw(
            computeCmds,
            pipeline,
            resourceBindings,
            vertexBindings,
            HgiBufferHandle(),
            drawBufferByteOffset,
            drawCount,
            stride,
            0);

    return commands;
}

HgiIndirectCommandsUniquePtr
HgiMetalIndirectCommandEncoder::EncodeDrawIndexed(
    HgiComputeCmds * computeCmds,
    HgiGraphicsPipelineHandle const& pipeline,
    HgiResourceBindingsHandle const& resourceBindings,
    HgiVertexBufferBindingVector const& vertexBindings,
    HgiBufferHandle const& indexBuffer,
    HgiBufferHandle const& drawParameterBuffer,
    uint32_t drawBufferByteOffset,
    uint32_t drawCount,
    uint32_t stride,
    uint32_t patchBaseVertexByteOffset)
{
    HgiIndirectCommandsUniquePtr commands =
        _EncodeDraw(
            computeCmds,
            pipeline,
            resourceBindings,
            vertexBindings,
            indexBuffer,
            drawBufferByteOffset,
            drawCount,
            stride,
            patchBaseVertexByteOffset);

    return commands;
}

PXR_NAMESPACE_CLOSE_SCOPE
