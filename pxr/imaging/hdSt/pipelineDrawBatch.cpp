//
// Copyright 2021 Pixar
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
#include "pxr/imaging/hdSt/pipelineDrawBatch.h"

#include "pxr/imaging/hdSt/binding.h"
#include "pxr/imaging/hdSt/bufferArrayRange.h"
#include "pxr/imaging/hdSt/codeGen.h"
#include "pxr/imaging/hdSt/commandBuffer.h"
#include "pxr/imaging/hdSt/cullingShaderKey.h"
#include "pxr/imaging/hdSt/debugCodes.h"
#include "pxr/imaging/hdSt/drawItemInstance.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/hgiConversions.h"
#include "pxr/imaging/hdSt/materialNetworkShader.h"
#include "pxr/imaging/hdSt/indirectDrawBatch.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hdSt/shaderKey.h"
#include "pxr/imaging/hdSt/textureBinder.h"

#include "pxr/imaging/hd/debugCodes.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hgi/blitCmds.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"
#include "pxr/imaging/hgi/graphicsPipeline.h"
#include "pxr/imaging/hgi/indirectCommandEncoder.h"
#include "pxr/imaging/hgi/resourceBindings.h"

#include "pxr/base/gf/matrix4f.h"

#include "pxr/base/arch/hash.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticTokens.h"

#include <iostream>
#include <limits>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (constantPrimvars)

    (dispatchBuffer)
    (drawCullInput)

    (drawIndirect)
    (drawIndirectCull)
    (drawIndirectResult)

    (ulocCullParams)
);


TF_DEFINE_ENV_SETTING(HDST_ENABLE_PIPELINE_DRAW_BATCH_GPU_FRUSTUM_CULLING, true,
                      "Enable pipeline draw batching GPU frustum culling");

HdSt_PipelineDrawBatch::HdSt_PipelineDrawBatch(
    HdStDrawItemInstance * drawItemInstance,
    bool const allowGpuFrustumCulling,
    bool const allowIndirectCommandEncoding)
    : HdSt_DrawBatch(drawItemInstance)
    , _drawCommandBufferDirty(false)
    , _bufferArraysHash(0)
    , _barElementOffsetsHash(0)
    , _numVisibleItems(0)
    , _numTotalVertices(0)
    , _numTotalElements(0)
    /* The following two values are set before draw by
     * SetEnableTinyPrimCulling(). */
    , _useTinyPrimCulling(false)
    , _dirtyCullingProgram(false)
    /* The following four values are initialized in _Init(). */
    , _useDrawIndexed(true)
    , _useInstancing(false)
    , _useGpuCulling(false)
    , _useInstanceCulling(false)
    , _allowGpuFrustumCulling(allowGpuFrustumCulling)
    , _allowIndirectCommandEncoding(allowIndirectCommandEncoding)
    , _instanceCountOffset(0)
    , _cullInstanceCountOffset(0)
    , _drawCoordOffset(0)
    , _patchBaseVertexByteOffset(0)
{
    _Init(drawItemInstance);
}

HdSt_PipelineDrawBatch::~HdSt_PipelineDrawBatch() = default;

/*virtual*/
void
HdSt_PipelineDrawBatch::_Init(HdStDrawItemInstance * drawItemInstance)
{
    HdSt_DrawBatch::_Init(drawItemInstance);
    drawItemInstance->SetBatchIndex(0);
    drawItemInstance->SetBatch(this);

    // remember buffer arrays version for dispatch buffer updating
    HdStDrawItem const * drawItem = drawItemInstance->GetDrawItem();
    _bufferArraysHash = drawItem->GetBufferArraysHash();
    // _barElementOffsetsHash is updated during _CompileBatch
    _barElementOffsetsHash = 0;

    // determine drawing and culling config according to the first drawitem
    _useDrawIndexed = static_cast<bool>(drawItem->GetTopologyRange());
    _useInstancing  = static_cast<bool>(drawItem->GetInstanceIndexRange());
    _useGpuCulling  = _allowGpuFrustumCulling && IsEnabledGPUFrustumCulling();

    // note: _useInstancing condition is not necessary. it can be removed
    //       if we decide always to use instance culling.
    _useInstanceCulling = _useInstancing &&
        _useGpuCulling && IsEnabledGPUInstanceFrustumCulling();

    if (_useGpuCulling) {
        _cullingProgram.Initialize(
            _useDrawIndexed, _useInstanceCulling, _bufferArraysHash);
    }

    TF_DEBUG(HDST_DRAW_BATCH).Msg(
        "   Resetting dispatch buffer.\n");
    _dispatchBuffer.reset();
}

void
HdSt_PipelineDrawBatch::SetEnableTinyPrimCulling(bool tinyPrimCulling)
{
    if (_useTinyPrimCulling != tinyPrimCulling) {
        _useTinyPrimCulling = tinyPrimCulling;
        _dirtyCullingProgram = true;
    }
}

/* static */
bool
HdSt_PipelineDrawBatch::IsEnabled(HgiCapabilities const *hgiCapabilities)
{
    // We require Hgi resource generation.
    return HdSt_CodeGen::IsEnabledHgiResourceGeneration(hgiCapabilities);
}

/* static */
bool
HdSt_PipelineDrawBatch::IsEnabledGPUFrustumCulling()
{
    // Allow GPU frustum culling for PipelineDrawBatch to be disabled even
    // when other GPU frustum culling is enabled. Both switches must be true
    // for PipelineDrawBatch to use GPU frustum culling.
    static bool isEnabled =
        TfGetEnvSetting(HDST_ENABLE_PIPELINE_DRAW_BATCH_GPU_FRUSTUM_CULLING);
    return isEnabled && HdSt_IndirectDrawBatch::IsEnabledGPUFrustumCulling();
}

/* static */
bool
HdSt_PipelineDrawBatch::IsEnabledGPUCountVisibleInstances()
{
    return HdSt_IndirectDrawBatch::IsEnabledGPUCountVisibleInstances();
}

/* static */
bool
HdSt_PipelineDrawBatch::IsEnabledGPUInstanceFrustumCulling()
{
    return HdSt_IndirectDrawBatch::IsEnabledGPUInstanceFrustumCulling();
}

////////////////////////////////////////////////////////////
// GPU Command Buffer Preparation
////////////////////////////////////////////////////////////

namespace {

// Draw command dispatch buffers are built as arrays of uint32_t, but
// we use these struct definitions to reason consistently about element
// access and offsets.
//
// The _DrawingCoord struct defines bundles of element offsets into buffers
// which together represent the drawing coordinate input to the shader.
// These must be kept in sync with codeGen. For instanced culling we need
// only a subset of the drawing coord. It might be beneficial to rearrange
// the drawing coord tuples.
//
// Note: _Draw*Command structs are layed out such that the first elements
// match the layout of Vulkan and GL and D3D indirect draw parameters.
//
// Note: Metal Patch drawing uses a different encoding than Vulkan and GL
// and D3D. Also, there is no base vertex offset in the indexed draw encoding,
// so we need to manually step vertex buffer binding offsets while encoding
// draw commands.
//
// Note: GL specifies baseVertex as 'int' and other as 'uint', but
// we never set negative baseVertex in our use cases.

// DrawingCoord 10 integers (+ numInstanceLevels)
struct _DrawingCoord
{
    // drawingCoord0 (ivec4 for drawing and culling)
    uint32_t modelDC;
    uint32_t constantDC;
    uint32_t elementDC;
    uint32_t primitiveDC;

    // drawingCoord1 (ivec4 for drawing or ivec2 for culling)
    uint32_t fvarDC;
    uint32_t instanceIndexDC;
    uint32_t shaderDC;
    uint32_t vertexDC;

    // drawingCoord2 (ivec2 for drawing)
    uint32_t topVisDC;
    uint32_t varyingDC;

    // drawingCoordI (int32[] for drawing and culling)
    // uint32_t instanceDC[numInstanceLevels];
};

// DrawNonIndexed + non-instance culling : 14 integers (+ numInstanceLevels)
struct _DrawNonIndexedCommand
{
    union {
        struct {
            uint32_t count;
            uint32_t instanceCount;
            uint32_t baseVertex;
            uint32_t baseInstance;
        } common;
        struct {
            uint32_t patchCount;
            uint32_t instanceCount;
            uint32_t patchStart;
            uint32_t baseInstance;
        } metalPatch;
    };

    _DrawingCoord drawingCoord;
};

// DrawNonIndexed + Instance culling : 18 integers (+ numInstanceLevels)
struct _DrawNonIndexedInstanceCullCommand
{
    union {
        struct {
            uint32_t count;
            uint32_t instanceCount;
            uint32_t baseVertex;
            uint32_t baseInstance;
        } common;
        struct {
            uint32_t patchCount;
            uint32_t instanceCount;
            uint32_t patchStart;
            uint32_t baseInstance;
        } metalPatch;
    };

    uint32_t cullCount;
    uint32_t cullInstanceCount;
    uint32_t cullBaseVertex;
    uint32_t cullBaseInstance;

    _DrawingCoord drawingCoord;
};

// DrawIndexed + non-instance culling : 15 integers (+ numInstanceLevels)
struct _DrawIndexedCommand
{
    union {
        struct {
            uint32_t count;
            uint32_t instanceCount;
            uint32_t baseIndex;
            uint32_t baseVertex;
            uint32_t baseInstance;
        } common;
        struct {
            uint32_t patchCount;
            uint32_t instanceCount;
            uint32_t patchStart;
            uint32_t baseInstance;
            uint32_t baseVertex;
        } metalPatch;
    };

