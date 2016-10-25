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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/commandBuffer.h"
#include "pxr/imaging/hd/cullingShaderKey.h"
#include "pxr/imaging/hd/debugCodes.h"
#include "pxr/imaging/hd/geometricShader.h"
#include "pxr/imaging/hd/glslProgram.h"
#include "pxr/imaging/hd/indirectDrawBatch.h"
#include "pxr/imaging/hd/drawItemInstance.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/shader.h"
#include "pxr/imaging/hd/shaderKey.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/glslfx.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/iterator.h"

#include <limits>

static const GLuint64 HD_CULL_RESULT_TIMEOUT_NS = 5e9; // XXX how long to wait?

TF_DEFINE_ENV_SETTING(HD_ENABLE_GPU_TINY_PRIM_CULLING, true,
                      "Enable tiny prim culling");
TF_DEFINE_ENV_SETTING(HD_ENABLE_GPU_FRUSTUM_CULLING, true,
                      "Enable GPU frustum culling");
TF_DEFINE_ENV_SETTING(HD_ENABLE_GPU_COUNT_VISIBLE_INSTANCES, false,
                      "Enable GPU frustum culling visible count query");
TF_DEFINE_ENV_SETTING(HD_ENABLE_GPU_INSTANCE_FRUSTUM_CULLING, true,
                      "Enable GPU per-instance frustum culling");

Hd_IndirectDrawBatch::Hd_IndirectDrawBatch(
    HdDrawItemInstance * drawItemInstance)
    : Hd_DrawBatch(drawItemInstance)
    , _drawCommandBufferDirty(false)
    , _numVisibleItems(0)
    , _numTotalElements(0)
    , _instanceCountOffset(0)
    , _cullInstanceCountOffset(0)
    , _cullResultSync(0)
{
    _Init(drawItemInstance);
}

/*virtual*/
void
Hd_IndirectDrawBatch::_Init(HdDrawItemInstance * drawItemInstance)
{
    Hd_DrawBatch::_Init(drawItemInstance);
    drawItemInstance->SetBatchIndex(0);
    drawItemInstance->SetBatch(this);

    // remember buffer arrays version for dispatch buffer updating
    HdDrawItem const* drawItem = drawItemInstance->GetDrawItem();
    _bufferArraysHash = drawItem->GetBufferArraysHash();

    // determine gpu culling program by the first drawitem
    _useDrawArrays  = not drawItem->GetTopologyRange();
    _useInstancing = static_cast<bool>(drawItem->GetInstanceIndexRange());
    _useGpuCulling = IsEnabledGPUFrustumCulling();

    // note: _useInstancing condition is not necessary. it can be removed
    //       if we decide always to use instance culling instead of XFB.
    _useGpuInstanceCulling = _useInstancing and
        _useGpuCulling and IsEnabledGPUInstanceFrustumCulling();

    if (_useGpuCulling) {
        _cullingProgram.Initialize(
            _useDrawArrays, _useGpuInstanceCulling, _bufferArraysHash);
    }
}

Hd_IndirectDrawBatch::_CullingProgram &
Hd_IndirectDrawBatch::_GetCullingProgram()
{
    if (not _cullingProgram.GetGLSLProgram()) {
        // create a culling shader key
        Hd_CullingShaderKey shaderKey(_useGpuInstanceCulling,
                                      IsEnabledGPUTinyPrimCulling(),
                                      IsEnabledGPUCountVisibleInstances());

        // sharing the culling geometric shader for the same configuration.
        Hd_GeometricShaderSharedPtr cullShader =
            Hd_GeometricShader::Create(shaderKey);
        _cullingProgram.SetGeometricShader(cullShader);

        _cullingProgram.CompileShader(_drawItemInstances.front()->GetDrawItem(),
                                      /*indirect=*/true);
    }
    return _cullingProgram;
}

Hd_IndirectDrawBatch::~Hd_IndirectDrawBatch()
{
}

/* static */
bool
Hd_IndirectDrawBatch::IsEnabledGPUFrustumCulling()
{
    HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();
    // GPU XFB frustum culling should work since GL 4.0, but for now
    // the shader frustumCull.glslfx requires explicit uniform location
    static bool isEnabledGPUFrustumCulling =
        TfGetEnvSetting(HD_ENABLE_GPU_FRUSTUM_CULLING) and
        (caps.explicitUniformLocation);
    return isEnabledGPUFrustumCulling and
       not TfDebug::IsEnabled(HD_DISABLE_FRUSTUM_CULLING);
}

/* static */
bool
Hd_IndirectDrawBatch::IsEnabledGPUCountVisibleInstances()
{
    static bool isEnabledGPUCountVisibleInstances =
        TfGetEnvSetting(HD_ENABLE_GPU_COUNT_VISIBLE_INSTANCES);
    return isEnabledGPUCountVisibleInstances;
}

/* static */
bool
Hd_IndirectDrawBatch::IsEnabledGPUTinyPrimCulling()
{
    static bool isEnabledGPUTinyPrimCulling =
        TfGetEnvSetting(HD_ENABLE_GPU_TINY_PRIM_CULLING);
    return isEnabledGPUTinyPrimCulling;
}

/* static */
bool
Hd_IndirectDrawBatch::IsEnabledGPUInstanceFrustumCulling()
{
    HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();

    // GPU instance frustum culling requires SSBO of bindless buffer

    static bool isEnabledGPUInstanceFrustumCulling =
        TfGetEnvSetting(HD_ENABLE_GPU_INSTANCE_FRUSTUM_CULLING) and
        (caps.shaderStorageBufferEnabled or
         caps.bindlessBufferEnabled);
    return isEnabledGPUInstanceFrustumCulling;
}

