//
// Copyright 2016 Pixar
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
#include "pxr/imaging/garch/glApi.h"

// XXX We do not want to include specific HgiXX backends, but we need to do
// this temporarily until Storm has transitioned fully to Hgi.
#include "pxr/imaging/hgiGL/graphicsCmds.h"

#include "pxr/imaging/hdSt/bufferArrayRange.h"
#include "pxr/imaging/hdSt/commandBuffer.h"
#include "pxr/imaging/hdSt/cullingShaderKey.h"
#include "pxr/imaging/hdSt/debugCodes.h"
#include "pxr/imaging/hdSt/drawItemInstance.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/glConversions.h"
#include "pxr/imaging/hdSt/materialNetworkShader.h"
#include "pxr/imaging/hdSt/indirectDrawBatch.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hdSt/shaderKey.h"

#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/debugCodes.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hgi/blitCmds.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/hio/glslfx.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/staticTokens.h"

#include <iostream>
#include <limits>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (dispatchBuffer)

    (drawCommandIndex)
    (drawIndirect)
    (drawIndirectCull)
    (drawIndirectResult)

    (instanceCountInput)

    (ulocCullParams)
);


TF_DEFINE_ENV_SETTING(HD_ENABLE_GPU_FRUSTUM_CULLING, true,
                      "Enable GPU frustum culling");
TF_DEFINE_ENV_SETTING(HD_ENABLE_GPU_COUNT_VISIBLE_INSTANCES, false,
                      "Enable GPU frustum culling visible count query");
TF_DEFINE_ENV_SETTING(HD_ENABLE_GPU_INSTANCE_FRUSTUM_CULLING, true,
                      "Enable GPU per-instance frustum culling");

HdSt_IndirectDrawBatch::HdSt_IndirectDrawBatch(
    HdStDrawItemInstance * drawItemInstance)
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

    , _instanceCountOffset(0)
    , _cullInstanceCountOffset(0)
{
    _Init(drawItemInstance);
}