    _DrawingCoord drawingCoord;
};

// DrawIndexed + Instance culling : 19 integers (+ numInstanceLevels)
struct _DrawIndexedInstanceCullCommand
{
    union {
        struct {
            uint32_t count;
            uint32_t instanceCount;
            uint32_t baseIndex;
            uint32_t baseVertex;
            uint32_t baseInstance;
        } common;
        struct {
            uint32_t patchCount;
            uint32_t instanceCount;
            uint32_t patchStart;
            uint32_t baseInstance;
            uint32_t baseVertex;
        } metalPatch;
    };

    uint32_t cullCount;
    uint32_t cullInstanceCount;
    uint32_t cullBaseVertex;
    uint32_t cullBaseInstance;

    _DrawingCoord drawingCoord;
};

// These traits capture sizes and offsets for the _Draw*Command structs
struct _DrawCommandTraits
{
    // Since the underlying buffer is an array of uint32_t, we capture
    // the size of the struct as the number of uint32_t elements.
    size_t numUInt32;

    // Additional uint32_t values needed to align command entries.
    size_t numUInt32Padding;

    size_t instancerNumLevels;
    size_t instanceIndexWidth;

    size_t count_offset;
    size_t instanceCount_offset;
    size_t baseInstance_offset;
    size_t cullCount_offset;
    size_t cullInstanceCount_offset;

    size_t drawingCoord0_offset;
    size_t drawingCoord1_offset;
    size_t drawingCoord2_offset;
    size_t drawingCoordI_offset;

    size_t patchBaseVertex_offset;
};

template <typename CmdType>
void _SetDrawCommandTraits(_DrawCommandTraits * traits,
                           int const instancerNumLevels,
                           size_t const uint32Alignment)
{
    // Number of uint32_t in the command struct
    // followed by instanceDC[instancerNumLevals]
    traits->numUInt32 = sizeof(CmdType) / sizeof(uint32_t)
                      + instancerNumLevels;

    if (uint32Alignment > 0) {
        size_t const alignMask = uint32Alignment - 1;
        size_t const alignedNumUInt32 =
                        (traits->numUInt32 + alignMask) & ~alignMask;
        traits->numUInt32Padding = alignedNumUInt32 - traits->numUInt32;
        traits->numUInt32 = alignedNumUInt32;
    } else {
        traits->numUInt32Padding = 0;
    }

    traits->instancerNumLevels = instancerNumLevels;
    traits->instanceIndexWidth = instancerNumLevels + 1;

    traits->count_offset = offsetof(CmdType, common.count);
    traits->instanceCount_offset = offsetof(CmdType, common.instanceCount);
    traits->baseInstance_offset = offsetof(CmdType, common.baseInstance);

    // These are different only for instanced culling.
    traits->cullCount_offset = traits->count_offset;
    traits->cullInstanceCount_offset = traits->instanceCount_offset;
}

template <typename CmdType>
void _SetInstanceCullTraits(_DrawCommandTraits * traits)
{
    traits->cullCount_offset = offsetof(CmdType, cullCount);
    traits->cullInstanceCount_offset = offsetof(CmdType, cullInstanceCount);
}

template <typename CmdType>
void _SetDrawingCoordTraits(_DrawCommandTraits * traits)
{
    // drawingCoord bundles are located by the offsets to their first elements
    traits->drawingCoord0_offset =
        offsetof(CmdType, drawingCoord) + offsetof(_DrawingCoord, modelDC);
    traits->drawingCoord1_offset =
        offsetof(CmdType, drawingCoord) + offsetof(_DrawingCoord, fvarDC);
    traits->drawingCoord2_offset =
        offsetof(CmdType, drawingCoord) + offsetof(_DrawingCoord, topVisDC);

    // drawingCoord instancer bindings follow the base drawing coord struct
    traits->drawingCoordI_offset = sizeof(CmdType);

    // needed to step vertex buffer binding offsets for Metal tessellation
    traits->patchBaseVertex_offset =
        offsetof(CmdType, drawingCoord) + offsetof(_DrawingCoord, vertexDC);
}

_DrawCommandTraits
_GetDrawCommandTraits(int const instancerNumLevels,
                      bool const useDrawIndexed,
                      bool const useInstanceCulling,
                      size_t const uint32Alignment)
{
    _DrawCommandTraits traits;
    if (!useDrawIndexed) {
        if (useInstanceCulling) {
            using CmdType = _DrawNonIndexedInstanceCullCommand;
            _SetDrawCommandTraits<CmdType>(&traits, instancerNumLevels,
                                           uint32Alignment);
            _SetInstanceCullTraits<CmdType>(&traits);
            _SetDrawingCoordTraits<CmdType>(&traits);
        } else {
            using CmdType = _DrawNonIndexedCommand;
            _SetDrawCommandTraits<CmdType>(&traits, instancerNumLevels,
                                           uint32Alignment);
            _SetDrawingCoordTraits<CmdType>(&traits);
        }
    } else {
        if (useInstanceCulling) {
            using CmdType = _DrawIndexedInstanceCullCommand;
            _SetDrawCommandTraits<CmdType>(&traits, instancerNumLevels,
                                           uint32Alignment);
            _SetInstanceCullTraits<CmdType>(&traits);
            _SetDrawingCoordTraits<CmdType>(&traits);
        } else {
            using CmdType = _DrawIndexedCommand;
            _SetDrawCommandTraits<CmdType>(&traits, instancerNumLevels,
                                           uint32Alignment);
            _SetDrawingCoordTraits<CmdType>(&traits);
        }
    }
    return traits;
}

void
_AddDrawResourceViews(HdStDispatchBufferSharedPtr const & dispatchBuffer,
                      _DrawCommandTraits const & traits)
{
    // draw indirect command
    dispatchBuffer->AddBufferResourceView(
        HdTokens->drawDispatch, {HdTypeInt32, 1},
        traits.count_offset);
    // drawing coord 0
    dispatchBuffer->AddBufferResourceView(
        HdTokens->drawingCoord0, {HdTypeInt32Vec4, 1},
        traits.drawingCoord0_offset);
    // drawing coord 1
    dispatchBuffer->AddBufferResourceView(
        HdTokens->drawingCoord1, {HdTypeInt32Vec4, 1},
        traits.drawingCoord1_offset);
    // drawing coord 2
    dispatchBuffer->AddBufferResourceView(
        HdTokens->drawingCoord2, {HdTypeInt32Vec2, 1},
        traits.drawingCoord2_offset);
    // instance drawing coords
    if (traits.instancerNumLevels > 0) {
        dispatchBuffer->AddBufferResourceView(
            HdTokens->drawingCoordI, {HdTypeInt32, traits.instancerNumLevels},
            traits.drawingCoordI_offset);
    }
}

HdBufferArrayRangeSharedPtr
_GetShaderBar(HdSt_MaterialNetworkShaderSharedPtr const & shader)
{
    return shader ? shader->GetShaderData() : nullptr;
}

// Collection of resources for a drawItem
struct _DrawItemState
{
    explicit _DrawItemState(HdStDrawItem const * drawItem)
        : constantBar(std::static_pointer_cast<HdStBufferArrayRange>(
                    drawItem->GetConstantPrimvarRange()))
        , indexBar(std::static_pointer_cast<HdStBufferArrayRange>(
                    drawItem->GetTopologyRange()))
        , topVisBar(std::static_pointer_cast<HdStBufferArrayRange>(
                    drawItem->GetTopologyVisibilityRange()))
        , elementBar(std::static_pointer_cast<HdStBufferArrayRange>(
                    drawItem->GetElementPrimvarRange()))
        , fvarBar(std::static_pointer_cast<HdStBufferArrayRange>(
                    drawItem->GetFaceVaryingPrimvarRange()))
        , varyingBar(std::static_pointer_cast<HdStBufferArrayRange>(
                    drawItem->GetVaryingPrimvarRange()))
        , vertexBar(std::static_pointer_cast<HdStBufferArrayRange>(
                    drawItem->GetVertexPrimvarRange()))
        , shaderBar(std::static_pointer_cast<HdStBufferArrayRange>(
                    _GetShaderBar(drawItem->GetMaterialNetworkShader())))
        , instanceIndexBar(std::static_pointer_cast<HdStBufferArrayRange>(
                    drawItem->GetInstanceIndexRange()))
    {
        instancePrimvarBars.resize(drawItem->GetInstancePrimvarNumLevels());
        for (size_t i = 0; i < instancePrimvarBars.size(); ++i) {
            instancePrimvarBars[i] =
                std::static_pointer_cast<HdStBufferArrayRange>(
                    drawItem->GetInstancePrimvarRange(i));
        }
    }