void
Hd_IndirectDrawBatch::_CompileBatch()
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    int drawCount = (int)_drawItemInstances.size();
    if (_drawItemInstances.empty()) return;

    // note that when chaging struct definition of XFB culling,
    // Hd_IndirectDrawBatch::_CullingProgram::_CustomLink should also be
    // changed accordingly.

    // drawcommand is configured as one of followings:
    //
    // DrawArrays + XFB culling  : 12 integers (+ numInstanceLevels)
    struct _DrawArraysCommand {
        GLuint count;
        GLuint instanceCount;
        GLuint first;
        GLuint baseInstance;

        // XXX: This is just padding to avoid configuration changes during
        // transform feedback, which are not accounted for during shader
        // caching. We should find a better solution.
        GLuint __reserved_0;

        GLuint modelDC;
        GLuint constantDC;
        GLuint elementDC;
        GLuint primitiveDC;
        GLuint fvarDC;
        GLuint instanceIndexDC;
        GLuint shaderDC;
    };

    // DrawArrays + Instance culling : 15 integers (+ numInstanceLevels)
    struct _DrawArraysInstanceCullCommand {
        GLuint count;
        GLuint instanceCount;
        GLuint first;
        GLuint baseInstance;
        GLuint cullCount;
        GLuint cullInstanceCount;
        GLuint cullFirstVertex;
        GLuint cullBaseInstance;
        GLuint modelDC;
        GLuint constantDC;
        GLuint elementDC;
        GLuint primitiveDC;
        GLuint fvarDC;
        GLuint instanceIndexDC;
        GLuint shaderDC;
    };

    // DrawElements + XFB culling : 12 integers (+ numInstanceLevels)
    struct _DrawElementsCommand {
        GLuint count;
        GLuint instanceCount;
        GLuint first;
        GLuint baseVertex;
        GLuint baseInstance;
        GLuint modelDC;
        GLuint constantDC;
        GLuint elementDC;
        GLuint primitiveDC;
        GLuint fvarDC;
        GLuint instanceIndexDC;
        GLuint shaderDC;
    };

    // DrawElements + Instance culling : 16 integers (+ numInstanceLevels)
    struct _DrawElementsInstanceCullCommand {
        GLuint count;
        GLuint instanceCount;
        GLuint first;
        GLuint baseVertex;
        GLuint baseInstance;
        GLuint cullCount;
        GLuint cullInstanceCount;
        GLuint cullFirstVertex;
        GLuint cullBaseInstance;
        GLuint modelDC;
        GLuint constantDC;
        GLuint elementDC;
        GLuint primitiveDC;
        GLuint fvarDC;
        GLuint instanceIndexDC;
        GLuint shaderDC;
    };

    // Count the number of visible items. We may actually draw fewer
    // items than this when GPU frustum culling is active
    _numVisibleItems = 0;

    // elements to be drawn (early out for empty batch)
    _numTotalElements = 0;
    _numTotalVertices = 0;

    int instancerNumLevels
        = _drawItemInstances[0]->GetDrawItem()->GetInstancePrimVarNumLevels();

    // how many integers in the dispatch struct
    int commandNumUints = _useDrawArrays
        ? (_useGpuInstanceCulling
           ? sizeof(_DrawArraysInstanceCullCommand)/sizeof(GLuint)
           : sizeof(_DrawArraysCommand)/sizeof(GLuint))
        : (_useGpuInstanceCulling
           ? sizeof(_DrawElementsInstanceCullCommand)/sizeof(GLuint)
           : sizeof(_DrawElementsCommand)/sizeof(GLuint));
    // followed by instanceDC[numlevels]
    commandNumUints += instancerNumLevels;

    TF_DEBUG(HD_MDI).Msg("\nCompile MDI Batch\n");
    TF_DEBUG(HD_MDI).Msg(" - num uints: %d\n", commandNumUints);
    TF_DEBUG(HD_MDI).Msg(" - useDrawArrays: %d\n", _useDrawArrays);
    TF_DEBUG(HD_MDI).Msg(" - useGpuInstanceCulling: %d\n",
                                                    _useGpuInstanceCulling);

    size_t numDrawItemInstances = _drawItemInstances.size();
    TF_DEBUG(HD_MDI).Msg(" - num draw items: %zu\n", numDrawItemInstances);

    // Note: GL specifies baseVertex as 'int' and other as 'uint' in
    // drawcommand struct, but we never set negative baseVertex in our
    // usecases for bufferArray so we use uint for all fields here.
    _drawCommandBuffer.resize(numDrawItemInstances * commandNumUints);
    std::vector<GLuint>::iterator cmdIt = _drawCommandBuffer.begin();

    TF_DEBUG(HD_MDI).Msg(" - Processing Items:\n");
    for (size_t item = 0; item < numDrawItemInstances; ++item) {
        HdDrawItemInstance const * instance = _drawItemInstances[item];
        HdDrawItem const * drawItem = _drawItemInstances[item]->GetDrawItem();

        //
        // index buffer data
        //
        HdBufferArrayRangeSharedPtr const &
            indexBar = drawItem->GetTopologyRange();

        //
        // element (per-face) buffer data
        //
        HdBufferArrayRangeSharedPtr const &
            elementBar = drawItem->GetElementPrimVarRange();

        //
        // vertex attrib buffer data
        //
        HdBufferArrayRangeSharedPtr const &
            vertexBar = drawItem->GetVertexPrimVarRange();

        //
        // constant buffer data
        //
        HdBufferArrayRangeSharedPtr const &
            constantBar = drawItem->GetConstantPrimVarRange();

        //
        // face varying buffer data
        //
        HdBufferArrayRangeSharedPtr const &
            fvarBar = drawItem->GetFaceVaryingPrimVarRange();

        //
        // instance buffer data
        //
        int instanceIndexWidth = instancerNumLevels + 1;
        std::vector<HdBufferArrayRangeSharedPtr> instanceBars(instancerNumLevels);
        for (int i = 0; i < instancerNumLevels; ++i) {
            instanceBars[i] = drawItem->GetInstancePrimVarRange(i);
        }

        //
        // instance indices
        //
        HdBufferArrayRangeSharedPtr const &
            instanceIndexBar = drawItem->GetInstanceIndexRange();

        //
        // shader parameter
        //
        HdBufferArrayRangeSharedPtr const &
            shaderBar = drawItem->GetSurfaceShader()->GetShaderData();

        // 3 for triangles, 4 for quads, n for patches
        GLuint numIndicesPerPrimitive
            = drawItem->GetGeometricShader()->GetPrimitiveIndexSize();

        //
        // Get parameters from our buffer range objects to
        // allow drawing to access the correct elements from
        // aggregated buffers.
        //
        GLuint numElements = indexBar ? indexBar->GetNumElements() : 0;
        GLuint vertexOffset = 0;
        GLuint vertexCount = 0;
        if (vertexBar) {
            vertexOffset = vertexBar->GetOffset();
            vertexCount = vertexBar->GetNumElements();
        }
        // if delegate fails to get vertex primvars, it could be empty.
        // skip the drawitem to prevent drawing uninitialized vertices.
        if (vertexCount == 0) numElements = 0;
        GLuint baseInstance      = (GLuint)item;

        // drawing coordinates.
        GLuint modelDC         = 0; // reserved for future extension
        GLuint constantDC      = constantBar ? constantBar->GetIndex() : 0;
        GLuint elementDC       = elementBar ? elementBar->GetOffset() : 0;
        GLuint primitiveDC     = indexBar ? indexBar->GetOffset() : 0;
        GLuint fvarDC          = fvarBar ? fvarBar->GetOffset() : 0;
        GLuint instanceIndexDC = instanceIndexBar ? instanceIndexBar->GetOffset() : 0;
        GLuint shaderDC        = shaderBar ? shaderBar->GetIndex() : 0;

        GLuint indicesCount  = numElements * numIndicesPerPrimitive;
        // It's possible to have instanceIndexBar which is empty, and no instancePrimvars.
        // in that case instanceCount should be 0, instead of 1, otherwise
        // frustum culling shader writes the result out to out-of-bound buffer.
        // this is covered by testHdDrawBatching/EmptyDrawBatchTest
        GLuint instanceCount = instanceIndexBar
            ? instanceIndexBar->GetNumElements()/instanceIndexWidth
            : 1;
        if (not instance->IsVisible()) instanceCount = 0;
        GLuint firstIndex = indexBar ? indexBar->GetOffset() * numIndicesPerPrimitive : 0;

        if (_useDrawArrays) {
            if (_useGpuInstanceCulling) {
                *cmdIt++ = vertexCount;
                *cmdIt++ = instanceCount;
                *cmdIt++ = vertexOffset;
                *cmdIt++ = baseInstance;
                *cmdIt++ = 1;             /* cullCount (always 1) */
                *cmdIt++ = instanceCount; /* cullInstanceCount */
                *cmdIt++ = 0;             /* cullFirstVertex (not used)*/
                *cmdIt++ = baseInstance;  /* cullBaseInstance */
                *cmdIt++ = modelDC;
                *cmdIt++ = constantDC;
                *cmdIt++ = elementDC;
                *cmdIt++ = primitiveDC;
                *cmdIt++ = fvarDC;
                *cmdIt++ = instanceIndexDC;
                *cmdIt++ = shaderDC;
            } else {
                *cmdIt++ = vertexCount;
                *cmdIt++ = instanceCount;
                *cmdIt++ = vertexOffset;
                *cmdIt++ = baseInstance;
                cmdIt++; // __reserved_0
                *cmdIt++ = modelDC;
                *cmdIt++ = constantDC;
                *cmdIt++ = elementDC;
                *cmdIt++ = primitiveDC;
                *cmdIt++ = fvarDC;
                *cmdIt++ = instanceIndexDC;
                *cmdIt++ = shaderDC;
            }
        } else {
            if (_useGpuInstanceCulling) {
                *cmdIt++ = indicesCount;
                *cmdIt++ = instanceCount;
                *cmdIt++ = firstIndex;
                *cmdIt++ = vertexOffset;
                *cmdIt++ = baseInstance;
                *cmdIt++ = 1;             /* cullCount (always 1) */
                *cmdIt++ = instanceCount; /* cullInstanceCount */
                *cmdIt++ = 0;             /* cullFirstVertex (not used)*/
                *cmdIt++ = baseInstance;  /* cullBaseInstance */
                *cmdIt++ = modelDC;
                *cmdIt++ = constantDC;
                *cmdIt++ = elementDC;
                *cmdIt++ = primitiveDC;
                *cmdIt++ = fvarDC;
                *cmdIt++ = instanceIndexDC;
                *cmdIt++ = shaderDC;
            } else {
                *cmdIt++ = indicesCount;
                *cmdIt++ = instanceCount;
                *cmdIt++ = firstIndex;
                *cmdIt++ = vertexOffset;
                *cmdIt++ = baseInstance;
                *cmdIt++ = modelDC;
                *cmdIt++ = constantDC;
                *cmdIt++ = elementDC;
                *cmdIt++ = primitiveDC;
                *cmdIt++ = fvarDC;
                *cmdIt++ = instanceIndexDC;
                *cmdIt++ = shaderDC;
            }
        }
        for (int i = 0; i < instancerNumLevels; ++i) {
            GLuint instanceDC = instanceBars[i] ? instanceBars[i]->GetOffset() : 0;
            *cmdIt++ = instanceDC;
        }

        if (TfDebug::IsEnabled(HD_MDI)) {
            std::vector<GLuint>::iterator cmdIt2 = cmdIt - commandNumUints;
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

    HdResourceRegistry *resourceRegistry = &HdResourceRegistry::GetInstance();

    // allocate draw dispatch buffer
    _dispatchBuffer =
        resourceRegistry->RegisterDispatchBuffer(HdTokens->drawIndirect,
                                                 drawCount,
                                                 commandNumUints);
    // define binding views
    if (_useDrawArrays) {
        if (_useGpuInstanceCulling) {
            // draw indirect command
            _dispatchBuffer->AddBufferResourceView(
                HdTokens->drawDispatch, GL_INT, 1,
                offsetof(_DrawArraysInstanceCullCommand, count));
            // drawing coords 0
            _dispatchBuffer->AddBufferResourceView(
                HdTokens->drawingCoord0, GL_INT, 4,
                offsetof(_DrawArraysInstanceCullCommand, modelDC));
            // drawing coords 1
            _dispatchBuffer->AddBufferResourceView(
                HdTokens->drawingCoord1, GL_INT, 3,
                offsetof(_DrawArraysInstanceCullCommand, fvarDC));
            // instance drawing coords
            if (instancerNumLevels > 0) {
                _dispatchBuffer->AddBufferResourceView(
                    HdTokens->drawingCoordI, GL_INT, instancerNumLevels,
                    sizeof(_DrawArraysInstanceCullCommand));
            }
        } else {
            // draw indirect command
            _dispatchBuffer->AddBufferResourceView(
                HdTokens->drawDispatch, GL_INT, 1,
                offsetof(_DrawArraysCommand, count));
            // drawing coords 0
            _dispatchBuffer->AddBufferResourceView(
                HdTokens->drawingCoord0, GL_INT, 4,
                offsetof(_DrawArraysCommand, modelDC));
            // drawing coords 1
            _dispatchBuffer->AddBufferResourceView(
                HdTokens->drawingCoord1, GL_INT, 3,
                offsetof(_DrawArraysCommand, fvarDC));
            // instance drawing coords
            if (instancerNumLevels > 0) {
                _dispatchBuffer->AddBufferResourceView(
                    HdTokens->drawingCoordI, GL_INT, instancerNumLevels,
                    sizeof(_DrawArraysCommand));
            }
        }
    } else {
        if (_useGpuInstanceCulling) {
            // draw indirect command
            _dispatchBuffer->AddBufferResourceView(
                HdTokens->drawDispatch, GL_INT, 1,
                offsetof(_DrawElementsInstanceCullCommand, count));
            // drawing coords 0
            _dispatchBuffer->AddBufferResourceView(
                HdTokens->drawingCoord0, GL_INT, 4,
                offsetof(_DrawElementsInstanceCullCommand, modelDC));
            // drawing coords 1
            _dispatchBuffer->AddBufferResourceView(
                HdTokens->drawingCoord1, GL_INT, 3,
                offsetof(_DrawElementsInstanceCullCommand, fvarDC));
            // instance drawing coords
            if (instancerNumLevels > 0) {
                _dispatchBuffer->AddBufferResourceView(
                    HdTokens->drawingCoordI, GL_INT, instancerNumLevels,
                    sizeof(_DrawElementsInstanceCullCommand));
            }
        } else {
            // draw indirect command
            _dispatchBuffer->AddBufferResourceView(
                HdTokens->drawDispatch, GL_INT, 1,
                offsetof(_DrawElementsCommand, count));
            // drawing coords 0
            _dispatchBuffer->AddBufferResourceView(
                HdTokens->drawingCoord0, GL_INT, 4,
                offsetof(_DrawElementsCommand, modelDC));
            // drawing coords 1
            _dispatchBuffer->AddBufferResourceView(
                HdTokens->drawingCoord1, GL_INT, 3,
                    offsetof(_DrawElementsCommand, fvarDC));
            // instance drawing coords
            if (instancerNumLevels > 0) {
                _dispatchBuffer->AddBufferResourceView(
                    HdTokens->drawingCoordI, GL_INT, instancerNumLevels,
                    sizeof(_DrawElementsCommand));
            }
        }
    }

    // copy data
    _dispatchBuffer->CopyData(_drawCommandBuffer);

    if (_useGpuCulling) {
        // Make a duplicate of the draw dispatch buffer to use as an input
        // for GPU frustum culling (a single buffer cannot be bound for
        // both reading and xform feedback). We use only the instanceCount
        // and drawingCoord parameters, but it is simplest to just make
        // a copy.
        _dispatchBufferCullInput =
            resourceRegistry->RegisterDispatchBuffer(
                HdTokens->drawIndirectCull,
                drawCount,
                commandNumUints);

        // define binding views
        //
        // READ THIS CAREFULLY whenever you try to add/remove/shuffle
        // the drawing coordinate struct.
        //
        // We use (GL_INT, 2) as a type of drawingCoord1 for GPU culling.
        // Because drawingCoord1 is defined as 3 integers struct,
        //
        //   GLuint fvarDC;
        //   GLuint instanceIndexDC;
        //   GLuint shaderDC;
        //
        // And CodeGen generates GetInstanceIndexCoord() as
        //
        //  int GetInstanceIndexCoord() { return GetDrawingCoord1().y; }
        //
        // so the instanceIndex coord must be the second element.
        //
        // We prefer smaller number of attributes to be processed in
        // the vertex input assembler, which in general gives a better
        // performance especially in older hardware. In this case we can't
        // skip fvarDC without changing CodeGen logic, but we can skip
        // shaderDC for culling.
        //
        if (_useDrawArrays) {
            if (_useGpuInstanceCulling) {
                // cull indirect command
                _dispatchBufferCullInput->AddBufferResourceView(
                    HdTokens->drawDispatch, GL_INT, 1,
                    offsetof(_DrawArraysInstanceCullCommand, cullCount));
                // cull drawing coord 0
                _dispatchBufferCullInput->AddBufferResourceView(
                    HdTokens->drawingCoord0, GL_INT, 4,
                    offsetof(_DrawArraysInstanceCullCommand, modelDC));
                // cull drawing coord 1
                _dispatchBufferCullInput->AddBufferResourceView(
                    HdTokens->drawingCoord1, GL_INT, 2, // see the comment above
                    offsetof(_DrawArraysInstanceCullCommand, fvarDC));
                // cull instance drawing coord
                if (instancerNumLevels > 0) {
                    _dispatchBufferCullInput->AddBufferResourceView(
                        HdTokens->drawingCoordI, GL_INT, instancerNumLevels,
                        sizeof(_DrawArraysInstanceCullCommand));
                }
                // cull draw index
                _dispatchBufferCullInput->AddBufferResourceView(
                    HdTokens->drawCommandIndex, GL_INT, 1,
                    offsetof(_DrawArraysInstanceCullCommand, baseInstance));
            } else {
                // cull indirect command
                _dispatchBufferCullInput->AddBufferResourceView(
                    HdTokens->drawDispatch, GL_INT, 1,
                    offsetof(_DrawArraysCommand, count));
                // cull drawing coord 0
                _dispatchBufferCullInput->AddBufferResourceView(
                    HdTokens->drawingCoord0, GL_INT, 4,
                    offsetof(_DrawArraysCommand, modelDC));
                // cull instance count input
                _dispatchBufferCullInput->AddBufferResourceView(
                    HdTokens->instanceCountInput, GL_INT, 1,
                    offsetof(_DrawArraysCommand, instanceCount));
            }
        } else {
            if (_useGpuInstanceCulling) {
                // cull indirect command
                _dispatchBufferCullInput->AddBufferResourceView(
                    HdTokens->drawDispatch, GL_INT, 1,
                    offsetof(_DrawElementsInstanceCullCommand, cullCount));
                // cull drawing coord 0
                _dispatchBufferCullInput->AddBufferResourceView(
                    HdTokens->drawingCoord0, GL_INT, 4,
                    offsetof(_DrawElementsInstanceCullCommand, modelDC));
                // cull drawing coord 1
                _dispatchBufferCullInput->AddBufferResourceView(
                    HdTokens->drawingCoord1, GL_INT, 2, // see the comment above
                    offsetof(_DrawElementsInstanceCullCommand, fvarDC));
                // cull instance drawing coord
                if (instancerNumLevels > 0) {
                    _dispatchBufferCullInput->AddBufferResourceView(
                        HdTokens->drawingCoordI, GL_INT, instancerNumLevels,
                        sizeof(_DrawElementsInstanceCullCommand));
                }
                // cull draw index
                _dispatchBufferCullInput->AddBufferResourceView(
                    HdTokens->drawCommandIndex, GL_INT, 1,
                    offsetof(_DrawElementsInstanceCullCommand, baseInstance));
            } else {
                // cull indirect command
                _dispatchBufferCullInput->AddBufferResourceView(
                    HdTokens->drawDispatch, GL_INT, 1,
                    offsetof(_DrawElementsCommand, count));
                // cull drawing coord 0
                _dispatchBufferCullInput->AddBufferResourceView(
                    HdTokens->drawingCoord0, GL_INT, 4,
                    offsetof(_DrawElementsCommand, modelDC));
                // cull instance count input
                _dispatchBufferCullInput->AddBufferResourceView(
                    HdTokens->instanceCountInput, GL_INT, 1,
                    offsetof(_DrawElementsCommand, instanceCount));
            }
        }

        // copy data
        _dispatchBufferCullInput->CopyData(_drawCommandBuffer);
    }

    // cache the location of instanceCount, to be used at
    // DrawItemInstanceChanged().
    if (_useDrawArrays) {
        if (_useGpuInstanceCulling) {
            _instanceCountOffset =
                offsetof(_DrawArraysInstanceCullCommand, instanceCount)/sizeof(GLuint);
            _cullInstanceCountOffset =
                offsetof(_DrawArraysInstanceCullCommand, cullInstanceCount)/sizeof(GLuint);
        } else {
            _instanceCountOffset = _cullInstanceCountOffset =
                offsetof(_DrawArraysCommand, instanceCount)/sizeof(GLuint);
        }
    } else {
        if (_useGpuInstanceCulling) {
            _instanceCountOffset =
                offsetof(_DrawElementsInstanceCullCommand, instanceCount)/sizeof(GLuint);
            _cullInstanceCountOffset =
                offsetof(_DrawElementsInstanceCullCommand, cullInstanceCount)/sizeof(GLuint);
        } else {
            _instanceCountOffset = _cullInstanceCountOffset =
                offsetof(_DrawElementsCommand, instanceCount)/sizeof(GLuint);
        }
    }
}

bool
Hd_IndirectDrawBatch::Validate(bool deepValidation)
{
    if (not TF_VERIFY(not _drawItemInstances.empty())) return false;

    // check the hash to see they've been reallocated/migrated or not.
    // note that we just need to compare the hash of the first item,
    // since drawitems are aggregated and ensure that they are sharing
    // same buffer arrays.

    HdDrawItem const* batchItem = _drawItemInstances.front()->GetDrawItem();

    size_t bufferArraysHash = batchItem->GetBufferArraysHash();

    if (_bufferArraysHash != bufferArraysHash) {
        _bufferArraysHash = bufferArraysHash;
        _dispatchBuffer.reset();
        return false;
    }

    // Deep validation is needed when a drawItem changes its buffer spec,
    // surface shader or geometric shader.
    if (deepValidation) {
        // look through all draw items to be still compatible

        size_t numDrawItemInstances = _drawItemInstances.size();
        for (size_t item = 0; item < numDrawItemInstances; ++item) {
            HdDrawItem const * drawItem
                = _drawItemInstances[item]->GetDrawItem();

            if (not _IsAggregated(batchItem, drawItem)) {
                return false;
            }
        }

    }

    return true;
}

void
Hd_IndirectDrawBatch::_ValidateCompatibility(
            HdBufferArrayRangeSharedPtr const& constantBar,
            HdBufferArrayRangeSharedPtr const& indexBar,
            HdBufferArrayRangeSharedPtr const& elementBar,
            HdBufferArrayRangeSharedPtr const& fvarBar,
            HdBufferArrayRangeSharedPtr const& vertexBar,
            int instancerNumLevels,
            HdBufferArrayRangeSharedPtr const& instanceIndexBar,
            std::vector<HdBufferArrayRangeSharedPtr> const& instanceBars) const
{
    HdDrawItem const* failed = nullptr;

    for (HdDrawItemInstance const* itemInstance : _drawItemInstances) {
        HdDrawItem const* itm = itemInstance->GetDrawItem();

        if (constantBar and not TF_VERIFY(constantBar 
                        ->IsAggregatedWith(itm->GetConstantPrimVarRange())))
                        { failed = itm; break; }
        if (indexBar and not TF_VERIFY(indexBar
                        ->IsAggregatedWith(itm->GetTopologyRange())))
                        { failed = itm; break; }
        if (elementBar and not TF_VERIFY(elementBar
                        ->IsAggregatedWith(itm->GetElementPrimVarRange())))
                        { failed = itm; break; }
        if (fvarBar and not TF_VERIFY(fvarBar
                        ->IsAggregatedWith(itm->GetFaceVaryingPrimVarRange())))
                        { failed = itm; break; }
        if (vertexBar and not TF_VERIFY(vertexBar
                        ->IsAggregatedWith(itm->GetVertexPrimVarRange())))
                        { failed = itm; break; }
        if (not TF_VERIFY(instancerNumLevels
                        == itm->GetInstancePrimVarNumLevels()))
                        { failed = itm; break; }
        if (instanceIndexBar and not TF_VERIFY(instanceIndexBar
                        ->IsAggregatedWith(itm->GetInstanceIndexRange())))
                        { failed = itm; break; }
        if (not TF_VERIFY(instancerNumLevels == (int)instanceBars.size()))
                        { failed = itm; break; }

        std::vector<HdBufferArrayRangeSharedPtr> itmInstanceBars(
                                                            instancerNumLevels);
        if (instanceIndexBar) {
            for (int i = 0; i < instancerNumLevels; ++i) {
                if (itmInstanceBars[i] and not TF_VERIFY(itmInstanceBars[i] 
                            ->IsAggregatedWith(itm->GetInstancePrimVarRange(i)),
                        "%d", i)) { failed = itm; break; }
            }
        }
    }

    if (failed) {
        std::cout << failed->GetRprimID() << std::endl;
    }
}

void
Hd_IndirectDrawBatch::PrepareDraw(
    HdRenderPassStateSharedPtr const &renderPassState)
{
    HD_TRACE_FUNCTION();
    if (not glBindBuffer) return; // glew initialized

    //
    // compile
    //

    if (not _dispatchBuffer) {
        _CompileBatch();
    }

    // there is no non-zero draw items.
    if ((    _useDrawArrays and _numTotalVertices == 0) or
        (not _useDrawArrays and _numTotalElements == 0)) return;

    HdDrawItem const* batchItem = _drawItemInstances.front()->GetDrawItem();

    // Bypass freezeCulling if the command buffer is dirty.
    bool freezeCulling = TfDebug::IsEnabled(HD_FREEZE_CULL_FRUSTUM)
                                and not _drawCommandBufferDirty;

    bool gpuCulling = _useGpuCulling;

    if (gpuCulling and not _useGpuInstanceCulling) {
        // disable GPU culling when instancing enabled and
        // not using instance culling.
        if (batchItem->GetInstanceIndexRange()) gpuCulling = false;
    }

    // Do we have to update our dispach buffer because drawitem instance
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

    if (gpuCulling and not freezeCulling) {
        if (_useGpuInstanceCulling) {
            _GPUFrustumCulling(batchItem, renderPassState);
        } else {
            _GPUFrustumCullingXFB(batchItem, renderPassState);
        }
    }

    if (TfDebug::IsEnabled(HD_DRAWITEM_DRAWN)) {
        void const *bufferData = NULL;
        // instanceCount is a second entry of drawcommand for both
        // DrawArraysIndirect and DrawElementsIndirect.
        const void *instanceCountOffset =
            (const void*)
            (_dispatchBuffer->GetResource(HdTokens->drawDispatch)->GetOffset()
             + sizeof(GLuint));
        const int dispatchBufferStride =
            _dispatchBuffer->GetEntireResource()->GetStride();

        HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();
        if (gpuCulling) {
            if (caps.directStateAccessEnabled) {
                bufferData = glMapNamedBufferEXT(
                    _dispatchBuffer->GetEntireResource()->GetId(),
                    GL_READ_ONLY);
            } else {
                glBindBuffer(GL_ARRAY_BUFFER,
                             _dispatchBuffer->GetEntireResource()->GetId());
                bufferData = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
        }

        for (size_t item=0; item<_drawItemInstances.size(); ++item) {
            HdDrawItemInstance const * drawItemInstance =
                _drawItemInstances[item];

            if(not drawItemInstance->IsVisible()) {
                continue;
            }

            HdDrawItem const * drawItem = drawItemInstance->GetDrawItem();

            if (gpuCulling) {
                GLint const *instanceCount =
                    (GLint const *)(
                        (ptrdiff_t)(bufferData)
                        + (ptrdiff_t)(instanceCountOffset)
                        + item*dispatchBufferStride);

                bool isVisible = (*instanceCount > 0);
                if (not isVisible) {
                    continue;
                }
            }

            std::stringstream ss;
            ss << *drawItem;
            TF_DEBUG(HD_DRAWITEM_DRAWN).Msg("PREP DRAW: \n%s\n", 
                    ss.str().c_str());
        }

        if (gpuCulling) {
            if (caps.directStateAccessEnabled) {
                glUnmapNamedBufferEXT(_dispatchBuffer->GetEntireResource()->GetId());
            } else {
                glBindBuffer(GL_ARRAY_BUFFER,
                             _dispatchBuffer->GetEntireResource()->GetId());
                glUnmapBuffer(GL_ARRAY_BUFFER);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
        }
    }

    if (gpuCulling and not freezeCulling) {
        if (IsEnabledGPUCountVisibleInstances()) {
            _EndGPUCountVisibleInstances(_cullResultSync, &_numVisibleItems);
            glDeleteSync(_cullResultSync);
            _cullResultSync = 0;
        }
    }
}

void
Hd_IndirectDrawBatch::ExecuteDraw(
    HdRenderPassStateSharedPtr const &renderPassState)
{
    HD_TRACE_FUNCTION();

    if (not glBindBuffer) return; // glew initialized

    if (not TF_VERIFY(not _drawItemInstances.empty())) return;

    HdDrawItem const* batchItem = _drawItemInstances.front()->GetDrawItem();

    if (not TF_VERIFY(batchItem)) return;

    if (not TF_VERIFY(_dispatchBuffer)) return;

    // there is no non-zero draw items.
    if ((    _useDrawArrays and _numTotalVertices == 0) or
        (not _useDrawArrays and _numTotalElements == 0)) return;

    //
    // draw
    //

    // bind program
    _DrawingProgram & program = _GetDrawingProgram(renderPassState,
                                                   /*indirect=*/true);
    HdGLSLProgramSharedPtr const &glslProgram = program.GetGLSLProgram();
    if (not TF_VERIFY(glslProgram)) return;
    if (not TF_VERIFY(glslProgram->Validate())) return;

    GLuint programId = glslProgram->GetProgram().GetId();
    TF_VERIFY(programId);

    glUseProgram(programId);

    const Hd_ResourceBinder &binder = program.GetBinder();
    const HdShaderSharedPtrVector &shaders = program.GetComposedShaders();

    // XXX: for surfaces shader, we need to iterate all drawItems to
    //      make textures resident, instead of just the first batchItem
    TF_FOR_ALL(it, shaders) {
        (*it)->BindResources(binder, programId);
    }

    // constant buffer bind
    HdBufferArrayRangeSharedPtr constantBar = batchItem->GetConstantPrimVarRange();
    binder.BindConstantBuffer(constantBar);

    // index buffer bind
    HdBufferArrayRangeSharedPtr indexBar = batchItem->GetTopologyRange();
    binder.BindBufferArray(indexBar);

    // element buffer bind
    HdBufferArrayRangeSharedPtr elementBar = batchItem->GetElementPrimVarRange();
    binder.BindBufferArray(elementBar);

    // fvar buffer bind
    HdBufferArrayRangeSharedPtr fvarBar = batchItem->GetFaceVaryingPrimVarRange();
    binder.BindBufferArray(fvarBar);

    // vertex buffer bind
    HdBufferArrayRangeSharedPtr vertexBar = batchItem->GetVertexPrimVarRange();
    binder.BindBufferArray(vertexBar);

    // instance buffer bind
    int instancerNumLevels = batchItem->GetInstancePrimVarNumLevels();
    std::vector<HdBufferArrayRangeSharedPtr> instanceBars(instancerNumLevels);

    // intance index indirection
    HdBufferArrayRangeSharedPtr instanceIndexBar = batchItem->GetInstanceIndexRange();
    if (instanceIndexBar) {
        // note that while instanceIndexBar is mandatory for instancing but
        // instanceBar can technically be empty (it doesn't make sense though)
        // testHdInstance --noprimvars covers that case.
        for (int i = 0; i < instancerNumLevels; ++i) {
            instanceBars[i] = batchItem->GetInstancePrimVarRange(i);
            binder.BindInstanceBufferArray(instanceBars[i], i);
        }
        binder.BindBufferArray(instanceIndexBar);
    }

    if (false and ARCH_UNLIKELY(TfDebug::IsEnabled(HD_SAFE_MODE))) {
        _ValidateCompatibility(constantBar,
                               indexBar,
                               elementBar,
                               fvarBar,
                               vertexBar,
                               instancerNumLevels,
                               instanceIndexBar,
                               instanceBars);
    }

    // shader buffer bind
    HdBufferArrayRangeSharedPtr shaderBar;
    TF_FOR_ALL(shader, shaders) {
        shaderBar = (*shader)->GetShaderData();
        if (shaderBar) {
            binder.BindBuffer(HdTokens->surfaceShaderParams, 
                              shaderBar->GetResource());
        }
    }

    // drawindirect command, drawing coord, instanceIndexBase bind
    HdBufferArrayRangeSharedPtr dispatchBar =
        _dispatchBuffer->GetBufferArrayRange();
    binder.BindBufferArray(dispatchBar);

    // update geometric shader states
    program.GetGeometricShader()->BindResources(binder, programId);

    GLuint batchCount = _dispatchBuffer->GetCount();

    TF_DEBUG(HD_DRAWITEM_DRAWN).Msg("DRAW (indirect): %d\n", batchCount);

    if (_useDrawArrays) {
        TF_DEBUG(HD_MDI).Msg("MDI Drawing Arrays:\n"
                " - primitive mode: %d\n"
                " - indirect: %d\n"
                " - drawCount: %d\n"
                " - stride: %zu\n",
               program.GetGeometricShader()->GetPrimitiveMode(),
               0, batchCount,
               _dispatchBuffer->GetCommandNumUints()*sizeof(GLuint));

        glMultiDrawArraysIndirect(
            program.GetGeometricShader()->GetPrimitiveMode(),
            0, // draw command always starts with 0
            batchCount,
            _dispatchBuffer->GetCommandNumUints()*sizeof(GLuint));
    } else {
        TF_DEBUG(HD_MDI).Msg("MDI Drawing Elements:\n"
                " - primitive mode: %d\n"
                " - buffer type: GL_UNSIGNED_INT\n"
                " - indirect: %d\n"
                " - drawCount: %d\n"
                " - stride: %zu\n",
               program.GetGeometricShader()->GetPrimitiveMode(),
               0, batchCount,
               _dispatchBuffer->GetCommandNumUints()*sizeof(GLuint));

        glMultiDrawElementsIndirect(
            program.GetGeometricShader()->GetPrimitiveMode(),
            GL_UNSIGNED_INT,
            0, // draw command always starts with 0
            batchCount,
            _dispatchBuffer->GetCommandNumUints()*sizeof(GLuint));
    }

    HD_PERF_COUNTER_INCR(HdPerfTokens->drawCalls);
    HD_PERF_COUNTER_ADD(HdTokens->itemsDrawn, _numVisibleItems);

    //
    // cleanup
    //
    binder.UnbindConstantBuffer(constantBar);
    binder.UnbindBufferArray(elementBar);
    binder.UnbindBufferArray(fvarBar);
    binder.UnbindBufferArray(indexBar);
    binder.UnbindBufferArray(vertexBar);
    binder.UnbindBufferArray(dispatchBar);
    if(shaderBar) {
        binder.UnbindBuffer(HdTokens->surfaceShaderParams, 
                            shaderBar->GetResource());
    }

    if (instanceIndexBar) {
        for (int i = 0; i < instancerNumLevels; ++i) {
            binder.UnbindInstanceBufferArray(instanceBars[i], i);
        }
        binder.UnbindBufferArray(instanceIndexBar);
    }

    TF_FOR_ALL(it, shaders) {
        (*it)->UnbindResources(binder, programId);
    }
    program.GetGeometricShader()->UnbindResources(binder, programId);

    glUseProgram(0);
}

void
Hd_IndirectDrawBatch::_GPUFrustumCulling(
    HdDrawItem const *batchItem,
    HdRenderPassStateSharedPtr const &renderPassState)
{
    HdBufferArrayRangeSharedPtr constantBar =
        batchItem->GetConstantPrimVarRange();
    int instancerNumLevels = batchItem->GetInstancePrimVarNumLevels();
    std::vector<HdBufferArrayRangeSharedPtr> instanceBars(instancerNumLevels);
    for (int i = 0; i < instancerNumLevels; ++i) {
        instanceBars[i] = batchItem->GetInstancePrimVarRange(i);
    }
    HdBufferArrayRangeSharedPtr instanceIndexBar =
        batchItem->GetInstanceIndexRange();
    HdBufferArrayRangeSharedPtr cullDispatchBar =
        _dispatchBufferCullInput->GetBufferArrayRange();

    _CullingProgram cullingProgram = _GetCullingProgram();
    HdGLSLProgramSharedPtr const &
        glslProgram = cullingProgram.GetGLSLProgram();

    if (not TF_VERIFY(glslProgram)) return;
    if (not TF_VERIFY(glslProgram->Validate())) return;

    // We perform frustum culling on the GPU using transform feedback,
    // stomping the instanceCount of each drawing command in the
    // dispatch buffer to 0 for primitives that are culled, skipping
    // over other elements.

    const Hd_ResourceBinder &binder = cullingProgram.GetBinder();

    GLuint programId = glslProgram->GetProgram().GetId();
    glUseProgram(programId);

    // bind buffers
    binder.BindConstantBuffer(constantBar);

    // bind per-drawitem attribute (drawingCoord, instanceCount, drawCommand)
    binder.BindBufferArray(cullDispatchBar);

    if (instanceIndexBar) {
        int instancerNumLevels = batchItem->GetInstancePrimVarNumLevels();
        for (int i = 0; i < instancerNumLevels; ++i) {
            binder.BindInstanceBufferArray(instanceBars[i], i);
        }
        binder.BindBufferArray(instanceIndexBar);
    }

    if (IsEnabledGPUCountVisibleInstances()) {
        _BeginGPUCountVisibleInstances();
    }

    // bind destination buffer (using entire buffer bind to start from offset=0)
    binder.BindBuffer(HdTokens->dispatchBuffer,
                      _dispatchBuffer->GetEntireResource());

    // set cull parameters
    unsigned int drawCommandNumUints = _dispatchBuffer->GetCommandNumUints();
    GfMatrix4f cullMatrix(renderPassState->GetCullMatrix());
    GfVec2f drawRangeNDC(renderPassState->GetDrawingRangeNDC());
    binder.BindUniformui(HdTokens->ulocDrawCommandNumUints, 1, &drawCommandNumUints);
    binder.BindUniformf(HdTokens->ulocCullMatrix, 16, cullMatrix.GetArray());
    binder.BindUniformf(HdTokens->ulocDrawRangeNDC, 2, drawRangeNDC.GetArray());

    // run culling shader
    bool validProgram = true;

    // XXX: should we cache cull command offset?
    HdBufferResourceSharedPtr cullCommandBuffer =
        _dispatchBufferCullInput->GetResource(HdTokens->drawDispatch);
    if (not TF_VERIFY(cullCommandBuffer)) {
        validProgram = false;
    }

    if (validProgram) {
        glEnable(GL_RASTERIZER_DISCARD);

        int resetPass = 1;
        binder.BindUniformi(HdTokens->ulocResetPass, 1, &resetPass);
        glMultiDrawArraysIndirect(
            GL_POINTS,
            reinterpret_cast<const GLvoid*>((intptr_t)(cullCommandBuffer->GetOffset())),
            _dispatchBufferCullInput->GetCount(),
            cullCommandBuffer->GetStride());

        // dispatch buffer is bound via SSBO
        // (see _CullingProgram::_GetCustomBindings)
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        resetPass = 0;
        binder.BindUniformi(HdTokens->ulocResetPass, 1, &resetPass);
        glMultiDrawArraysIndirect(
            GL_POINTS,
            reinterpret_cast<const GLvoid*>((intptr_t)(cullCommandBuffer->GetOffset())),
            _dispatchBufferCullInput->GetCount(),
            cullCommandBuffer->GetStride());

        glDisable(GL_RASTERIZER_DISCARD);
    }

    // Reset all vertex attribs and their divisors. Note that the drawing
    // program has different bindings from the culling program does
    // in general, even though most of buffers will likely be assigned
    // with same attrib divisors again.
    binder.UnbindConstantBuffer(constantBar);
    binder.UnbindBufferArray(cullDispatchBar);
    if (instanceIndexBar) {
        int instancerNumLevels = batchItem->GetInstancePrimVarNumLevels();
        for (int i = 0; i < instancerNumLevels; ++i) {
            binder.UnbindInstanceBufferArray(instanceBars[i], i);
        }
        binder.UnbindBufferArray(instanceIndexBar);
    }

    // unbind destination dispatch buffer
    binder.UnbindBuffer(HdTokens->dispatchBuffer,
                        _dispatchBuffer->GetEntireResource());

    // make sure the culling results (instanceIndices and instanceCount)
    // are synchronized for the next drawing.
    glMemoryBarrier(
        GL_COMMAND_BARRIER_BIT |         // instanceCount for MDI
        GL_SHADER_STORAGE_BARRIER_BIT |  // instanceCount for shader
        GL_UNIFORM_BARRIER_BIT);         // instanceIndices

    // a fence has to be added after the memory barrier.
    if (IsEnabledGPUCountVisibleInstances()) {
        _cullResultSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    } else {
        _cullResultSync = 0;
    }

}

void
Hd_IndirectDrawBatch::_GPUFrustumCullingXFB(
    HdDrawItem const *batchItem,
    HdRenderPassStateSharedPtr const &renderPassState)
{
    HdBufferArrayRangeSharedPtr constantBar =
        batchItem->GetConstantPrimVarRange();

    HdBufferArrayRangeSharedPtr cullDispatchBar =
        _dispatchBufferCullInput->GetBufferArrayRange();

    _CullingProgram &cullingProgram = _GetCullingProgram();

    HdGLSLProgramSharedPtr const &
        glslProgram = cullingProgram.GetGLSLProgram();
    if (not TF_VERIFY(glslProgram)) return;
    if (not TF_VERIFY(glslProgram->Validate())) return;

    // We perform frustum culling on the GPU using transform feedback,
    // stomping the instanceCount of each drawing command in the
    // dispatch buffer to 0 for primitives that are culled, skipping
    // over other elements.

    GLuint programId = glslProgram->GetProgram().GetId();
    glUseProgram(programId);

    const Hd_ResourceBinder &binder = cullingProgram.GetBinder();

    // bind constant
    binder.BindConstantBuffer(constantBar);
    // bind drawing coord, instance count
    binder.BindBufferArray(cullDispatchBar);

    if (IsEnabledGPUCountVisibleInstances()) {
        _BeginGPUCountVisibleInstances();
    }

    // set cull parameters
    GfMatrix4f cullMatrix(renderPassState->GetCullMatrix());
    GfVec2f drawRangeNDC(renderPassState->GetDrawingRangeNDC());
    binder.BindUniformf(HdTokens->ulocCullMatrix, 16, cullMatrix.GetArray());
    binder.BindUniformf(HdTokens->ulocDrawRangeNDC, 2, drawRangeNDC.GetArray());

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0,
                     _dispatchBuffer->GetEntireResource()->GetId());
    glBeginTransformFeedback(GL_POINTS);

    glEnable(GL_RASTERIZER_DISCARD);
    glDrawArrays(GL_POINTS, 0, _dispatchBufferCullInput->GetCount());
    glDisable(GL_RASTERIZER_DISCARD);

    if (IsEnabledGPUCountVisibleInstances()) {
        glMemoryBarrier(GL_TRANSFORM_FEEDBACK_BARRIER_BIT);
        _cullResultSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    } else {
        _cullResultSync = 0;
    }

    glEndTransformFeedback();
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);

    // unbind all
    binder.UnbindConstantBuffer(constantBar);
    binder.UnbindBufferArray(cullDispatchBar);

    glUseProgram(0);
}

void
Hd_IndirectDrawBatch::DrawItemInstanceChanged(HdDrawItemInstance const* instance)
{
    // We need to check the visiblity and update if needed
    if (_dispatchBuffer) {
        size_t batchIndex = instance->GetBatchIndex();
        int commandNumUints = _dispatchBuffer->GetCommandNumUints();
        int numLevels = instance->GetDrawItem()->GetInstancePrimVarNumLevels();
        int instanceIndexWidth = numLevels + 1;

        // When XFB culling is being used, cullcommand points the same location
        // as drawcommands. Then we update the same place twice, it would be ok
        // than branching.
        std::vector<GLuint>::iterator instanceCountIt =
            _drawCommandBuffer.begin()
            + batchIndex * commandNumUints
            + _instanceCountOffset;
        std::vector<GLuint>::iterator cullInstanceCountIt =
            _drawCommandBuffer.begin()
            + batchIndex * commandNumUints
            + _cullInstanceCountOffset;

        HdBufferArrayRangeSharedPtr const &instanceIndexBar =
            instance->GetDrawItem()->GetInstanceIndexRange();
        int newInstanceCount = instanceIndexBar
                             ? instanceIndexBar->GetNumElements() : 1;
        newInstanceCount = instance->IsVisible()
                         ? (newInstanceCount/std::max(1, instanceIndexWidth))
                         : 0;

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
Hd_IndirectDrawBatch::_BeginGPUCountVisibleInstances()
{
    if (not _resultBuffer) {
        HdResourceRegistry *resourceRegistry =
                &HdResourceRegistry::GetInstance();

        _resultBuffer = 
            resourceRegistry->RegisterPersistentBuffer(
                HdTokens->drawIndirectResult, sizeof(GLint), 0);
    }

    // Reset visible item count
    if (_resultBuffer->GetMappedAddress()) {
        *((GLint *)_resultBuffer->GetMappedAddress()) = 0;
    } else {
        GLint count = 0;
        HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();
        if (caps.directStateAccessEnabled) {
            glNamedBufferSubDataEXT(_resultBuffer->GetId(), 0,
                                    sizeof(count), &count);
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, _resultBuffer->GetId());
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(count), &count);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }

    // XXX: temporarily hack during refactoring.
    // we'd like to use the same API as other buffers.
    int binding = _cullingProgram.GetBinder().GetBinding(
        HdTokens->drawIndirectResult).GetLocation();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, _resultBuffer->GetId());
}

void
Hd_IndirectDrawBatch::_EndGPUCountVisibleInstances(GLsync resultSync, size_t * result)
{
    GLenum status = glClientWaitSync(resultSync,
            GL_SYNC_FLUSH_COMMANDS_BIT, HD_CULL_RESULT_TIMEOUT_NS);

    if (status != GL_ALREADY_SIGNALED and status != GL_CONDITION_SATISFIED) {
        // We could loop, but we don't expect to timeout.
        TF_RUNTIME_ERROR("Unexpected ClientWaitSync timeout");
        *result = 0;
        return;
    }

    // Return visible item count
    if (_resultBuffer->GetMappedAddress()) {
        *result = *((GLint *)_resultBuffer->GetMappedAddress());
    } else {
        GLint count = 0;
        HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();
        if (caps.directStateAccessEnabled) {
            glGetNamedBufferSubDataEXT(_resultBuffer->GetId(), 0,
                                       sizeof(count), &count);
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, _resultBuffer->GetId());
            glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(count), &count);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        *result = count;
    }

    // XXX: temporarily hack during refactoring.
    // we'd like to use the same API as other buffers.
    int binding = _cullingProgram.GetBinder().GetBinding(
        HdTokens->drawIndirectResult).GetLocation();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, 0);
}