/*virtual*/
void
HdSt_IndirectDrawBatch::_Init(HdStDrawItemInstance * drawItemInstance)
{
    HdSt_DrawBatch::_Init(drawItemInstance);
    drawItemInstance->SetBatchIndex(0);
    drawItemInstance->SetBatch(this);

    // remember buffer arrays version for dispatch buffer updating
    HdStDrawItem const* drawItem = drawItemInstance->GetDrawItem();
    _bufferArraysHash = drawItem->GetBufferArraysHash();
    // _barElementOffsetsHash is updated during _CompileBatch
    _barElementOffsetsHash = 0;

    // determine drawing and culling config according to the first drawitem
    _useDrawIndexed = static_cast<bool>(drawItem->GetTopologyRange());
    _useInstancing  = static_cast<bool>(drawItem->GetInstanceIndexRange());
    _useGpuCulling  = IsEnabledGPUFrustumCulling();

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

HdSt_IndirectDrawBatch::_CullingProgram &
HdSt_IndirectDrawBatch::_GetCullingProgram(
    HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    if (!_cullingProgram.GetGLSLProgram() || _dirtyCullingProgram) {
        // create a culling shader key
        HdSt_CullingShaderKey shaderKey(_useInstanceCulling,
            _useTinyPrimCulling,
            IsEnabledGPUCountVisibleInstances());

        // sharing the culling geometric shader for the same configuration.
        HdSt_GeometricShaderSharedPtr cullShader =
            HdSt_GeometricShader::Create(shaderKey, resourceRegistry);
        _cullingProgram.SetGeometricShader(cullShader);

        _cullingProgram.CompileShader(_drawItemInstances.front()->GetDrawItem(),
                                      /*indirect=*/true,
                                       resourceRegistry);

        _dirtyCullingProgram = false;
    }
    return _cullingProgram;
}

HdSt_IndirectDrawBatch::~HdSt_IndirectDrawBatch()
{
}

void
HdSt_IndirectDrawBatch::SetEnableTinyPrimCulling(bool tinyPrimCulling)
{
    if (_useTinyPrimCulling != tinyPrimCulling) {
        _useTinyPrimCulling = tinyPrimCulling;
        _dirtyCullingProgram = true;
    }
}

/* static */
bool
HdSt_IndirectDrawBatch::IsEnabledGPUFrustumCulling()
{
    // GPU frustum culling requires SSBO of bindless buffer

    static bool isEnabledGPUFrustumCulling =
        TfGetEnvSetting(HD_ENABLE_GPU_FRUSTUM_CULLING);
    return isEnabledGPUFrustumCulling &&
       !TfDebug::IsEnabled(HDST_DISABLE_FRUSTUM_CULLING);
}

/* static */
bool
HdSt_IndirectDrawBatch::IsEnabledGPUCountVisibleInstances()
{
    static bool isEnabledGPUCountVisibleInstances =
        TfGetEnvSetting(HD_ENABLE_GPU_COUNT_VISIBLE_INSTANCES);
    return isEnabledGPUCountVisibleInstances;
}

/* static */
bool
HdSt_IndirectDrawBatch::IsEnabledGPUInstanceFrustumCulling()
{
    // GPU instance frustum culling requires SSBO of bindless buffer

    static bool isEnabledGPUInstanceFrustumCulling =
        TfGetEnvSetting(HD_ENABLE_GPU_INSTANCE_FRUSTUM_CULLING);
    return isEnabledGPUInstanceFrustumCulling;
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
// match the layout of Vulkan and GL indirect draw buffers.
//
// Note: GL specifies baseVertex as 'int' and other as 'uint', but
// we never set negative baseVertex in our use cases.

// DrawingCoord 10 integers (+ numInstanceLevels)
struct _DrawingCoord
{
    // drawingCoord0 (ivec4 for for drawing and culling)
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
    uint32_t count;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t baseInstance;

    _DrawingCoord drawingCoord;
};

// DrawNonIndexed + Instance culling : 18 integers (+ numInstanceLevels)
struct _DrawNonIndexedInstanceCullCommand
{
    uint32_t count;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t baseInstance;

    uint32_t cullCount;
    uint32_t cullInstanceCount;
    uint32_t cullFirstVertex;
    uint32_t cullBaseInstance;

    _DrawingCoord drawingCoord;
};

// DrawIndexed + non-instance culling : 15 integers (+ numInstanceLevels)
struct _DrawIndexedCommand
{
    uint32_t count;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t baseVertex;
    uint32_t baseInstance;

    _DrawingCoord drawingCoord;
};

// DrawIndexed + Instance culling : 19 integers (+ numInstanceLevels)
struct _DrawIndexedInstanceCullCommand
{
    uint32_t count;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t baseVertex;
    uint32_t baseInstance;

    uint32_t cullCount;
    uint32_t cullInstanceCount;
    uint32_t cullFirstVertex;
    uint32_t cullBaseInstance;

    _DrawingCoord drawingCoord;
};

// These traits capture sizes and offsets for the _Draw*Command structs
struct _DrawCommandTraits
{
    // Since the underlying buffer is an array of uint32_t, we capture
    // the size of the struct as the number of uint32_t elements.
    size_t numUInt32;

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
};

template <typename CmdType>
void _SetDrawCommandTraits(_DrawCommandTraits * traits, int instancerNumLevels)
{
    // Number of uint32_t in the command struct
    // followed by instanceDC[instancerNumLevals]
    traits->numUInt32 = sizeof(CmdType) / sizeof(uint32_t)
                      + instancerNumLevels;

    traits->instancerNumLevels = instancerNumLevels;
    traits->instanceIndexWidth = instancerNumLevels + 1;

    traits->count_offset = offsetof(CmdType, count);
    traits->instanceCount_offset = offsetof(CmdType, instanceCount);
    traits->baseInstance_offset = offsetof(CmdType, baseInstance);

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
}

_DrawCommandTraits
_GetDrawCommandTraits(int const instancerNumLevels,
                      bool const useDrawIndexed,
                      bool const useInstanceCulling)
{
    _DrawCommandTraits traits;
    if (!useDrawIndexed) {
        if (useInstanceCulling) {
            using CmdType = _DrawNonIndexedInstanceCullCommand;
            _SetDrawCommandTraits<CmdType>(&traits, instancerNumLevels);
            _SetInstanceCullTraits<CmdType>(&traits);
            _SetDrawingCoordTraits<CmdType>(&traits);
        } else {
            using CmdType = _DrawNonIndexedCommand;
            _SetDrawCommandTraits<CmdType>(&traits, instancerNumLevels);
            _SetDrawingCoordTraits<CmdType>(&traits);
        }
    } else {
        if (useInstanceCulling) {
            using CmdType = _DrawIndexedInstanceCullCommand;
            _SetDrawCommandTraits<CmdType>(&traits, instancerNumLevels);
            _SetInstanceCullTraits<CmdType>(&traits);
            _SetDrawingCoordTraits<CmdType>(&traits);
        } else {
            using CmdType = _DrawIndexedCommand;
            _SetDrawCommandTraits<CmdType>(&traits, instancerNumLevels);
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

void
_AddInstanceCullResourceViews(HdStDispatchBufferSharedPtr const & cullInput,
                              _DrawCommandTraits const & traits)
{
    // cull indirect command
    cullInput->AddBufferResourceView(
        HdTokens->drawDispatch, {HdTypeInt32, 1},
        traits.cullCount_offset);
    // cull drawing coord 0
    cullInput->AddBufferResourceView(
        HdTokens->drawingCoord0, {HdTypeInt32Vec4, 1},
        traits.drawingCoord0_offset);
    // cull drawing coord 1
    cullInput->AddBufferResourceView(
        // see the comment above
        HdTokens->drawingCoord1, {HdTypeInt32Vec2, 1},
        traits.drawingCoord1_offset);
    // cull instance drawing coord
    if (traits.instancerNumLevels > 0) {
        cullInput->AddBufferResourceView(
            HdTokens->drawingCoordI, {HdTypeInt32, traits.instancerNumLevels},
            traits.drawingCoordI_offset);
    }
    // cull draw index
    cullInput->AddBufferResourceView(
        _tokens->drawCommandIndex, {HdTypeInt32, 1},
        traits.baseInstance_offset);
}

void
_AddNonInstanceCullResourceViews(HdStDispatchBufferSharedPtr const & cullInput,
                                 _DrawCommandTraits const & traits)
{
    // cull indirect command
    cullInput->AddBufferResourceView(
        HdTokens->drawDispatch, {HdTypeInt32, 1},
        traits.count_offset);
    // cull drawing coord 0
    cullInput->AddBufferResourceView(
        HdTokens->drawingCoord0, {HdTypeInt32Vec4, 1},
        traits.drawingCoord0_offset);
    // cull draw index
    cullInput->AddBufferResourceView(
        _tokens->drawCommandIndex, {HdTypeInt32, 1},
        traits.baseInstance_offset);
    // cull instance count input
    cullInput->AddBufferResourceView(
        _tokens->instanceCountInput, {HdTypeInt32, 1},
        traits.instanceCount_offset);
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
                    drawItem->GetMaterialNetworkShader()->GetShaderData()))
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
                  int instanceIndexWidth)
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

} // annonymous namespace

void
HdSt_IndirectDrawBatch::_CompileBatch(
    HdStResourceRegistrySharedPtr const & resourceRegistry)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (_drawItemInstances.empty()) return;

    size_t const numDrawItemInstances = _drawItemInstances.size();

    size_t const instancerNumLevels =
        _drawItemInstances[0]->GetDrawItem()->GetInstancePrimvarNumLevels();

    // Get the layout of the command buffer we are building.
    _DrawCommandTraits const traits =
        _GetDrawCommandTraits(instancerNumLevels,
                              _useDrawIndexed, _useInstanceCulling);

    TF_DEBUG(HD_MDI).Msg("\nCompile Dispatch Buffer\n");
    TF_DEBUG(HD_MDI).Msg(" - numuint32: %zd\n", traits.numUInt32);
    TF_DEBUG(HD_MDI).Msg(" - useDrawIndexed: %d\n", _useDrawIndexed);
    TF_DEBUG(HD_MDI).Msg(" - useInstanceCulling: %d\n", _useInstanceCulling);
    TF_DEBUG(HD_MDI).Msg(" - num draw items: %zu\n", numDrawItemInstances);

    _drawCommandBuffer.resize(numDrawItemInstances * traits.numUInt32);
    std::vector<uint32_t>::iterator cmdIt = _drawCommandBuffer.begin();

    // Count the number of visible items. We may actually draw fewer
    // items than this when GPU frustum culling is active.
    _numVisibleItems = 0;
    _numTotalElements = 0;
    _numTotalVertices = 0;

    TF_DEBUG(HD_MDI).Msg(" - Processing Items:\n");
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

        uint32_t const firstVertex = vertexDC;
        uint32_t const vertexCount = _GetElementCount(dc.vertexBar);

        // if delegate fails to get vertex primvars, it could be empty.
        // skip the drawitem to prevent drawing uninitialized vertices.
        uint32_t const numElements =
            vertexCount != 0 ? _GetElementCount(dc.indexBar) : 0;

        uint32_t const firstIndex = primitiveDC * numIndicesPerPrimitive;
        uint32_t const indicesCount = numElements * numIndicesPerPrimitive;

        uint32_t const instanceCount =
            _GetInstanceCount(drawItemInstance,
                              dc.instanceIndexBar,
                              traits.instanceIndexWidth);

        uint32_t const baseInstance = (uint32_t)item;

        // draw command
        if (!_useDrawIndexed) {
            if (_useInstanceCulling) {
                // _DrawNonIndexedInstanceCullCommand
                *cmdIt++ = vertexCount;
                *cmdIt++ = instanceCount;
                *cmdIt++ = firstVertex;
                *cmdIt++ = baseInstance;

                *cmdIt++ = 1;             /* cullCount (always 1) */
                *cmdIt++ = instanceCount; /* cullInstanceCount */
                *cmdIt++ = 0;             /* cullFirstVertex (not used)*/
                *cmdIt++ = baseInstance;  /* cullBaseInstance */
            } else {
                // _DrawNonIndexedCommand
                *cmdIt++ = vertexCount;
                *cmdIt++ = instanceCount;
                *cmdIt++ = firstVertex;
                *cmdIt++ = baseInstance;
            }
        } else {
            if (_useInstanceCulling) {
                // _DrawIndexedInstanceCullCommand
                *cmdIt++ = indicesCount;
                *cmdIt++ = instanceCount;
                *cmdIt++ = firstIndex;
                *cmdIt++ = firstVertex;
                *cmdIt++ = baseInstance;

                *cmdIt++ = 1;             /* cullCount (always 1) */
                *cmdIt++ = instanceCount; /* cullInstanceCount */
                *cmdIt++ = 0;             /* cullFirstVertex (not used)*/
                *cmdIt++ = baseInstance;  /* cullBaseInstance */
            } else {
                // _DrawIndexedCommand
                *cmdIt++ = indicesCount;
                *cmdIt++ = instanceCount;
                *cmdIt++ = firstIndex;
                *cmdIt++ = firstVertex;
                *cmdIt++ = baseInstance;
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

        if (TfDebug::IsEnabled(HD_MDI)) {
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

    TF_DEBUG(HD_MDI).Msg(" - Num Visible: %zu\n", _numVisibleItems);
    TF_DEBUG(HD_MDI).Msg(" - Total Elements: %zu\n", _numTotalElements);
    TF_DEBUG(HD_MDI).Msg(" - Total Verts: %zu\n", _numTotalVertices);

    // make sure we filled all
    TF_VERIFY(cmdIt == _drawCommandBuffer.end());

    // cache the location of instanceCount and cullInstanceCount,
    // to be used during DrawItemInstanceChanged().
    _instanceCountOffset = traits.instanceCount_offset/sizeof(uint32_t);
    _cullInstanceCountOffset = traits.cullInstanceCount_offset/sizeof(uint32_t);

    // allocate draw dispatch buffer
    _dispatchBuffer =
        resourceRegistry->RegisterDispatchBuffer(_tokens->drawIndirect,
                                                 numDrawItemInstances,
                                                 traits.numUInt32);

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

        // add culling resource views
        if (_useInstanceCulling) {
            _AddInstanceCullResourceViews(_dispatchBufferCullInput, traits);
        } else {
            _AddNonInstanceCullResourceViews(_dispatchBufferCullInput, traits);
        }

        // copy data
        _dispatchBufferCullInput->CopyData(_drawCommandBuffer);
    }
}

HdSt_DrawBatch::ValidationResult
HdSt_IndirectDrawBatch::Validate(bool deepValidation)
{
    if (!TF_VERIFY(!_drawItemInstances.empty())) {
        return ValidationResult::RebuildAllBatches;
    }

    TF_DEBUG(HDST_DRAW_BATCH).Msg(
        "Validating indirect draw batch %p (deep validation = %d)...\n",
        (void*)(this), deepValidation);

    // check the hash to see they've been reallocated/migrated or not.
    // note that we just need to compare the hash of the first item,
    // since drawitems are aggregated and ensure that they are sharing
    // same buffer arrays.
    HdStDrawItem const* batchItem = _drawItemInstances.front()->GetDrawItem();
    size_t const bufferArraysHash = batchItem->GetBufferArraysHash();

    if (_bufferArraysHash != bufferArraysHash) {
        _bufferArraysHash = bufferArraysHash;
        TF_DEBUG(HDST_DRAW_BATCH).Msg(
            "   Buffer arrays hash changed. Need to rebuild batch.\n");
        return ValidationResult::RebuildBatch;
    }

    // Deep validation is flagged explicitly when a drawItem has changes to
    // its BARs (e.g. buffer spec, aggregation, element offsets) or when its 
    // surface shader or geometric shader changes.
    if (deepValidation) {
        HD_TRACE_SCOPE("Indirect draw batch deep validation");
        // look through all draw items to be still compatible

        size_t numDrawItemInstances = _drawItemInstances.size();
        size_t barElementOffsetsHash = 0;

        for (size_t item = 0; item < numDrawItemInstances; ++item) {
            HdStDrawItem const * drawItem
                = _drawItemInstances[item]->GetDrawItem();

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

void
HdSt_IndirectDrawBatch::_ValidateCompatibility(
            HdStBufferArrayRangeSharedPtr const& constantBar,
            HdStBufferArrayRangeSharedPtr const& indexBar,
            HdStBufferArrayRangeSharedPtr const& topologyVisibilityBar,
            HdStBufferArrayRangeSharedPtr const& elementBar,
            HdStBufferArrayRangeSharedPtr const& fvarBar,
            HdStBufferArrayRangeSharedPtr const& varyingBar,
            HdStBufferArrayRangeSharedPtr const& vertexBar,
            int instancerNumLevels,
            HdStBufferArrayRangeSharedPtr const& instanceIndexBar,
            std::vector<HdStBufferArrayRangeSharedPtr> const& instanceBars) const
{
    HdStDrawItem const* failed = nullptr;

    for (HdStDrawItemInstance const* itemInstance : _drawItemInstances) {
        HdStDrawItem const* itm = itemInstance->GetDrawItem();

        if (constantBar && !TF_VERIFY(constantBar 
                        ->IsAggregatedWith(itm->GetConstantPrimvarRange())))
                        { failed = itm; break; }
        if (indexBar && !TF_VERIFY(indexBar
                        ->IsAggregatedWith(itm->GetTopologyRange())))
                        { failed = itm; break; }
        if (topologyVisibilityBar && !TF_VERIFY(topologyVisibilityBar
                        ->IsAggregatedWith(itm->GetTopologyVisibilityRange())))
                        { failed = itm; break; }
        if (elementBar && !TF_VERIFY(elementBar
                        ->IsAggregatedWith(itm->GetElementPrimvarRange())))
                        { failed = itm; break; }
        if (fvarBar && !TF_VERIFY(fvarBar
                        ->IsAggregatedWith(itm->GetFaceVaryingPrimvarRange())))
                        { failed = itm; break; }
        if (varyingBar && !TF_VERIFY(varyingBar
                        ->IsAggregatedWith(itm->GetVaryingPrimvarRange())))
                        { failed = itm; break; }
        if (vertexBar && !TF_VERIFY(vertexBar
                        ->IsAggregatedWith(itm->GetVertexPrimvarRange())))
                        { failed = itm; break; }
        if (!TF_VERIFY(instancerNumLevels
                        == itm->GetInstancePrimvarNumLevels()))
                        { failed = itm; break; }
        if (instanceIndexBar && !TF_VERIFY(instanceIndexBar
                        ->IsAggregatedWith(itm->GetInstanceIndexRange())))
                        { failed = itm; break; }
        if (!TF_VERIFY(instancerNumLevels == (int)instanceBars.size()))
                        { failed = itm; break; }

        std::vector<HdStBufferArrayRangeSharedPtr> itmInstanceBars(
                                                            instancerNumLevels);
        if (instanceIndexBar) {
            for (int i = 0; i < instancerNumLevels; ++i) {
                if (itmInstanceBars[i] && !TF_VERIFY(itmInstanceBars[i] 
                            ->IsAggregatedWith(itm->GetInstancePrimvarRange(i)),
                        "%d", i)) { failed = itm; break; }
            }
        }
    }

    if (failed) {
        std::cout << failed->GetRprimID() << std::endl;
    }
}

bool
HdSt_IndirectDrawBatch::_HasNothingToDraw() const
{
    return ( _useDrawIndexed && _numTotalElements == 0) ||
           (!_useDrawIndexed && _numTotalVertices == 0);
}

void
HdSt_IndirectDrawBatch::PrepareDraw(
    HdStRenderPassStateSharedPtr const &renderPassState,
    HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    HD_TRACE_FUNCTION();

    GLF_GROUP_FUNCTION();

    renderPassState->Bind();

    //
    // compile
    //

    if (!_dispatchBuffer) {
        _CompileBatch(resourceRegistry);
    }

    if (_HasNothingToDraw()) return;

    HdStDrawItem const* batchItem = _drawItemInstances.front()->GetDrawItem();

    // Bypass freezeCulling if the command buffer is dirty.
    bool freezeCulling = TfDebug::IsEnabled(HD_FREEZE_CULL_FRUSTUM)
                                && !_drawCommandBufferDirty;

    bool gpuCulling = _useGpuCulling;

    if (gpuCulling && !_useInstanceCulling) {
        // disable GPU culling when instancing enabled and
        // not using instance culling.
        if (batchItem->GetInstanceIndexRange()) gpuCulling = false;
    }

    // Do we have to update our dispatch buffer because drawitem instance
    // data has changed?
    // On the first time through, after batches have just been compiled,
    // the flag will be false because the resource registry will have already
    // uploaded the buffer.
    if (_drawCommandBufferDirty) {
        _dispatchBuffer->CopyData(_drawCommandBuffer);

        if (gpuCulling) {
            _dispatchBufferCullInput->CopyData(_drawCommandBuffer);
        }
        _drawCommandBufferDirty = false;
    }

    //
    // cull
    //

    if (gpuCulling && !freezeCulling) {
        if (_useInstanceCulling) {
            _GPUFrustumInstanceCulling(
                batchItem, GfMatrix4f(renderPassState->GetCullMatrix()),
                renderPassState->GetDrawingRangeNDC(), resourceRegistry);
        } else {
            _GPUFrustumNonInstanceCulling(
                batchItem, GfMatrix4f(renderPassState->GetCullMatrix()),
                renderPassState->GetDrawingRangeNDC(), resourceRegistry);
        }

        if (IsEnabledGPUCountVisibleInstances()) {
            _EndGPUCountVisibleInstances(resourceRegistry, &_numVisibleItems);
        }
    }

    renderPassState->Unbind();
}

void
HdSt_IndirectDrawBatch::ExecuteDraw(
    HgiGraphicsCmds *gfxCmds,
    HdStRenderPassStateSharedPtr const &renderPassState,
    HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    HgiGLGraphicsCmds* glGfxCmds = dynamic_cast<HgiGLGraphicsCmds*>(gfxCmds);

    if (glGfxCmds) {
        // XXX Tmp code path to allow non-hgi code to insert functions into
        // HgiGL ops-stack. Will be removed once Storms uses Hgi everywhere
        auto executeDrawOp = [this, renderPassState, resourceRegistry] {
            this->_ExecuteDraw(renderPassState, resourceRegistry);
        };
        glGfxCmds->InsertFunctionOp(executeDrawOp);
    } else {
        _ExecuteDraw(renderPassState, resourceRegistry);
    }
}

void
HdSt_IndirectDrawBatch::_ExecuteDraw(
    HdStRenderPassStateSharedPtr const &renderPassState,
    HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    HD_TRACE_FUNCTION();

    if (!glBindBuffer) return; // glew initialized

    if (!TF_VERIFY(!_drawItemInstances.empty())) return;

    HdStDrawItem const* batchItem = _drawItemInstances.front()->GetDrawItem();

    if (!TF_VERIFY(batchItem)) return;

    if (!TF_VERIFY(_dispatchBuffer)) return;

    if (_HasNothingToDraw()) return;

    GLF_GROUP_FUNCTION();
    
    renderPassState->Bind();

    //
    // draw
    //

    // bind program
    _DrawingProgram & program = _GetDrawingProgram(renderPassState,
                                                   /*indirect=*/true,
                                                   resourceRegistry);
    HdStGLSLProgramSharedPtr const &glslProgram = program.GetGLSLProgram();
    if (!TF_VERIFY(glslProgram)) return;
    if (!TF_VERIFY(glslProgram->Validate())) return;

    GLuint programId = glslProgram->GetProgram()->GetRawResource();
    TF_VERIFY(programId);

    GlfDebugLabelProgram(programId, "DrawingProgram");
    glUseProgram(programId);

    const HdSt_ResourceBinder &binder = program.GetBinder();
    const HdStShaderCodeSharedPtrVector &shaders = program.GetComposedShaders();

    // XXX: for surfaces shader, we need to iterate all drawItems to
    //      make textures resident, instead of just the first batchItem
    for (HdStShaderCodeSharedPtr const & shader : shaders) {
        shader->BindResources(programId, binder);
    }

    // constant buffer bind
    HdBufferArrayRangeSharedPtr constantBar_ = batchItem->GetConstantPrimvarRange();
    HdStBufferArrayRangeSharedPtr constantBar =
        std::static_pointer_cast<HdStBufferArrayRange>(constantBar_);
    binder.BindConstantBuffer(constantBar);

    // index buffer bind
    HdBufferArrayRangeSharedPtr indexBar_ = batchItem->GetTopologyRange();
    HdStBufferArrayRangeSharedPtr indexBar =
        std::static_pointer_cast<HdStBufferArrayRange>(indexBar_);
    binder.BindBufferArray(indexBar);

    // topology visibility buffer bind
    HdBufferArrayRangeSharedPtr topVisBar_ =
        batchItem->GetTopologyVisibilityRange();
    HdStBufferArrayRangeSharedPtr topVisBar =
        std::static_pointer_cast<HdStBufferArrayRange>(topVisBar_);
    binder.BindInterleavedBuffer(topVisBar, HdTokens->topologyVisibility);

    // element buffer bind
    HdBufferArrayRangeSharedPtr elementBar_ = batchItem->GetElementPrimvarRange();
    HdStBufferArrayRangeSharedPtr elementBar =
        std::static_pointer_cast<HdStBufferArrayRange>(elementBar_);
    binder.BindBufferArray(elementBar);

    // fvar buffer bind
    HdBufferArrayRangeSharedPtr fvarBar_ = batchItem->GetFaceVaryingPrimvarRange();
    HdStBufferArrayRangeSharedPtr fvarBar =
        std::static_pointer_cast<HdStBufferArrayRange>(fvarBar_);
    binder.BindBufferArray(fvarBar);

    // varying buffer bind
    HdBufferArrayRangeSharedPtr varyingBar_ = batchItem->GetVaryingPrimvarRange();
    HdStBufferArrayRangeSharedPtr varyingBar =
        std::static_pointer_cast<HdStBufferArrayRange>(varyingBar_);
    binder.BindBufferArray(varyingBar);

    // vertex buffer bind
    HdBufferArrayRangeSharedPtr vertexBar_ = batchItem->GetVertexPrimvarRange();
    HdStBufferArrayRangeSharedPtr vertexBar =
         std::static_pointer_cast<HdStBufferArrayRange>(vertexBar_);
    binder.BindBufferArray(vertexBar);

    // instance buffer bind
    int instancerNumLevels = batchItem->GetInstancePrimvarNumLevels();
    std::vector<HdStBufferArrayRangeSharedPtr> instanceBars(instancerNumLevels);

    // instance index indirection
    HdBufferArrayRangeSharedPtr instanceIndexBar_ = batchItem->GetInstanceIndexRange();
    HdStBufferArrayRangeSharedPtr instanceIndexBar =
        std::static_pointer_cast<HdStBufferArrayRange>(instanceIndexBar_);
    if (instanceIndexBar) {
        // note that while instanceIndexBar is mandatory for instancing but
        // instanceBar can technically be empty (it doesn't make sense though)
        // testHdInstance --noprimvars covers that case.
        for (int i = 0; i < instancerNumLevels; ++i) {
            HdBufferArrayRangeSharedPtr ins_ = batchItem->GetInstancePrimvarRange(i);
            HdStBufferArrayRangeSharedPtr ins =
                std::static_pointer_cast<HdStBufferArrayRange>(ins_);
            instanceBars[i] = ins;
            binder.BindInstanceBufferArray(instanceBars[i], i);
        }
        binder.BindBufferArray(instanceIndexBar);
    }

    if (false && ARCH_UNLIKELY(TfDebug::IsEnabled(HD_SAFE_MODE))) {
        _ValidateCompatibility(constantBar,
                               indexBar,
                               topVisBar,
                               elementBar,
                               fvarBar,
                               varyingBar,
                               vertexBar,
                               instancerNumLevels,
                               instanceIndexBar,
                               instanceBars);
    }

    // shader buffer bind
    HdStBufferArrayRangeSharedPtr shaderBar;
    for (HdStShaderCodeSharedPtr const & shader : shaders) {
        HdBufferArrayRangeSharedPtr shaderBar_ = shader->GetShaderData();
        shaderBar = std::static_pointer_cast<HdStBufferArrayRange>(shaderBar_);
        if (shaderBar) {
            binder.BindBuffer(HdTokens->materialParams, 
                              shaderBar->GetResource());
        }
    }

    // drawindirect command, drawing coord, instanceIndexBase bind
    HdStBufferArrayRangeSharedPtr dispatchBar =
        _dispatchBuffer->GetBufferArrayRange();
    binder.BindBufferArray(dispatchBar);

    // update geometric shader states
    HdSt_GeometricShaderSharedPtr const & geometricShader =
                                                program.GetGeometricShader();
    geometricShader->BindResources(programId, binder);

    renderPassState->ApplyStateFromGeometricShader(binder, geometricShader);

    if (geometricShader->IsPrimTypePatches()) {
        glPatchParameteri(GL_PATCH_VERTICES,
                          geometricShader->GetPrimitiveIndexSize());
    }

    uint32_t batchCount = _dispatchBuffer->GetCount();

    if (!_useDrawIndexed) {
        TF_DEBUG(HD_MDI).Msg("MDI Drawing Arrays:\n"
                " - primitive mode: %d\n"
                " - indirect: %d\n"
                " - drawCount: %d\n"
                " - stride: %zu\n",
               HdStGLConversions::GetPrimitiveMode(geometricShader.get()),
               0, batchCount,
               _dispatchBuffer->GetCommandNumUints()*sizeof(uint32_t));

        glMultiDrawArraysIndirect(
            HdStGLConversions::GetPrimitiveMode(geometricShader.get()),
            0, // draw command always starts with 0
            batchCount,
            _dispatchBuffer->GetCommandNumUints()*sizeof(uint32_t));
    } else {
        TF_DEBUG(HD_MDI).Msg("MDI Drawing Elements:\n"
                " - primitive mode: %d\n"
                " - buffer type: GL_UNSIGNED_INT\n"
                " - indirect: %d\n"
                " - drawCount: %d\n"
                " - stride: %zu\n",
               HdStGLConversions::GetPrimitiveMode(geometricShader.get()),
               0, batchCount,
               _dispatchBuffer->GetCommandNumUints()*sizeof(uint32_t));

        glMultiDrawElementsIndirect(
            HdStGLConversions::GetPrimitiveMode(geometricShader.get()),
            GL_UNSIGNED_INT,
            0, // draw command always starts with 0
            batchCount,
            _dispatchBuffer->GetCommandNumUints()*sizeof(uint32_t));
    }

    HD_PERF_COUNTER_INCR(HdPerfTokens->drawCalls);
    HD_PERF_COUNTER_ADD(HdTokens->itemsDrawn, _numVisibleItems);

    //
    // cleanup
    //
    binder.UnbindConstantBuffer(constantBar);
    binder.UnbindInterleavedBuffer(topVisBar, HdTokens->topologyVisibility);
    binder.UnbindBufferArray(elementBar);
    binder.UnbindBufferArray(fvarBar);
    binder.UnbindBufferArray(indexBar);
    binder.UnbindBufferArray(vertexBar);
    binder.UnbindBufferArray(varyingBar);
    binder.UnbindBufferArray(dispatchBar);
    if(shaderBar) {
        binder.UnbindBuffer(HdTokens->materialParams, 
                            shaderBar->GetResource());
    }

    if (instanceIndexBar) {
        for (int i = 0; i < instancerNumLevels; ++i) {
            binder.UnbindInstanceBufferArray(instanceBars[i], i);
        }
        binder.UnbindBufferArray(instanceIndexBar);
    }

    for (HdStShaderCodeSharedPtr const & shader : shaders) {
        shader->UnbindResources(programId, binder);
    }
    geometricShader->UnbindResources(programId, binder);

    glUseProgram(0);

    renderPassState->Unbind();
}

static HgiGraphicsPipelineSharedPtr
_GetCullPipeline(
    HdStResourceRegistrySharedPtr const &resourceRegistry,
    HdStGLSLProgramSharedPtr const& shaderProgram,
    size_t byteSizeUniforms)
{
    // Culling pipeline is compatible as long as the shader is the same.
    HgiShaderProgramHandle const& prg = shaderProgram->GetProgram();
    uint64_t hash = reinterpret_cast<uint64_t>(prg.Get());

    HdInstance<HgiGraphicsPipelineSharedPtr> pipelineInstance =
        resourceRegistry->RegisterGraphicsPipeline(hash);

    if (pipelineInstance.IsFirstInstance()) {
        // Create a points primitive, vertex shader only pipeline that uses
        // a uniform block data for the 'cullParams' in the shader.
        HgiGraphicsPipelineDesc pipeDesc;
        pipeDesc.shaderConstantsDesc.stageUsage = HgiShaderStageVertex;
        pipeDesc.shaderConstantsDesc.byteSize = byteSizeUniforms;
        pipeDesc.depthState.depthTestEnabled = false;
        pipeDesc.depthState.depthWriteEnabled = false;
        pipeDesc.primitiveType = HgiPrimitiveTypePointList;
        pipeDesc.shaderProgram = shaderProgram->GetProgram();
        pipeDesc.rasterizationState.rasterizerEnabled = false;

        Hgi* hgi = resourceRegistry->GetHgi();
        HgiGraphicsPipelineHandle pso = hgi->CreateGraphicsPipeline(pipeDesc);

        pipelineInstance.SetValue(
            std::make_shared<HgiGraphicsPipelineHandle>(pso));
    }

    return pipelineInstance.GetValue();
}

void
HdSt_IndirectDrawBatch::_GPUFrustumInstanceCulling(
    HdStDrawItem const *batchItem,
    GfMatrix4f const &cullMatrix,
    GfVec2f const &drawRangeNdc,
    HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    HdBufferArrayRangeSharedPtr constantBar_ =
        batchItem->GetConstantPrimvarRange();
    HdStBufferArrayRangeSharedPtr constantBar =
        std::static_pointer_cast<HdStBufferArrayRange>(constantBar_);
    int instancerNumLevels = batchItem->GetInstancePrimvarNumLevels();
    std::vector<HdStBufferArrayRangeSharedPtr> instanceBars(instancerNumLevels);
    for (int i = 0; i < instancerNumLevels; ++i) {
        HdBufferArrayRangeSharedPtr ins_ = batchItem->GetInstancePrimvarRange(i);

        HdStBufferArrayRangeSharedPtr ins =
            std::static_pointer_cast<HdStBufferArrayRange>(ins_);

        instanceBars[i] = ins;
    }
    HdBufferArrayRangeSharedPtr instanceIndexBar_ =
        batchItem->GetInstanceIndexRange();
    HdStBufferArrayRangeSharedPtr instanceIndexBar =
        std::static_pointer_cast<HdStBufferArrayRange>(instanceIndexBar_);

    HdStBufferArrayRangeSharedPtr cullDispatchBar =
        _dispatchBufferCullInput->GetBufferArrayRange();

    _CullingProgram cullingProgram = _GetCullingProgram(resourceRegistry);

    HdStGLSLProgramSharedPtr const &
        glslProgram = cullingProgram.GetGLSLProgram();

    if (!TF_VERIFY(glslProgram)) return;
    if (!TF_VERIFY(glslProgram->Validate())) return;

    struct Uniforms {
        GfMatrix4f cullMatrix;
        GfVec2f drawRangeNDC;
        uint32_t drawCommandNumUints;
        int32_t resetPass;
    } cullParams;

    // We perform frustum culling on the GPU with the rasterizer disabled,
    // stomping the instanceCount of each drawing command in the
    // dispatch buffer to 0 for primitives that are culled, skipping
    // over other elements.

    const HdSt_ResourceBinder &binder = cullingProgram.GetBinder();

    // XXX Remove this once we switch the resource bindings below to Hgi.
    // Right now we need this since 'binder' uses raw gl calls.
    GLuint programId = glslProgram->GetProgram()->GetRawResource();
    glUseProgram(programId);

    // bind buffers
    binder.BindConstantBuffer(constantBar);

    // bind per-drawitem attribute (drawingCoord, instanceCount, drawCommand)
    binder.BindBufferArray(cullDispatchBar);

    if (instanceIndexBar) {
        int instancerNumLevels = batchItem->GetInstancePrimvarNumLevels();
        for (int i = 0; i < instancerNumLevels; ++i) {
            binder.BindInstanceBufferArray(instanceBars[i], i);
        }
        binder.BindBufferArray(instanceIndexBar);
    }

    if (IsEnabledGPUCountVisibleInstances()) {
        _BeginGPUCountVisibleInstances(resourceRegistry);
        binder.BindBuffer(_tokens->drawIndirectResult, _resultBuffer);
    }

    // bind destination buffer (using entire buffer bind to start from offset=0)
    binder.BindBuffer(_tokens->dispatchBuffer,
                      _dispatchBuffer->GetEntireResource());

    // set cull parameters
    cullParams.drawCommandNumUints = _dispatchBuffer->GetCommandNumUints();
    cullParams.cullMatrix = cullMatrix;
    cullParams.drawRangeNDC = drawRangeNdc;
    cullParams.resetPass = 1;

    // run culling shader
    bool validProgram = true;

    // XXX: should we cache cull command offset?
    HdStBufferResourceSharedPtr cullCommandBuffer =
        _dispatchBufferCullInput->GetResource(HdTokens->drawDispatch);
    if (!TF_VERIFY(cullCommandBuffer)) {
        validProgram = false;
    }

    if (validProgram) {
        Hgi* hgi = resourceRegistry->GetHgi();

        HgiGraphicsPipelineSharedPtr const& pso =
            _GetCullPipeline(resourceRegistry, glslProgram, sizeof(Uniforms));
        HgiGraphicsPipelineHandle psoHandle = *pso.get();

        // Get the bind index for the 'cullParams' uniform block
        HdBinding binding = binder.GetBinding(_tokens->ulocCullParams);
        int bindLoc = binding.GetLocation();

        // GfxCmds has no attachment since it is a vertex only shader.
        HgiGraphicsCmdsDesc gfxDesc;
        HgiGraphicsCmdsUniquePtr cullGfxCmds = hgi->CreateGraphicsCmds(gfxDesc);
        cullGfxCmds->PushDebugGroup("GPU frustum instance culling");
        cullGfxCmds->BindPipeline(psoHandle);

        // Reset Pass
        cullGfxCmds->SetConstantValues(
            psoHandle, HgiShaderStageVertex, 
            bindLoc, sizeof(Uniforms), &cullParams);

        cullGfxCmds->DrawIndirect(
            cullCommandBuffer->GetHandle(),
            cullCommandBuffer->GetOffset(),
            _dispatchBufferCullInput->GetCount(),
            cullCommandBuffer->GetStride());

        // Make sure the reset-pass memory writes
        // are visible to the culling shader pass.
        cullGfxCmds->MemoryBarrier(HgiMemoryBarrierAll);

        // Perform Culling
        cullParams.resetPass = 0;
        cullGfxCmds->SetConstantValues(
            psoHandle, HgiShaderStageVertex,
            bindLoc, sizeof(Uniforms), &cullParams);

        cullGfxCmds->DrawIndirect(
            cullCommandBuffer->GetHandle(),
            cullCommandBuffer->GetOffset(),
            _dispatchBufferCullInput->GetCount(),
            cullCommandBuffer->GetStride());

        // Make sure culling memory writes are
        // visible to execute draw.
        cullGfxCmds->MemoryBarrier(HgiMemoryBarrierAll);

        cullGfxCmds->PopDebugGroup();
        hgi->SubmitCmds(cullGfxCmds.get());
    }

    // XXX Remove the unbinding since it uses raw gl calls.
    // We can unbind the dispatchBuffer inside Hgi::DrawIndirect or
    // we can add this unbinding logic inside HgiGl's scoped state holder.

    // Reset all vertex attribs and their divisors. Note that the drawing
    // program has different bindings from the culling program does
    // in general, even though most of buffers will likely be assigned
    // with same attrib divisors again.
    binder.UnbindConstantBuffer(constantBar);
    binder.UnbindBufferArray(cullDispatchBar);
    if (instanceIndexBar) {
        int instancerNumLevels = batchItem->GetInstancePrimvarNumLevels();
        for (int i = 0; i < instancerNumLevels; ++i) {
            binder.UnbindInstanceBufferArray(instanceBars[i], i);
        }
        binder.UnbindBufferArray(instanceIndexBar);
    }

    // unbind destination dispatch buffer
    binder.UnbindBuffer(_tokens->dispatchBuffer,
                        _dispatchBuffer->GetEntireResource());

    if (IsEnabledGPUCountVisibleInstances()) {
        binder.UnbindBuffer(_tokens->drawIndirectResult, _resultBuffer);
    }
}

void
HdSt_IndirectDrawBatch::_GPUFrustumNonInstanceCulling(
    HdStDrawItem const *batchItem,
    GfMatrix4f const &cullMatrix,
    GfVec2f const &drawRangeNdc,
    HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    HdBufferArrayRangeSharedPtr constantBar_ =
        batchItem->GetConstantPrimvarRange();
    HdStBufferArrayRangeSharedPtr constantBar =
        std::static_pointer_cast<HdStBufferArrayRange>(constantBar_);

    HdStBufferArrayRangeSharedPtr cullDispatchBar =
        _dispatchBufferCullInput->GetBufferArrayRange();

    _CullingProgram &cullingProgram = _GetCullingProgram(resourceRegistry);

    HdStGLSLProgramSharedPtr const &
        glslProgram = cullingProgram.GetGLSLProgram();
    if (!TF_VERIFY(glslProgram)) return;
    if (!TF_VERIFY(glslProgram->Validate())) return;

    // We perform frustum culling on the GPU with the rasterizer disabled,
    // stomping the instanceCount of each drawing command in the
    // dispatch buffer to 0 for primitives that are culled, skipping
    // over other elements.

    struct Uniforms {
        GfMatrix4f cullMatrix;
        GfVec2f drawRangeNDC;
        uint32_t drawCommandNumUints;
    } cullParams;

    // XXX Remove this once we switch the resource bindings below to Hgi.
    // Right now we need this since 'binder' uses raw gl calls.
    GLuint programId = glslProgram->GetProgram()->GetRawResource();
    glUseProgram(programId);

    const HdSt_ResourceBinder &binder = cullingProgram.GetBinder();

    // bind constant
    binder.BindConstantBuffer(constantBar);
    // bind drawing coord, instance count
    binder.BindBufferArray(cullDispatchBar);

    if (IsEnabledGPUCountVisibleInstances()) {
        _BeginGPUCountVisibleInstances(resourceRegistry);
        binder.BindBuffer(_tokens->drawIndirectResult, _resultBuffer);
    }

    // set cull parameters
    cullParams.drawCommandNumUints = _dispatchBuffer->GetCommandNumUints();
    cullParams.cullMatrix = cullMatrix;
    cullParams.drawRangeNDC = drawRangeNdc;

    // bind destination buffer (using entire buffer bind to start from offset=0)
    binder.BindBuffer(_tokens->dispatchBuffer,
                      _dispatchBuffer->GetEntireResource());

    Hgi* hgi = resourceRegistry->GetHgi();

    HgiGraphicsPipelineSharedPtr const& pso =
        _GetCullPipeline(resourceRegistry, glslProgram, sizeof(Uniforms));
    HgiGraphicsPipelineHandle psoHandle = *pso.get();

    // Get the bind index for the 'resetPass' uniform
    HdBinding binding = binder.GetBinding(_tokens->ulocCullParams);
    int bindLoc = binding.GetLocation();

    //
    // Perform Culling
    //

    // GfxCmds has no attachment since it is a vertex only shader.
    HgiGraphicsCmdsDesc gfxDesc;
    HgiGraphicsCmdsUniquePtr cullGfxCmds= hgi->CreateGraphicsCmds(gfxDesc);
    cullGfxCmds->PushDebugGroup("GPU frustum culling (Non-instanced)");
    cullGfxCmds->BindPipeline(psoHandle);
    cullGfxCmds->SetConstantValues(
        psoHandle, HgiShaderStageVertex,
        bindLoc, sizeof(Uniforms), &cullParams);

    cullGfxCmds->Draw(_dispatchBufferCullInput->GetCount(), 0, 1, 0);

    // Make sure culling memory writes are visible to execute draw.
    cullGfxCmds->MemoryBarrier(HgiMemoryBarrierAll);

    cullGfxCmds->PopDebugGroup();
    hgi->SubmitCmds(cullGfxCmds.get());

    // XXX Remove the unbinding since it uses raw gl calls.
    // We can unbind the dispatchBuffer inside Hgi::DrawIndirect or
    // we can add this unbinding logic inside HgiGl's scoped state holder.

    // unbind destination dispatch buffer
    binder.UnbindBuffer(_tokens->dispatchBuffer,
                        _dispatchBuffer->GetEntireResource());

    // unbind all
    binder.UnbindConstantBuffer(constantBar);
    binder.UnbindBufferArray(cullDispatchBar);

    if (IsEnabledGPUCountVisibleInstances()) {
        binder.UnbindBuffer(_tokens->drawIndirectResult, _resultBuffer);
    }
}

void
HdSt_IndirectDrawBatch::DrawItemInstanceChanged(HdStDrawItemInstance const* instance)
{
    // We need to check the visibility and update if needed
    if (_dispatchBuffer) {
        size_t batchIndex = instance->GetBatchIndex();
        int commandNumUints = _dispatchBuffer->GetCommandNumUints();
        int numLevels = instance->GetDrawItem()->GetInstancePrimvarNumLevels();
        int instanceIndexWidth = numLevels + 1;

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

        TF_DEBUG(HD_MDI).Msg("\nInstance Count changed: %d -> %d\n",
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
}

void
HdSt_IndirectDrawBatch::_BeginGPUCountVisibleInstances(
    HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    if (!_resultBuffer) {
        HdTupleType tupleType;
        tupleType.type = HdTypeInt32;
        tupleType.count = 1;

        _resultBuffer = 
            resourceRegistry->RegisterBufferResource(
                _tokens->drawIndirectResult, tupleType);
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

    // For now we need to submit here, because there are raw gl calls after
    // _BeginGPUCountVisibleInstances that rely on this having executed on GPU.
    // XXX Remove this once the rest of indirectDrawBatch is using Hgi.
    resourceRegistry->SubmitBlitWork();
}

void
HdSt_IndirectDrawBatch::_EndGPUCountVisibleInstances(
    HdStResourceRegistrySharedPtr const &resourceRegistry,
    size_t * result)
{
    // Submit and wait for all the work recorded up to this point.
    // The GPU work must complete before we can read-back the GPU buffer.
    // GPU frustum culling is (currently) a vertex shader without a fragment
    // shader, so we submit the blit work, but do not have any compute work.
    resourceRegistry->SubmitBlitWork(HgiSubmitWaitTypeWaitUntilCompleted);

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
HdSt_IndirectDrawBatch::_CullingProgram::Initialize(
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
HdSt_IndirectDrawBatch::_CullingProgram::_GetCustomBindings(
    HdBindingRequestVector *customBindings,
    bool *enableInstanceDraw) const
{
    if (!TF_VERIFY(enableInstanceDraw) ||
        !TF_VERIFY(customBindings)) return;

    customBindings->push_back(HdBindingRequest(HdBinding::SSBO,
                                  _tokens->drawIndirectResult));
    customBindings->push_back(HdBindingRequest(HdBinding::SSBO,
                                  _tokens->dispatchBuffer));
    customBindings->push_back(HdBindingRequest(HdBinding::UBO,
                                               _tokens->ulocCullParams));

    if (_useInstanceCulling) {
        customBindings->push_back(
            HdBindingRequest(HdBinding::DRAW_INDEX_INSTANCE,
                _tokens->drawCommandIndex));
    } else {
        // non-instance culling
        customBindings->push_back(
            HdBindingRequest(HdBinding::DRAW_INDEX,
                _tokens->drawCommandIndex));
        customBindings->push_back(
            HdBindingRequest(HdBinding::DRAW_INDEX,
                _tokens->instanceCountInput));
    }

    // set instanceDraw true if instanceCulling is enabled.
    // this value will be used to determine if glVertexAttribDivisor needs to
    // be enabled or not.
    *enableInstanceDraw = _useInstanceCulling;
}

PXR_NAMESPACE_CLOSE_SCOPE