    HdStBufferArrayRangeSharedPtr constantBar;
    HdStBufferArrayRangeSharedPtr indexBar;
    HdStBufferArrayRangeSharedPtr topVisBar;
    HdStBufferArrayRangeSharedPtr elementBar;
    HdStBufferArrayRangeSharedPtr fvarBar;
    HdStBufferArrayRangeSharedPtr varyingBar;
    HdStBufferArrayRangeSharedPtr vertexBar;
    HdStBufferArrayRangeSharedPtr shaderBar;
    HdStBufferArrayRangeSharedPtr instanceIndexBar;
    std::vector<HdStBufferArrayRangeSharedPtr> instancePrimvarBars;
};

uint32_t
_GetElementOffset(HdBufferArrayRangeSharedPtr const & range)
{
    return range ? range->GetElementOffset() : 0;
}

uint32_t
_GetElementCount(HdBufferArrayRangeSharedPtr const & range)
{
    return range ? range->GetNumElements() : 0;
}

uint32_t
_GetInstanceCount(HdStDrawItemInstance const * drawItemInstance,
                  HdBufferArrayRangeSharedPtr const & instanceIndexBar,
                  int const instanceIndexWidth)
{
    // It's possible to have an instanceIndexBar which exists but is empty,
    // i.e. GetNumElements() == 0, and no instancePrimvars.
    // In that case instanceCount should be 0, instead of 1, otherwise the
    // frustum culling shader might write out-of-bounds to the result buffer.
    // this is covered by testHdDrawBatching/EmptyDrawBatchTest
    uint32_t const numInstances =
        instanceIndexBar ? instanceIndexBar->GetNumElements() : 1;
    uint32_t const instanceCount =
        drawItemInstance->IsVisible() ? (numInstances / instanceIndexWidth) : 0;
    return instanceCount;
}

HdStBufferResourceSharedPtr
_AllocateTessFactorsBuffer(
    HdStDrawItem const * drawItem,
    HdStResourceRegistrySharedPtr const & resourceRegistry)
{
    if (!drawItem) { return nullptr; }

    HdStBufferArrayRangeSharedPtr indexBar(
        std::static_pointer_cast<HdStBufferArrayRange>(
                drawItem->GetTopologyRange()));
    if (!indexBar) { return nullptr; }

    HdStBufferResourceSharedPtr indexBuffer =
         indexBar->GetResource(HdTokens->indices);
    if (!indexBuffer) { return nullptr; }

    HgiBufferHandle const & indexBufferHandle = indexBuffer->GetHandle();
    if (!indexBufferHandle) { return nullptr; }

    size_t const byteSizeOfTuple =
        HdDataSizeOfTupleType(indexBuffer->GetTupleType());
    size_t const byteSizeOfResource =
        indexBufferHandle->GetByteSizeOfResource();

    size_t const numElements = byteSizeOfResource / byteSizeOfTuple;
    size_t const numTessFactorsPerElement = 6;

    return resourceRegistry->RegisterBufferResource(
        HdTokens->tessFactors,
        HdTupleType{HdTypeHalfFloat, numElements*numTessFactorsPerElement},
        HgiBufferUsageUniform);
}

} // annonymous namespace

void
HdSt_PipelineDrawBatch::_CompileBatch(
    HdStResourceRegistrySharedPtr const & resourceRegistry)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (_drawItemInstances.empty()) return;

    size_t const numDrawItemInstances = _drawItemInstances.size();

    size_t const instancerNumLevels =
        _drawItemInstances[0]->GetDrawItem()->GetInstancePrimvarNumLevels();

    bool const useMetalTessellation =
        _drawItemInstances[0]->GetDrawItem()->
                GetGeometricShader()->GetUseMetalTessellation();

    // Align drawing commands to 32 bytes for Metal.
    size_t const uint32Alignment = useMetalTessellation ? 8 : 0;

    // Get the layout of the command buffer we are building.
    _DrawCommandTraits const traits =
        _GetDrawCommandTraits(instancerNumLevels,
                              _useDrawIndexed,
                              _useInstanceCulling,
                              uint32Alignment);

    TF_DEBUG(HDST_DRAW).Msg("\nCompile Dispatch Buffer\n");
    TF_DEBUG(HDST_DRAW).Msg(" - numUInt32: %zd\n", traits.numUInt32);
    TF_DEBUG(HDST_DRAW).Msg(" - useDrawIndexed: %d\n", _useDrawIndexed);
    TF_DEBUG(HDST_DRAW).Msg(" - useInstanceCulling: %d\n", _useInstanceCulling);
    TF_DEBUG(HDST_DRAW).Msg(" - num draw items: %zu\n", numDrawItemInstances);

    _drawCommandBuffer.resize(numDrawItemInstances * traits.numUInt32);
    std::vector<uint32_t>::iterator cmdIt = _drawCommandBuffer.begin();

    // Count the number of visible items. We may actually draw fewer
    // items than this when GPU frustum culling is active.
    _numVisibleItems = 0;
    _numTotalElements = 0;
    _numTotalVertices = 0;

    TF_DEBUG(HDST_DRAW).Msg(" - Processing Items:\n");
    _barElementOffsetsHash = 0;
    for (size_t item = 0; item < numDrawItemInstances; ++item) {
        HdStDrawItemInstance const *drawItemInstance = _drawItemInstances[item];
        HdStDrawItem const *drawItem = drawItemInstance->GetDrawItem();

        _barElementOffsetsHash =
            TfHash::Combine(_barElementOffsetsHash,
                            drawItem->GetElementOffsetsHash());

        _DrawItemState const dc(drawItem);

        // drawing coordinates.
        uint32_t const modelDC         = 0; // reserved for future extension
        uint32_t const constantDC      = _GetElementOffset(dc.constantBar);
        uint32_t const vertexDC        = _GetElementOffset(dc.vertexBar);
        uint32_t const topVisDC        = _GetElementOffset(dc.topVisBar);
        uint32_t const elementDC       = _GetElementOffset(dc.elementBar);
        uint32_t const primitiveDC     = _GetElementOffset(dc.indexBar);
        uint32_t const fvarDC          = _GetElementOffset(dc.fvarBar);
        uint32_t const instanceIndexDC = _GetElementOffset(dc.instanceIndexBar);
        uint32_t const shaderDC        = _GetElementOffset(dc.shaderBar);
        uint32_t const varyingDC       = _GetElementOffset(dc.varyingBar);

        // 3 for triangles, 4 for quads, 6 for triquads, n for patches
        uint32_t const numIndicesPerPrimitive =
            drawItem->GetGeometricShader()->GetPrimitiveIndexSize();

        uint32_t const baseVertex = vertexDC;
        uint32_t const vertexCount = _GetElementCount(dc.vertexBar);

        // if delegate fails to get vertex primvars, it could be empty.
        // skip the drawitem to prevent drawing uninitialized vertices.
        uint32_t const numElements =
            vertexCount != 0 ? _GetElementCount(dc.indexBar) : 0;

        uint32_t const baseIndex = primitiveDC * numIndicesPerPrimitive;
        uint32_t const indexCount = numElements * numIndicesPerPrimitive;

        uint32_t const instanceCount =
            _GetInstanceCount(drawItemInstance,
                              dc.instanceIndexBar,
                              traits.instanceIndexWidth);

        // tessellated patches are encoded differently for Metal.
        uint32_t const patchStart = primitiveDC;
        uint32_t const patchCount = numElements;

        uint32_t const baseInstance = (uint32_t)item;

        // draw command
        if (!_useDrawIndexed) {
            if (_useInstanceCulling) {
                // _DrawNonIndexedInstanceCullCommand
                if (useMetalTessellation) {
                    *cmdIt++ = patchCount;
                    *cmdIt++ = instanceCount;
                    *cmdIt++ = patchStart;
                    *cmdIt++ = baseInstance;

                    *cmdIt++ = 1;             /* cullCount (always 1) */
                    *cmdIt++ = instanceCount; /* cullInstanceCount */
                    *cmdIt++ = 0;             /* cullBaseVertex (not used)*/
                    *cmdIt++ = baseInstance;  /* cullBaseInstance */
                } else {
                    *cmdIt++ = vertexCount;
                    *cmdIt++ = instanceCount;
                    *cmdIt++ = baseVertex;
                    *cmdIt++ = baseInstance;

                    *cmdIt++ = 1;             /* cullCount (always 1) */
                    *cmdIt++ = instanceCount; /* cullInstanceCount */
                    *cmdIt++ = 0;             /* cullBaseVertex (not used)*/
                    *cmdIt++ = baseInstance;  /* cullBaseInstance */
                }
            } else {
                // _DrawNonIndexedCommand
                if (useMetalTessellation) {
                    *cmdIt++ = patchCount;
                    *cmdIt++ = instanceCount;
                    *cmdIt++ = patchStart;
                    *cmdIt++ = baseInstance;
                } else {
                    *cmdIt++ = vertexCount;
                    *cmdIt++ = instanceCount;
                    *cmdIt++ = baseVertex;
                    *cmdIt++ = baseInstance;
                }
            }
        } else {
            if (_useInstanceCulling) {
                // _DrawIndexedInstanceCullCommand
                if (useMetalTessellation) {
                    *cmdIt++ = patchCount;
                    *cmdIt++ = instanceCount;
                    *cmdIt++ = patchStart;
                    *cmdIt++ = baseInstance;
                    *cmdIt++ = baseVertex;

                    *cmdIt++ = 1;             /* cullCount (always 1) */
                    *cmdIt++ = instanceCount; /* cullInstanceCount */
                    *cmdIt++ = 0;             /* cullBaseVertex (not used)*/
                    *cmdIt++ = baseInstance;  /* cullBaseInstance */
                } else {
                    *cmdIt++ = indexCount;
                    *cmdIt++ = instanceCount;
                    *cmdIt++ = baseIndex;
                    *cmdIt++ = baseVertex;
                    *cmdIt++ = baseInstance;

                    *cmdIt++ = 1;             /* cullCount (always 1) */
                    *cmdIt++ = instanceCount; /* cullInstanceCount */
                    *cmdIt++ = 0;             /* cullBaseVertex (not used)*/
                    *cmdIt++ = baseInstance;  /* cullBaseInstance */
                }
            } else {
                // _DrawIndexedCommand
                if (useMetalTessellation) {
                    *cmdIt++ = patchCount;
                    *cmdIt++ = instanceCount;
                    *cmdIt++ = patchStart;
                    *cmdIt++ = baseInstance;
                    *cmdIt++ = baseVertex;
                } else {
                    *cmdIt++ = indexCount;
                    *cmdIt++ = instanceCount;
                    *cmdIt++ = baseIndex;
                    *cmdIt++ = baseVertex;
                    *cmdIt++ = baseInstance;
                }
            }
        }

        // drawingCoord0
        *cmdIt++ = modelDC;
        *cmdIt++ = constantDC;
        *cmdIt++ = elementDC;
        *cmdIt++ = primitiveDC;

        // drawingCoord1
        *cmdIt++ = fvarDC;
        *cmdIt++ = instanceIndexDC;
        *cmdIt++ = shaderDC;
        *cmdIt++ = vertexDC;

        // drawingCoord2
        *cmdIt++ = topVisDC;
        *cmdIt++ = varyingDC;

        // drawingCoordI
        for (size_t i = 0; i < dc.instancePrimvarBars.size(); ++i) {
            uint32_t instanceDC = _GetElementOffset(dc.instancePrimvarBars[i]);
            *cmdIt++ = instanceDC;
        }

        // add padding and clear to 0
        for (size_t i = 0; i < traits.numUInt32Padding; ++i) {
            *cmdIt++ = 0;
        }

        if (TfDebug::IsEnabled(HDST_DRAW)) {
            std::vector<uint32_t>::iterator cmdIt2 = cmdIt - traits.numUInt32;
            std::cout << "   - ";
            while (cmdIt2 != cmdIt) {
                std::cout << *cmdIt2 << " ";
                cmdIt2++;
            }
            std::cout << std::endl;
        }

        _numVisibleItems += instanceCount;
        _numTotalElements += numElements;
        _numTotalVertices += vertexCount;
    }

    TF_DEBUG(HDST_DRAW).Msg(" - Num Visible: %zu\n", _numVisibleItems);
    TF_DEBUG(HDST_DRAW).Msg(" - Total Elements: %zu\n", _numTotalElements);
    TF_DEBUG(HDST_DRAW).Msg(" - Total Verts: %zu\n", _numTotalVertices);

    // make sure we filled all
    TF_VERIFY(cmdIt == _drawCommandBuffer.end());

    // cache the location of instanceCount and cullInstanceCount,
    // to be used during DrawItemInstanceChanged().
    _instanceCountOffset = traits.instanceCount_offset/sizeof(uint32_t);
    _cullInstanceCountOffset = traits.cullInstanceCount_offset/sizeof(uint32_t);

    // cache the offset needed for compute culling.
    _drawCoordOffset = traits.drawingCoord0_offset / sizeof(uint32_t);

    // cache the location of patchBaseVertex for tessellated patch drawing.
    _patchBaseVertexByteOffset = traits.patchBaseVertex_offset;

    // allocate draw dispatch buffer
    _dispatchBuffer =
        resourceRegistry->RegisterDispatchBuffer(_tokens->drawIndirect,
                                                 numDrawItemInstances,
                                                 traits.numUInt32);

    // allocate tessFactors buffer for Metal tessellation
    if (useMetalTessellation &&
        _drawItemInstances[0]->GetDrawItem()->
                GetGeometricShader()->IsPrimTypePatches()) {

        _tessFactorsBuffer = _AllocateTessFactorsBuffer(
                                _drawItemInstances[0]->GetDrawItem(),
                                resourceRegistry);
    }

    // add drawing resource views
    _AddDrawResourceViews(_dispatchBuffer, traits);

    // copy data
    _dispatchBuffer->CopyData(_drawCommandBuffer);

    if (_useGpuCulling) {
        // Make a duplicate of the draw dispatch buffer to use as an input
        // for GPU frustum culling (a single buffer cannot be bound for
        // both reading and writing). We use only the instanceCount
        // and drawingCoord parameters, but it is simplest to just make
        // a copy.
        _dispatchBufferCullInput =
            resourceRegistry->RegisterDispatchBuffer(_tokens->drawIndirectCull,
                                                     numDrawItemInstances,
                                                     traits.numUInt32);

        // copy data
        _dispatchBufferCullInput->CopyData(_drawCommandBuffer);
    }
}