void
Hd_IndirectDrawBatch::_CullingProgram::Initialize(
    bool useDrawArrays, bool useInstanceCulling, size_t bufferArrayHash)
{
    if (useDrawArrays      != _useDrawArrays or
        useInstanceCulling != _useInstanceCulling or
        bufferArrayHash    != _bufferArrayHash) {
        // reset shader
        Reset();
    }

    _useDrawArrays = useDrawArrays;
    _useInstanceCulling = useInstanceCulling;
    _bufferArrayHash = bufferArrayHash;
}

/* virtual */
void
Hd_IndirectDrawBatch::_CullingProgram::_GetCustomBindings(
    HdBindingRequestVector *customBindings,
    bool *enableInstanceDraw) const
{
    if (not TF_VERIFY(enableInstanceDraw) or
        not TF_VERIFY(customBindings)) return;

    customBindings->push_back(HdBindingRequest(HdBinding::SSBO,
                                  HdTokens->drawIndirectResult));
    customBindings->push_back(HdBindingRequest(HdBinding::SSBO,
                                  HdTokens->dispatchBuffer));
    customBindings->push_back(HdBindingRequest(HdBinding::UNIFORM,
                                               HdTokens->ulocDrawRangeNDC));
    customBindings->push_back(HdBindingRequest(HdBinding::UNIFORM,
                                               HdTokens->ulocCullMatrix));

    if (_useInstanceCulling) {
        customBindings->push_back(
            HdBindingRequest(HdBinding::DRAW_INDEX_INSTANCE,
                HdTokens->drawCommandIndex));
        customBindings->push_back(
            HdBindingRequest(HdBinding::UNIFORM,
                HdTokens->ulocDrawCommandNumUints));
        customBindings->push_back(
            HdBindingRequest(HdBinding::UNIFORM,
                HdTokens->ulocResetPass));
    } else {
        // XFB culling
        customBindings->push_back(
            HdBindingRequest(HdBinding::DRAW_INDEX,
                HdTokens->instanceCountInput));
    }

    // set instanceDraw true if instanceCulling is enabled.
    // this value will be used to determine if glVertexAttribDivisor needs to
    // be enabled or not.
    *enableInstanceDraw = _useInstanceCulling;
}