HdSt_DrawBatch::ValidationResult
HdSt_PipelineDrawBatch::Validate(bool deepValidation)
{
    if (!TF_VERIFY(!_drawItemInstances.empty())) {
        return ValidationResult::RebuildAllBatches;
    }

    TF_DEBUG(HDST_DRAW_BATCH).Msg(
        "Validating pipeline draw batch %p (deep validation = %d)...\n",
        (void*)(this), deepValidation);

    // check the hash to see they've been reallocated/migrated or not.
    // note that we just need to compare the hash of the first item,
    // since drawitems are aggregated and ensure that they are sharing
    // same buffer arrays.
    HdStDrawItem const * batchItem = _drawItemInstances.front()->GetDrawItem();
    size_t const bufferArraysHash = batchItem->GetBufferArraysHash();

    if (_bufferArraysHash != bufferArraysHash) {
        _bufferArraysHash = bufferArraysHash;
        TF_DEBUG(HDST_DRAW_BATCH).Msg(
            "   Buffer arrays hash changed. Need to rebuild batch.\n");
        return ValidationResult::RebuildBatch;
    }

    // Deep validation is flagged explicitly when a drawItem has changes to
    // its BARs (e.g. buffer spec, aggregation, element offsets) or when its
    // material network shader or geometric shader changes.
    if (deepValidation) {
        TRACE_SCOPE("Pipeline draw batch deep validation");
        // look through all draw items to be still compatible

        size_t numDrawItemInstances = _drawItemInstances.size();
        size_t barElementOffsetsHash = 0;

        for (size_t item = 0; item < numDrawItemInstances; ++item) {
            HdStDrawItem const * drawItem =
                _drawItemInstances[item]->GetDrawItem();

            if (!TF_VERIFY(drawItem->GetGeometricShader())) {
                return ValidationResult::RebuildAllBatches;
            }

            if (!_IsAggregated(batchItem, drawItem)) {
                 TF_DEBUG(HDST_DRAW_BATCH).Msg(
                    "   Deep validation: Found draw item that fails aggregation"
                    " test. Need to rebuild all batches.\n");
                return ValidationResult::RebuildAllBatches;
            }

            barElementOffsetsHash = TfHash::Combine(barElementOffsetsHash,
                drawItem->GetElementOffsetsHash());
        }

        if (_barElementOffsetsHash != barElementOffsetsHash) {
             TF_DEBUG(HDST_DRAW_BATCH).Msg(
                "   Deep validation: Element offsets hash mismatch."
                "   Rebuilding batch (even though only the dispatch buffer"
                "   needs to be updated)\n.");
            return ValidationResult::RebuildBatch;
        }

    }

    TF_DEBUG(HDST_DRAW_BATCH).Msg(
        "   Validation passed. No need to rebuild batch.\n");
    return ValidationResult::ValidBatch;
}

bool
HdSt_PipelineDrawBatch::_HasNothingToDraw() const
{
    return ( _useDrawIndexed && _numTotalElements == 0) ||
           (!_useDrawIndexed && _numTotalVertices == 0);
}

void
HdSt_PipelineDrawBatch::PrepareDraw(
    HgiGraphicsCmds *gfxCmds,
    HdStRenderPassStateSharedPtr const & renderPassState,
    HdStResourceRegistrySharedPtr const & resourceRegistry)
{
    TRACE_FUNCTION();

    if (!_dispatchBuffer) {
        _CompileBatch(resourceRegistry);
    }

    if (_HasNothingToDraw()) return;

    // Do we have to update our dispatch buffer because drawitem instance
    // data has changed?
    // On the first time through, after batches have just been compiled,
    // the flag will be false because the resource registry will have already
    // uploaded the buffer.
    bool const updateBufferData = _drawCommandBufferDirty;
    if (updateBufferData) {
        _dispatchBuffer->CopyData(_drawCommandBuffer);
        _drawCommandBufferDirty = false;
    }

    if (_useGpuCulling) {
        // Ignore passed in gfxCmds for now since GPU frustum culling
        // may still require multiple command buffer submissions.
        _ExecuteFrustumCull(updateBufferData,
                            renderPassState, resourceRegistry);
    }
}

void
HdSt_PipelineDrawBatch::EncodeDraw(
    HdStRenderPassStateSharedPtr const & renderPassState,
    HdStResourceRegistrySharedPtr const & resourceRegistry,
    bool firstDrawBatch)
{
    if (_HasNothingToDraw()) return;

    Hgi *hgi = resourceRegistry->GetHgi();
    HgiCapabilities const *capabilities = hgi->GetCapabilities();

    // For ICBs on Apple Silicon, we do not support rendering to non-MSAA
    // surfaces, such as OIT as Volumetrics.  Disable in these cases.
    bool const drawICB =
        _allowIndirectCommandEncoding &&
        capabilities->IsSet(HgiDeviceCapabilitiesBitsIndirectCommandBuffers) &&
        renderPassState->GetMultiSampleEnabled();

    _indirectCommands.reset();
    if (drawICB) {
        _PrepareIndirectCommandBuffer(renderPassState, resourceRegistry,
            firstDrawBatch);
    }
}

////////////////////////////////////////////////////////////
// GPU Resource Binding
////////////////////////////////////////////////////////////

namespace {

// Resources to Bind/Unbind for a drawItem
struct _BindingState : public _DrawItemState
{
    _BindingState(
            HdStDrawItem const * drawItem,
            HdStDispatchBufferSharedPtr const & dispatchBuffer,
            HdSt_ResourceBinder const & binder,
            HdStGLSLProgramSharedPtr const & glslProgram,
            HdStShaderCodeSharedPtrVector const & shaders,
            HdSt_GeometricShaderSharedPtr const & geometricShader)
        : _DrawItemState(drawItem)
        , dispatchBuffer(dispatchBuffer)
        , binder(binder)
        , glslProgram(glslProgram)
        , shaders(shaders)
        , geometricShader(geometricShader)
    { }


    // Core resources needed for view transformation & frustum culling.
    void GetBindingsForViewTransformation(
                HgiResourceBindingsDesc * bindingsDesc) const;

    // Core resources plus additional resources needed for drawing.
    void GetBindingsForDrawing(
                HgiResourceBindingsDesc * bindingsDesc,
                HdStBufferResourceSharedPtr const & tessFactorsBuffer,
                bool bindTessFactors) const;

    HdStDispatchBufferSharedPtr dispatchBuffer;
    HdSt_ResourceBinder const & binder;
    HdStGLSLProgramSharedPtr glslProgram;
    HdStShaderCodeSharedPtrVector shaders;
    HdSt_GeometricShaderSharedPtr geometricShader;
};

void
_BindingState::GetBindingsForViewTransformation(
    HgiResourceBindingsDesc * bindingsDesc) const
{
    bindingsDesc->debugName = "PipelineDrawBatch.ViewTransformation";

    // Bind the constant buffer for the prim transformation and bounds.
    binder.GetInterleavedBufferArrayBindingDesc(
        bindingsDesc, constantBar, _tokens->constantPrimvars);

    // Bind the instance buffers to support instance transformations.
    if (instanceIndexBar) {
        for (size_t level = 0; level < instancePrimvarBars.size(); ++level) {
            binder.GetInstanceBufferArrayBindingDesc(
                bindingsDesc, instancePrimvarBars[level], level);
        }
        binder.GetBufferArrayBindingDesc(bindingsDesc, instanceIndexBar);
    }
}

void
_BindingState::GetBindingsForDrawing(
    HgiResourceBindingsDesc * bindingsDesc,
    HdStBufferResourceSharedPtr const & tessFactorsBuffer,
    bool bindTessFactors) const
{
    GetBindingsForViewTransformation(bindingsDesc);

    bindingsDesc->debugName = "PipelineDrawBatch.Drawing";

    binder.GetInterleavedBufferArrayBindingDesc(
        bindingsDesc, topVisBar, HdTokens->topologyVisibility);

    binder.GetBufferArrayBindingDesc(bindingsDesc, indexBar);
    if (!geometricShader->IsPrimTypePoints()) {
        binder.GetBufferArrayBindingDesc(bindingsDesc, elementBar);
        binder.GetBufferArrayBindingDesc(bindingsDesc, fvarBar);
    }
    binder.GetBufferArrayBindingDesc(bindingsDesc, varyingBar);

    if (tessFactorsBuffer) {
        binder.GetBufferBindingDesc(bindingsDesc,
                                    HdTokens->tessFactors,
                                    tessFactorsBuffer,
                                    tessFactorsBuffer->GetOffset());
        if (bindTessFactors) {
            binder.GetBufferBindingDesc(bindingsDesc,
                                        HdTokens->tessFactors,
                                        tessFactorsBuffer,
                                        tessFactorsBuffer->GetOffset());
            HgiBufferBindDesc &tessFactorBuffDesc = bindingsDesc->buffers.back();
            tessFactorBuffDesc.resourceType = HgiBindResourceTypeTessFactors;
        }
    }

    for (HdStShaderCodeSharedPtr const & shader : shaders) {
        HdStBufferArrayRangeSharedPtr shaderBar =
                std::static_pointer_cast<HdStBufferArrayRange>(
                        shader->GetShaderData());

        binder.GetInterleavedBufferArrayBindingDesc(
            bindingsDesc, shaderBar, HdTokens->materialParams);

        HdStBindingRequestVector bindingRequests;
        shader->AddBindings(&bindingRequests);
        for (auto const & req : bindingRequests) {
            binder.GetBindingRequestBindingDesc(bindingsDesc, req);
        }
        HdSt_TextureBinder::GetBindingDescs(
                binder, bindingsDesc, shader->GetNamedTextureHandles());
    }
}

HgiVertexBufferDescVector
_GetVertexBuffersForViewTransformation(_BindingState const & state)
{
    // Bind the dispatchBuffer drawing coordinate resource views
    HdStBufferArrayRangeSharedPtr const dispatchBar =
                state.dispatchBuffer->GetBufferArrayRange();
    size_t const dispatchBufferStride =
                state.dispatchBuffer->GetCommandNumUints()*sizeof(uint32_t);

    HgiVertexAttributeDescVector attrDescVector;

    for (auto const & namedResource : dispatchBar->GetResources()) {
        HdStBinding const binding =
                                state.binder.GetBinding(namedResource.first);
        HdStBufferResourceSharedPtr const & resource = namedResource.second;
        HdTupleType const tupleType = resource->GetTupleType();

        if (binding.GetType() == HdStBinding::DRAW_INDEX_INSTANCE) {
            HgiVertexAttributeDesc attrDesc;
            attrDesc.format =
                HdStHgiConversions::GetHgiVertexFormat(tupleType.type);
            attrDesc.offset = resource->GetOffset(),
            attrDesc.shaderBindLocation = binding.GetLocation();
            attrDescVector.push_back(attrDesc);
        } else if (binding.GetType() ==
                                HdStBinding::DRAW_INDEX_INSTANCE_ARRAY) {
            for (size_t i = 0; i < tupleType.count; ++i) {
                HgiVertexAttributeDesc attrDesc;
                attrDesc.format =
                    HdStHgiConversions::GetHgiVertexFormat(tupleType.type);
                attrDesc.offset = resource->GetOffset() + i*sizeof(uint32_t);
                attrDesc.shaderBindLocation = binding.GetLocation() + i;
                attrDescVector.push_back(attrDesc);
            }
        }
    }

    // All drawing coordinate resources are sourced from the same buffer.
    HgiVertexBufferDesc bufferDesc;
    bufferDesc.bindingIndex = 0;
    bufferDesc.vertexAttributes = attrDescVector;
    bufferDesc.vertexStepFunction = HgiVertexBufferStepFunctionPerDrawCommand;
    bufferDesc.vertexStride = dispatchBufferStride;

    return HgiVertexBufferDescVector{bufferDesc};
}

HgiVertexBufferDescVector
_GetVertexBuffersForDrawing(_BindingState const & state)
{
    // Bind the vertexBar resources
    HgiVertexBufferDescVector vertexBufferDescVector =
        _GetVertexBuffersForViewTransformation(state);

    for (auto const & namedResource : state.vertexBar->GetResources()) {
        HdStBinding const binding =
                                state.binder.GetBinding(namedResource.first);
        HdStBufferResourceSharedPtr const & resource = namedResource.second;
        HdTupleType const tupleType = resource->GetTupleType();

        if (binding.GetType() == HdStBinding::VERTEX_ATTR) {
            HgiVertexAttributeDesc attrDesc;
            attrDesc.format =
                HdStHgiConversions::GetHgiVertexFormat(tupleType.type);
            attrDesc.offset = resource->GetOffset(),
            attrDesc.shaderBindLocation = binding.GetLocation();

            // Each vertexBar resource is sourced from a distinct buffer.
            HgiVertexBufferDesc bufferDesc;
            bufferDesc.bindingIndex = vertexBufferDescVector.size();
            bufferDesc.vertexAttributes = {attrDesc};
            if (state.geometricShader->GetUseMetalTessellation()) {
                bufferDesc.vertexStepFunction =
                    HgiVertexBufferStepFunctionPerPatchControlPoint;
            } else {
                bufferDesc.vertexStepFunction =
                    HgiVertexBufferStepFunctionPerVertex;
            }
            bufferDesc.vertexStride = HdDataSizeOfTupleType(tupleType);
            vertexBufferDescVector.push_back(bufferDesc);
        }
    }

    return vertexBufferDescVector;
}

uint32_t
_GetVertexBufferBindingsForViewTransformation(
    HgiVertexBufferBindingVector *bindings,
    _BindingState const & state)
{
    // Bind the dispatchBuffer drawing coordinate resource views
    HdStBufferResourceSharedPtr resource =
                state.dispatchBuffer->GetEntireResource();
    
    bindings->emplace_back(resource->GetHandle(),
                          (uint32_t)resource->GetOffset(),
                          0);

    return static_cast<uint32_t>(bindings->size());
}

uint32_t
_GetVertexBufferBindingsForDrawing(
    HgiVertexBufferBindingVector *bindings,
    _BindingState const & state)
{
    uint32_t nextBinding = // continue binding subsequent locations
        _GetVertexBufferBindingsForViewTransformation(bindings, state);

    for (auto const & namedResource : state.vertexBar->GetResources()) {
        HdStBinding const binding =
                                state.binder.GetBinding(namedResource.first);
        HdStBufferResourceSharedPtr const & resource = namedResource.second;

        if (binding.GetType() == HdStBinding::VERTEX_ATTR) {
            bindings->emplace_back(resource->GetHandle(),
                                   static_cast<uint32_t>(resource->GetOffset()),
                                   nextBinding);
            nextBinding++;
        }
    }

    return nextBinding;
}

} // annonymous namespace

////////////////////////////////////////////////////////////
// GPU Drawing
////////////////////////////////////////////////////////////