/* virtual */
bool
Hd_IndirectDrawBatch::_CullingProgram::_Link(
        HdGLSLProgramSharedPtr const & glslProgram)
{
    if (not TF_VERIFY(glslProgram)) return false;
    if (not glTransformFeedbackVaryings) return false; // glew initialized

    if (not _useInstanceCulling) {
        // This must match the layout of draw command.
        // (WBN to encode this in the shader using GL_ARB_enhanced_layouts
        // but that's not supported in 319.32)

        // CAUTION: this is currently padded to match drawElementsOutputs, since
        // our shader hash cannot take the XFB varying configuration into
        // account.
        const char *drawArraysOutputs[] = {
            "gl_SkipComponents1",  // count
            "resultInstanceCount", // instanceCount
            "gl_SkipComponents4",  // firstIndex - modelDC
                                   // (includes __reserved_0 to match drawElementsOutput)
            "gl_SkipComponents4",  // constantDC - fvarDC
            "gl_SkipComponents2",  // instanceIndexDC - shaderDC
        };
        const char *drawElementsOutputs[] = {
            "gl_SkipComponents1",  // count
            "resultInstanceCount", // instanceCount
            "gl_SkipComponents4",  // firstIndex - modelDC
            "gl_SkipComponents4",  // constantDC - fvarDC
            "gl_SkipComponents2",  // instanceIndexDC - shaderDC
        };
        const char **outputs = _useDrawArrays
            ? drawArraysOutputs
            : drawElementsOutputs;

        const int nOutputs = 5;
        BOOST_STATIC_ASSERT(
            sizeof(drawArraysOutputs)/sizeof(drawArraysOutputs[0])
            == nOutputs);
        BOOST_STATIC_ASSERT(
            sizeof(drawElementsOutputs)/sizeof(drawElementsOutputs[0])
            == nOutputs);
        glTransformFeedbackVaryings(glslProgram->GetProgram().GetId(),
                                    nOutputs,
                                    outputs, GL_INTERLEAVED_ATTRIBS);
    }

    return Hd_DrawBatch::_DrawingProgram::_Link(glslProgram);
}