static
HgiGraphicsPipelineSharedPtr
_GetDrawPipeline(
    HdStRenderPassStateSharedPtr const & renderPassState,
    HdStResourceRegistrySharedPtr const & resourceRegistry,
    _BindingState const & state,
    bool firstDrawBatch)
{
    // Drawing pipeline is compatible as long as the shader and
    // pipeline state are the same.
    HgiShaderProgramHandle const & programHandle =
                                        state.glslProgram->GetProgram();

    static const uint64_t salt = ArchHash64(__FUNCTION__, sizeof(__FUNCTION__));
    uint64_t hash = salt;
    hash = TfHash::Combine(hash, programHandle.Get());
    hash = TfHash::Combine(hash, renderPassState->GetGraphicsPipelineHash(
        state.geometricShader, firstDrawBatch));

    HdInstance<HgiGraphicsPipelineSharedPtr> pipelineInstance =
        resourceRegistry->RegisterGraphicsPipeline(hash);

    if (pipelineInstance.IsFirstInstance()) {
        HgiGraphicsPipelineDesc pipeDesc;

        renderPassState->InitGraphicsPipelineDesc(&pipeDesc,
                                                  state.geometricShader,
                                                  firstDrawBatch);

        pipeDesc.shaderProgram = state.glslProgram->GetProgram();
        pipeDesc.vertexBuffers = _GetVertexBuffersForDrawing(state);

        Hgi* hgi = resourceRegistry->GetHgi();
        HgiGraphicsPipelineHandle pso = hgi->CreateGraphicsPipeline(pipeDesc);

        pipelineInstance.SetValue(
            std::make_shared<HgiGraphicsPipelineHandle>(pso));
    }

    return pipelineInstance.GetValue();
}

static
HgiGraphicsPipelineSharedPtr
_GetPTCSPipeline(
    HdStRenderPassStateSharedPtr const & renderPassState,
    HdStResourceRegistrySharedPtr const & resourceRegistry,
    _BindingState const & state,
    bool firstDrawBatch)
{
    // PTCS pipeline is compatible as long as the shader and
    // pipeline state are the same.
    HgiShaderProgramHandle const & programHandle =
                                        state.glslProgram->GetProgram();

    static const uint64_t salt = ArchHash64(__FUNCTION__, sizeof(__FUNCTION__));
    uint64_t hash = salt;
    hash = TfHash::Combine(hash, programHandle.Get());
    hash = TfHash::Combine(hash, renderPassState->GetGraphicsPipelineHash(
        state.geometricShader, firstDrawBatch));

    HdInstance<HgiGraphicsPipelineSharedPtr> pipelineInstance =
        resourceRegistry->RegisterGraphicsPipeline(hash);

    if (pipelineInstance.IsFirstInstance()) {
        HgiGraphicsPipelineDesc pipeDesc;

        renderPassState->InitGraphicsPipelineDesc(&pipeDesc,
                                                  state.geometricShader,
                                                  firstDrawBatch);

        pipeDesc.rasterizationState.rasterizerEnabled = false;
        pipeDesc.multiSampleState.sampleCount = HgiSampleCount1;
        pipeDesc.multiSampleState.alphaToCoverageEnable = false;
        pipeDesc.depthState.depthWriteEnabled = false;
        pipeDesc.depthState.depthTestEnabled = false;
        pipeDesc.depthState.stencilTestEnabled = false;
        pipeDesc.primitiveType = HgiPrimitiveTypePatchList;
        pipeDesc.multiSampleState.multiSampleEnable = false;

        pipeDesc.shaderProgram = state.glslProgram->GetProgram();
        pipeDesc.vertexBuffers = _GetVertexBuffersForDrawing(state);
        pipeDesc.tessellationState.tessFactorMode =
            HgiTessellationState::TessControl;

        Hgi* hgi = resourceRegistry->GetHgi();
        HgiGraphicsPipelineHandle pso = hgi->CreateGraphicsPipeline(pipeDesc);

        pipelineInstance.SetValue(
            std::make_shared<HgiGraphicsPipelineHandle>(pso));
    }

    return pipelineInstance.GetValue();
}

void
HdSt_PipelineDrawBatch::ExecuteDraw(
    HgiGraphicsCmds * gfxCmds,
    HdStRenderPassStateSharedPtr const & renderPassState,
    HdStResourceRegistrySharedPtr const & resourceRegistry,
    bool firstDrawBatch)
{
    TRACE_FUNCTION();

    if (!TF_VERIFY(!_drawItemInstances.empty())) return;

    if (!TF_VERIFY(_dispatchBuffer)) return;

    if (_HasNothingToDraw()) return;

    Hgi *hgi = resourceRegistry->GetHgi();
    HgiCapabilities const *capabilities = hgi->GetCapabilities();

    if (_tessFactorsBuffer) {
        // Metal tessellation tessFactors are computed by PTCS.
        _ExecutePTCS(gfxCmds, renderPassState, resourceRegistry,
            firstDrawBatch);
        // Finish computing tessFactors before drawing.
        gfxCmds->InsertMemoryBarrier(HgiMemoryBarrierAll);
    }

    //
    // If an indirect command buffer was created in the Prepare phase then
    // execute it here.  Otherwise render with the normal graphicsCmd path.
    //
    if (_indirectCommands) {
        HgiIndirectCommandEncoder *encoder = hgi->GetIndirectCommandEncoder();
        encoder->ExecuteDraw(gfxCmds, _indirectCommands.get());

        hgi->DestroyResourceBindings(&(_indirectCommands->resourceBindings));
        _indirectCommands.reset();
    }
    else {
        _DrawingProgram & program = _GetDrawingProgram(renderPassState,
                                                       resourceRegistry);
        if (!TF_VERIFY(program.IsValid())) return;

        _BindingState state(
                _drawItemInstances.front()->GetDrawItem(),
                _dispatchBuffer,
                program.GetBinder(),
                program.GetGLSLProgram(),
                program.GetComposedShaders(),
                program.GetGeometricShader());

        HgiGraphicsPipelineSharedPtr pso =
            _GetDrawPipeline(
                renderPassState,
                resourceRegistry,
                state,
                firstDrawBatch);

        HgiGraphicsPipelineHandle psoHandle = *pso.get();
        gfxCmds->BindPipeline(psoHandle);

        HgiResourceBindingsDesc bindingsDesc;
        state.GetBindingsForDrawing(&bindingsDesc,
                _tessFactorsBuffer, /*bindTessFactors=*/true);

        HgiResourceBindingsHandle resourceBindings =
                hgi->CreateResourceBindings(bindingsDesc);
        gfxCmds->BindResources(resourceBindings);

        HgiVertexBufferBindingVector bindings;
        _GetVertexBufferBindingsForDrawing(&bindings, state);
        gfxCmds->BindVertexBuffers(bindings);
        
        // Drawing can be either direct or indirect. For either case,
        // the drawing batch and drawing program are prepared to resolve
        // drawing coordinate state indirectly, i.e. from buffer data.
        bool const drawIndirect =
            capabilities->IsSet(HgiDeviceCapabilitiesBitsMultiDrawIndirect);

        if (drawIndirect) {
            _ExecuteDrawIndirect(gfxCmds, state.indexBar);
        } else {
            _ExecuteDrawImmediate(gfxCmds, state.indexBar);
        }

        hgi->DestroyResourceBindings(&resourceBindings);
    }

    HD_PERF_COUNTER_INCR(HdPerfTokens->drawCalls);
    HD_PERF_COUNTER_ADD(HdTokens->itemsDrawn, _numVisibleItems);
}

void
HdSt_PipelineDrawBatch::_ExecuteDrawIndirect(
    HgiGraphicsCmds * gfxCmds,
    HdStBufferArrayRangeSharedPtr const & indexBar)
{
    TRACE_FUNCTION();

    HdStBufferResourceSharedPtr paramBuffer = _dispatchBuffer->
        GetBufferArrayRange()->GetResource(HdTokens->drawDispatch);
    if (!TF_VERIFY(paramBuffer)) return;

    if (!_useDrawIndexed) {
        gfxCmds->DrawIndirect(
            paramBuffer->GetHandle(),
            paramBuffer->GetOffset(),
            _dispatchBuffer->GetCount(),
            paramBuffer->GetStride());
    } else {
        HdStBufferResourceSharedPtr indexBuffer =
            indexBar->GetResource(HdTokens->indices);
        if (!TF_VERIFY(indexBuffer)) return;

        gfxCmds->DrawIndexedIndirect(
            indexBuffer->GetHandle(),
            paramBuffer->GetHandle(),
            paramBuffer->GetOffset(),
            _dispatchBuffer->GetCount(),
            paramBuffer->GetStride(),
            _drawCommandBuffer,
            _patchBaseVertexByteOffset);
    }
}

void
HdSt_PipelineDrawBatch::_ExecuteDrawImmediate(
    HgiGraphicsCmds * gfxCmds,
    HdStBufferArrayRangeSharedPtr const & indexBar)
{
    TRACE_FUNCTION();

    uint32_t const drawCount = _dispatchBuffer->GetCount();
    uint32_t const strideUInt32 = _dispatchBuffer->GetCommandNumUints();

    if (!_useDrawIndexed) {
        for (uint32_t i = 0; i < drawCount; ++i) {
            _DrawNonIndexedCommand const * cmd =
                reinterpret_cast<_DrawNonIndexedCommand*>(
                    &_drawCommandBuffer[i * strideUInt32]);

            if (cmd->common.count && cmd->common.instanceCount) {
                gfxCmds->Draw(
                    cmd->common.count,
                    cmd->common.baseVertex,
                    cmd->common.instanceCount,
                    cmd->common.baseInstance);
            }
        }
    } else {
        HdStBufferResourceSharedPtr indexBuffer =
            indexBar->GetResource(HdTokens->indices);
        if (!TF_VERIFY(indexBuffer)) return;

        bool const useMetalTessellation =
            _drawItemInstances[0]->GetDrawItem()->
                    GetGeometricShader()->GetUseMetalTessellation();

        for (uint32_t i = 0; i < drawCount; ++i) {
            _DrawIndexedCommand const * cmd =
                reinterpret_cast<_DrawIndexedCommand*>(
                    &_drawCommandBuffer[i * strideUInt32]);

            uint32_t const indexBufferByteOffset =
                static_cast<uint32_t>(cmd->common.baseIndex * sizeof(uint32_t));

            if (cmd->common.count && cmd->common.instanceCount) {
                if (useMetalTessellation) {
                    gfxCmds->DrawIndexed(
                        indexBuffer->GetHandle(),
                        cmd->metalPatch.patchCount,
                        indexBufferByteOffset,
                        cmd->metalPatch.baseVertex,
                        cmd->metalPatch.instanceCount,
                        cmd->metalPatch.baseInstance);
                } else {
                    gfxCmds->DrawIndexed(
                        indexBuffer->GetHandle(),
                        cmd->common.count,
                        indexBufferByteOffset,
                        cmd->common.baseVertex,
                        cmd->common.instanceCount,
                        cmd->common.baseInstance);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////
// GPU Frustum Culling
////////////////////////////////////////////////////////////

static
HgiComputePipelineSharedPtr
_GetCullPipeline(
    HdStResourceRegistrySharedPtr const & resourceRegistry,
    _BindingState const & state,
    size_t byteSizeUniforms)
{
    // Culling pipeline is compatible as long as the shader is the same.
    HgiShaderProgramHandle const & programHandle =
                                        state.glslProgram->GetProgram();
    uint64_t const hash = reinterpret_cast<uint64_t>(programHandle.Get());

    HdInstance<HgiComputePipelineSharedPtr> pipelineInstance =
        resourceRegistry->RegisterComputePipeline(hash);

    if (pipelineInstance.IsFirstInstance()) {
        // Create a points primitive, vertex shader only pipeline that uses
        // a uniform block data for the 'cullParams' in the shader.
        HgiComputePipelineDesc pipeDesc;
        pipeDesc.debugName = "FrustumCulling";
        pipeDesc.shaderProgram = programHandle;
        pipeDesc.shaderConstantsDesc.byteSize = byteSizeUniforms;

        Hgi* hgi = resourceRegistry->GetHgi();
        HgiComputePipelineSharedPtr pipe =
            std::make_shared<HgiComputePipelineHandle>(
                hgi->CreateComputePipeline(pipeDesc));

        pipelineInstance.SetValue(pipe);
    }

    return pipelineInstance.GetValue();
}

void
HdSt_PipelineDrawBatch::_PrepareIndirectCommandBuffer(
    HdStRenderPassStateSharedPtr const & renderPassState,
    HdStResourceRegistrySharedPtr const & resourceRegistry,
    bool firstDrawBatch)
{
    Hgi *hgi = resourceRegistry->GetHgi();
    _DrawingProgram & program = _GetDrawingProgram(renderPassState,
                                                   resourceRegistry);
    if (!TF_VERIFY(program.IsValid())) return;

    _BindingState state(
            _drawItemInstances.front()->GetDrawItem(),
            _dispatchBuffer,
            program.GetBinder(),
            program.GetGLSLProgram(),
            program.GetComposedShaders(),
            program.GetGeometricShader());

    HgiGraphicsPipelineSharedPtr pso =
        _GetDrawPipeline(
            renderPassState,
            resourceRegistry,
            state,
            firstDrawBatch);
    
    HgiGraphicsPipelineHandle psoHandle = *pso.get();

    HgiResourceBindingsDesc bindingsDesc;
    state.GetBindingsForDrawing(&bindingsDesc,
            _tessFactorsBuffer, /*bindTessFactors=*/true);

    HgiResourceBindingsHandle resourceBindings =
            hgi->CreateResourceBindings(bindingsDesc);
    
    HgiVertexBufferBindingVector vertexBindings;
    _GetVertexBufferBindingsForDrawing(&vertexBindings, state);

    HdStBufferResourceSharedPtr paramBuffer = _dispatchBuffer->
        GetBufferArrayRange()->GetResource(HdTokens->drawDispatch);
    
    HgiIndirectCommandEncoder *encoder = hgi->GetIndirectCommandEncoder();
    HgiComputeCmds *computeCmds =
        resourceRegistry->GetGlobalComputeCmds(HgiComputeDispatchConcurrent);

    if (!_useDrawIndexed) {
        _indirectCommands = encoder->EncodeDraw(
            computeCmds,
            psoHandle,
            resourceBindings,
            vertexBindings,
            paramBuffer->GetHandle(),
            paramBuffer->GetOffset(),
            _dispatchBuffer->GetCount(),
            paramBuffer->GetStride());
    } else {
        HdStBufferResourceSharedPtr indexBuffer =
            state.indexBar->GetResource(HdTokens->indices);

        _indirectCommands = encoder->EncodeDrawIndexed(
            computeCmds,
            psoHandle,
            resourceBindings,
            vertexBindings,
            indexBuffer->GetHandle(),
            paramBuffer->GetHandle(),
            paramBuffer->GetOffset(),
            _dispatchBuffer->GetCount(),
            paramBuffer->GetStride(),
            _patchBaseVertexByteOffset);
    }
}

void
HdSt_PipelineDrawBatch::_ExecuteFrustumCull(
    bool const updateBufferData,
    HdStRenderPassStateSharedPtr const & renderPassState,
    HdStResourceRegistrySharedPtr const & resourceRegistry)
{
    TRACE_FUNCTION();

    // Disable GPU culling when instancing enabled and
    // not using instance culling.
    if (_useInstancing && !_useInstanceCulling) return;

    // Bypass freezeCulling if the command buffer is dirty.
    bool const freezeCulling = TfDebug::IsEnabled(HD_FREEZE_CULL_FRUSTUM);
    if (freezeCulling && !updateBufferData) return;

    if (updateBufferData) {
        _dispatchBufferCullInput->CopyData(_drawCommandBuffer);
    }

    _CreateCullingProgram(resourceRegistry);
    if (!TF_VERIFY(_cullingProgram.IsValid())) return;

    struct Uniforms {
        GfMatrix4f cullMatrix;
        GfVec2f drawRangeNDC;
        uint32_t drawCommandNumUints;
    };

    // We perform frustum culling in a compute shader, stomping the
    // instanceCount of each drawing command in the dispatch buffer to 0 for
    // primitives that are culled, skipping over other elements.

    _BindingState state(
            _drawItemInstances.front()->GetDrawItem(),
            _dispatchBufferCullInput,
            _cullingProgram.GetBinder(),
            _cullingProgram.GetGLSLProgram(),
            _cullingProgram.GetComposedShaders(),
            _cullingProgram.GetGeometricShader());

    Hgi * hgi = resourceRegistry->GetHgi();

    HgiComputePipelineSharedPtr const & pso =
        _GetCullPipeline(resourceRegistry,
                         state,
                         sizeof(Uniforms));
    HgiComputePipelineHandle psoHandle = *pso.get();

    HgiComputeCmds* computeCmds =
        resourceRegistry->GetGlobalComputeCmds(HgiComputeDispatchConcurrent);
    computeCmds->PushDebugGroup("FrustumCulling Cmds");

    HgiResourceBindingsDesc bindingsDesc;
    state.GetBindingsForViewTransformation(&bindingsDesc);

    if (IsEnabledGPUCountVisibleInstances()) {
        _BeginGPUCountVisibleInstances(resourceRegistry);
        state.binder.GetBufferBindingDesc(
                &bindingsDesc,
                _tokens->drawIndirectResult,
                _resultBuffer,
                _resultBuffer->GetOffset());
                
    }

    // bind destination buffer
    // (using entire buffer bind to start from offset=0)
    state.binder.GetBufferBindingDesc(
            &bindingsDesc,
            _tokens->dispatchBuffer,
            _dispatchBuffer->GetEntireResource(),
            _dispatchBuffer->GetEntireResource()->GetOffset());

    // bind the read-only copy of the destination buffer for input.
    state.binder.GetBufferBindingDesc(
            &bindingsDesc,
            _tokens->drawCullInput,
            _dispatchBufferCullInput->GetEntireResource(),
            _dispatchBufferCullInput->GetEntireResource()->GetOffset());

    // HdSt_ResourceBinder::GetBufferBindingDesc() sets state usage
    // to all graphics pipeline stages. Instead we have to set all the
    // buffer stage usage to Compute.
    for (HgiBufferBindDesc & bufDesc : bindingsDesc.buffers) {
        bufDesc.stageUsage = HgiShaderStageCompute;
        bufDesc.writable = true;
    }

    HgiResourceBindingsHandle resourceBindings =
            hgi->CreateResourceBindings(bindingsDesc);

    computeCmds->BindResources(resourceBindings);
    computeCmds->BindPipeline(psoHandle);

    GfMatrix4f const &cullMatrix = GfMatrix4f(renderPassState->GetCullMatrix());
    GfVec2f const &drawRangeNdc = renderPassState->GetDrawingRangeNDC();

    HdStBufferResourceSharedPtr paramBuffer = _dispatchBuffer->
        GetBufferArrayRange()->GetResource(HdTokens->drawDispatch);

    // set instanced cull parameters
    Uniforms cullParams;
    cullParams.cullMatrix = cullMatrix;
    cullParams.drawRangeNDC = drawRangeNdc;
    cullParams.drawCommandNumUints = _dispatchBuffer->GetCommandNumUints();

    computeCmds->SetConstantValues(
        psoHandle, 0,
        sizeof(Uniforms), &cullParams);

    int const inputCount = _dispatchBufferCullInput->GetCount();
    computeCmds->Dispatch(inputCount, 1);
    computeCmds->PopDebugGroup();

    if (IsEnabledGPUCountVisibleInstances()) {
        _EndGPUCountVisibleInstances(resourceRegistry, &_numVisibleItems);
    }

    hgi->DestroyResourceBindings(&resourceBindings);
}


void
HdSt_PipelineDrawBatch::_ExecutePTCS(
    HgiGraphicsCmds *ptcsGfxCmds,
    HdStRenderPassStateSharedPtr const & renderPassState,
    HdStResourceRegistrySharedPtr const & resourceRegistry,
    bool firstDrawBatch)
{
    TRACE_FUNCTION();

    if (!TF_VERIFY(!_drawItemInstances.empty())) return;

    if (!TF_VERIFY(_dispatchBuffer)) return;

    if (_HasNothingToDraw()) return;

    HgiCapabilities const *capabilities =
        resourceRegistry->GetHgi()->GetCapabilities();

    // Drawing can be either direct or indirect. For either case,
    // the drawing batch and drawing program are prepared to resolve
    // drawing coordinate state indirectly, i.e. from buffer data.
    bool const drawIndirect =
            capabilities->IsSet(HgiDeviceCapabilitiesBitsMultiDrawIndirect);
    _DrawingProgram & program = _GetDrawingProgram(renderPassState,
                                                   resourceRegistry);
    if (!TF_VERIFY(program.IsValid())) return;

    _BindingState state(
            _drawItemInstances.front()->GetDrawItem(),
            _dispatchBuffer,
            program.GetBinder(),
            program.GetGLSLProgram(),
            program.GetComposedShaders(),
            program.GetGeometricShader());

    Hgi * hgi = resourceRegistry->GetHgi();

    HgiGraphicsPipelineSharedPtr const & psoTess =
        _GetPTCSPipeline(renderPassState,
                         resourceRegistry,
                         state,
                         firstDrawBatch);

    HgiGraphicsPipelineHandle psoTessHandle = *psoTess.get();
    ptcsGfxCmds->BindPipeline(psoTessHandle);

    HgiResourceBindingsDesc bindingsDesc;
    state.GetBindingsForDrawing(&bindingsDesc,
            _tessFactorsBuffer, /*bindTessFactors=*/false);

    HgiResourceBindingsHandle resourceBindings =
            hgi->CreateResourceBindings(bindingsDesc);
    ptcsGfxCmds->BindResources(resourceBindings);

    HgiVertexBufferBindingVector bindings;
    _GetVertexBufferBindingsForDrawing(&bindings, state);
    ptcsGfxCmds->BindVertexBuffers(bindings);

    if (drawIndirect) {
        _ExecuteDrawIndirect(ptcsGfxCmds, state.indexBar);
    } else {
        _ExecuteDrawImmediate(ptcsGfxCmds, state.indexBar);
    }

    hgi->DestroyResourceBindings(&resourceBindings);
}

void
HdSt_PipelineDrawBatch::DrawItemInstanceChanged(
        HdStDrawItemInstance const * instance)
{
    // We need to check the visibility and update if needed
    if (!_dispatchBuffer) return;

    size_t const batchIndex = instance->GetBatchIndex();
    int const commandNumUints = _dispatchBuffer->GetCommandNumUints();
    int const numLevels = instance->GetDrawItem()->GetInstancePrimvarNumLevels();
    int const instanceIndexWidth = numLevels + 1;

    // When non-instance culling is being used, cullcommand points the same
    // location as drawcommands. Then we update the same place twice, it
    // might be better than branching.
    std::vector<uint32_t>::iterator instanceCountIt =
        _drawCommandBuffer.begin()
        + batchIndex * commandNumUints
        + _instanceCountOffset;
    std::vector<uint32_t>::iterator cullInstanceCountIt =
        _drawCommandBuffer.begin()
        + batchIndex * commandNumUints
        + _cullInstanceCountOffset;

    HdBufferArrayRangeSharedPtr const &instanceIndexBar_ =
        instance->GetDrawItem()->GetInstanceIndexRange();
    HdStBufferArrayRangeSharedPtr instanceIndexBar =
        std::static_pointer_cast<HdStBufferArrayRange>(instanceIndexBar_);

    uint32_t const newInstanceCount =
        _GetInstanceCount(instance, instanceIndexBar, instanceIndexWidth);

    TF_DEBUG(HDST_DRAW).Msg("\nInstance Count changed: %d -> %d\n",
            *instanceCountIt,
            newInstanceCount);

    // Update instance count and overall count of visible items.
    if (static_cast<size_t>(newInstanceCount) != (*instanceCountIt)) {
        _numVisibleItems += (newInstanceCount - (*instanceCountIt));
        *instanceCountIt = newInstanceCount;
        *cullInstanceCountIt = newInstanceCount;
        _drawCommandBufferDirty = true;
    }
}

void
HdSt_PipelineDrawBatch::_BeginGPUCountVisibleInstances(
    HdStResourceRegistrySharedPtr const & resourceRegistry)
{
    if (!_resultBuffer) {
        HdTupleType tupleType;
        tupleType.type = HdTypeInt32;
        tupleType.count = 1;

        _resultBuffer =
            resourceRegistry->RegisterBufferResource(
                _tokens->drawIndirectResult, tupleType, HgiBufferUsageStorage);
    }

    // Reset visible item count
    static const int32_t count = 0;
    HgiBlitCmds* blitCmds = resourceRegistry->GetGlobalBlitCmds();
    HgiBufferCpuToGpuOp op;
    op.cpuSourceBuffer = &count;
    op.sourceByteOffset = 0;
    op.gpuDestinationBuffer = _resultBuffer->GetHandle();
    op.destinationByteOffset = 0;
    op.byteSize = sizeof(count);
    blitCmds->CopyBufferCpuToGpu(op);

    // For now we need to submit here, because there gfx commands after
    // _BeginGPUCountVisibleInstances that rely on this having executed on GPU.
    resourceRegistry->SubmitBlitWork();
}

void
HdSt_PipelineDrawBatch::_EndGPUCountVisibleInstances(
    HdStResourceRegistrySharedPtr const & resourceRegistry,
    size_t * result)
{
    // Submit and wait for all the work recorded up to this point.
    // The GPU work must complete before we can read-back the GPU buffer.
    resourceRegistry->SubmitComputeWork(HgiSubmitWaitTypeWaitUntilCompleted);

    int32_t count = 0;

    // Submit GPU buffer read back
    HgiBufferGpuToCpuOp copyOp;
    copyOp.byteSize = sizeof(count);
    copyOp.cpuDestinationBuffer = &count;
    copyOp.destinationByteOffset = 0;
    copyOp.gpuSourceBuffer = _resultBuffer->GetHandle();
    copyOp.sourceByteOffset = 0;

    HgiBlitCmds* blitCmds = resourceRegistry->GetGlobalBlitCmds();
    blitCmds->CopyBufferGpuToCpu(copyOp);
    resourceRegistry->SubmitBlitWork(HgiSubmitWaitTypeWaitUntilCompleted);

    *result = count;
}

void
HdSt_PipelineDrawBatch::_CreateCullingProgram(
    HdStResourceRegistrySharedPtr const & resourceRegistry)
{
    if (!_cullingProgram.GetGLSLProgram() || _dirtyCullingProgram) {
        // Create a culling compute shader key
        HdSt_CullingComputeShaderKey shaderKey(_useInstanceCulling,
            _useTinyPrimCulling,
            IsEnabledGPUCountVisibleInstances());

        // access the drawing coord from the drawCullInput buffer
        _CullingProgram::DrawingCoordBufferBinding drawingCoordBufferBinding{
            _tokens->drawCullInput,
            uint32_t(_drawCoordOffset),
            uint32_t(_dispatchBuffer->GetCommandNumUints()),
        };

        // sharing the culling geometric shader for the same configuration.
        HdSt_GeometricShaderSharedPtr cullShader =
            HdSt_GeometricShader::Create(shaderKey, resourceRegistry);
        _cullingProgram.SetDrawingCoordBufferBinding(drawingCoordBufferBinding);
        _cullingProgram.SetGeometricShader(cullShader);

        _cullingProgram.CompileShader(_drawItemInstances.front()->GetDrawItem(),
                                      resourceRegistry);

        _dirtyCullingProgram = false;
    }
}

void
HdSt_PipelineDrawBatch::_CullingProgram::Initialize(
    bool useDrawIndexed,
    bool useInstanceCulling,
    size_t bufferArrayHash)
{
    if (useDrawIndexed     != _useDrawIndexed     ||
        useInstanceCulling != _useInstanceCulling ||
        bufferArrayHash    != _bufferArrayHash) {
        // reset shader
        Reset();
    }

    _useDrawIndexed = useDrawIndexed;
    _useInstanceCulling = useInstanceCulling;
    _bufferArrayHash = bufferArrayHash;
}

/* virtual */
void
HdSt_PipelineDrawBatch::_CullingProgram::_GetCustomBindings(
    HdStBindingRequestVector * customBindings,
    bool * enableInstanceDraw) const
{
    if (!TF_VERIFY(enableInstanceDraw) ||
        !TF_VERIFY(customBindings)) return;

    customBindings->push_back(HdStBindingRequest(HdStBinding::SSBO,
                                  _tokens->drawIndirectResult));
    customBindings->push_back(HdStBindingRequest(HdStBinding::SSBO,
                                  _tokens->dispatchBuffer));
    customBindings->push_back(HdStBindingRequest(HdStBinding::UBO,
                                  _tokens->ulocCullParams));
    customBindings->push_back(HdStBindingRequest(HdStBinding::SSBO,
                                  _tokens->drawCullInput));

    // set instanceDraw true if instanceCulling is enabled.
    // this value will be used to determine if glVertexAttribDivisor needs to
    // be enabled or not.
    *enableInstanceDraw = _useInstanceCulling;
}

PXR_NAMESPACE_CLOSE_SCOPE
