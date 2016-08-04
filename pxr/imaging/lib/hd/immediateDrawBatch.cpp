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

#include "pxr/imaging/hd/immediateDrawBatch.h"
#include "pxr/imaging/hd/drawItemInstance.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/commandBuffer.h"
#include "pxr/imaging/hd/debugCodes.h"
#include "pxr/imaging/hd/geometricShader.h"
#include "pxr/imaging/hd/glslProgram.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/shader.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/iterator.h"

Hd_ImmediateDrawBatch::Hd_ImmediateDrawBatch(
    HdDrawItemInstance * drawItemInstance)
    : Hd_DrawBatch(drawItemInstance)
{
    _Init(drawItemInstance);
}

void
Hd_ImmediateDrawBatch::_Init(HdDrawItemInstance * drawItemInstance)
{
    Hd_DrawBatch::_Init(drawItemInstance);
    drawItemInstance->SetBatchIndex(0);
    drawItemInstance->SetBatch(this);
}

Hd_ImmediateDrawBatch::~Hd_ImmediateDrawBatch()
{
}

bool
Hd_ImmediateDrawBatch::Validate(bool deepValidation)
{
    if (not TF_VERIFY(not _drawItemInstances.empty())) return false;

    HdDrawItem const* batchItem = _drawItemInstances.front()->GetDrawItem();

    // immediate batch doesn't need to verify buffer array hash unlike indirect
    // batch.
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
Hd_ImmediateDrawBatch::PrepareDraw(
    HdRenderPassStateSharedPtr const &renderPassState)
{
}

void
Hd_ImmediateDrawBatch::ExecuteDraw(
    HdRenderPassStateSharedPtr const &renderPassState)
{
    HD_TRACE_FUNCTION();

    HdBufferArrayRangeSharedPtr indexBarCurrent;
    HdBufferArrayRangeSharedPtr elementBarCurrent;
    HdBufferArrayRangeSharedPtr vertexBarCurrent;
    HdBufferArrayRangeSharedPtr constantBarCurrent;
    HdBufferArrayRangeSharedPtr fvarBarCurrent;
    HdBufferArrayRangeSharedPtr instanceIndexBarCurrent;
    HdBufferArrayRangeSharedPtr shaderBarCurrent;
    std::vector<HdBufferArrayRangeSharedPtr> instanceBarCurrents;

    if (_drawItemInstances.empty()) return;

    if (not glUseProgram) return;  // glew initialized

    // bind program
    _DrawingProgram & program = _GetDrawingProgram(renderPassState,
                                                   /*indirect=*/false);

    HdGLSLProgramSharedPtr const &glslProgram = program.GetGLSLProgram();
    if (not TF_VERIFY(glslProgram)) return;
    if (not TF_VERIFY(glslProgram->Validate())) return;

    const Hd_ResourceBinder &binder = program.GetBinder();

    GLuint programId = glslProgram->GetProgram().GetId();
    TF_VERIFY(programId);

    glUseProgram(programId);

    HdShaderSharedPtrVector shaders
        = renderPassState->GetShaders(/*surfaceShader=*/HdShaderSharedPtr());
    bool hasOverrideShader = bool(renderPassState->GetOverrideShader());

    TF_FOR_ALL(it, shaders) {
        (*it)->BindResources(binder, programId);
    }

    // Set up geometric shader states
    // all batch item should have the same geometric shader.
    HdDrawItem const* batchItem = _drawItemInstances.front()->GetDrawItem();
    Hd_GeometricShaderSharedPtr const &geometricShader
        = batchItem->GetGeometricShader();
    geometricShader->BindResources(binder, programId);

    size_t numItemsDrawn = 0;
    TF_FOR_ALL(drawItemIt, _drawItemInstances) {
        if(not (*drawItemIt)->IsVisible()) {
            continue;
        }

        HdDrawItem const * drawItem = (*drawItemIt)->GetDrawItem();

        ++numItemsDrawn;
        if (TfDebug::IsEnabled(HD_DRAWITEM_DRAWN)) {
            std::stringstream ss;
            ss << *drawItem;
            TF_DEBUG(HD_DRAWITEM_DRAWN).Msg("DRAW: \n%s\n", 
                    ss.str().c_str());
        }

        //
        // index buffer data
        //
        HdBufferArrayRangeSharedPtr const & indexBar =
            drawItem->GetTopologyRange();
        if (indexBar and (not indexBar->IsAggregatedWith(indexBarCurrent))) {
            binder.UnbindBufferArray(indexBarCurrent);
            binder.BindBufferArray(indexBar);
            indexBarCurrent = indexBar;
        }

        //
        // per-face buffer data (fetched through ElementID in primitiveParam)
        //
        HdBufferArrayRangeSharedPtr const & elementBar =
            drawItem->GetElementPrimVarRange();
        if (elementBar and (not elementBar->IsAggregatedWith(elementBarCurrent))) {
            binder.UnbindBufferArray(elementBarCurrent);
            binder.BindBufferArray(elementBar);
            elementBarCurrent = elementBar;
        }

        //
        // vertex attrib buffer data
        //
        HdBufferArrayRangeSharedPtr const & vertexBar =
            drawItem->GetVertexPrimVarRange();

        if (vertexBar and (not vertexBar->IsAggregatedWith(vertexBarCurrent))) {
            binder.UnbindBufferArray(vertexBarCurrent);
            binder.BindBufferArray(vertexBar);
            vertexBarCurrent = vertexBar;
        }

        //
        // constant (uniform) buffer data
        //
        HdBufferArrayRangeSharedPtr const & constantBar =
            drawItem->GetConstantPrimVarRange();
        if (constantBar and (not constantBar->IsAggregatedWith(constantBarCurrent))) {
            binder.UnbindConstantBuffer(constantBarCurrent);
            binder.BindConstantBuffer(constantBar);
            constantBarCurrent = constantBar;
        }

        //
        // facevarying buffer data
        //
        HdBufferArrayRangeSharedPtr const & fvarBar =
            drawItem->GetFaceVaryingPrimVarRange();
        if (fvarBar and (not fvarBar->IsAggregatedWith(fvarBarCurrent))) {
            binder.UnbindBufferArray(fvarBarCurrent);
            binder.BindBufferArray(fvarBar);
            fvarBarCurrent = fvarBar;
        }

        //
        // instance buffer data
        //
        int instancerNumLevels = drawItem->GetInstancePrimVarNumLevels();
        int instanceIndexWidth = instancerNumLevels + 1;
        for (int i = 0; i < instancerNumLevels; ++i) {
            HdBufferArrayRangeSharedPtr const & instanceBar =
                drawItem->GetInstancePrimVarRange(i);
            if (instanceBar) {
                if (static_cast<size_t>(i) >= instanceBarCurrents.size()) {
                    instanceBarCurrents.push_back(instanceBar);
                    binder.BindInstanceBufferArray(instanceBar, i);
                } else if (not instanceBar->IsAggregatedWith(
                               instanceBarCurrents[i])) {
                    binder.UnbindInstanceBufferArray(instanceBarCurrents[i], i);
                    binder.BindInstanceBufferArray(instanceBar, i);
                    instanceBarCurrents[i] = instanceBar;
                }
            }
        }

        //
        // instance index indirection buffer
        //
        HdBufferArrayRangeSharedPtr const & instanceIndexBar =
            drawItem->GetInstanceIndexRange();
        if (instanceIndexBar and (not instanceIndexBar->IsAggregatedWith(instanceIndexBarCurrent))) {
            binder.UnbindBufferArray(instanceIndexBarCurrent);
            binder.BindBufferArray(instanceIndexBar);
            instanceIndexBarCurrent = instanceIndexBar;
        }

        //
        // shader buffer
        //
        HdBufferArrayRangeSharedPtr const & shaderBar =
            renderPassState->GetOverrideShader()
                ? HdBufferArrayRangeSharedPtr()
                : drawItem->GetSurfaceShader()->GetShaderData();
        // shaderBar isn't needed when the surfaceShader is overriden
        if (shaderBar and (not shaderBar->IsAggregatedWith(shaderBarCurrent))) {
            if (shaderBarCurrent) {
                binder.UnbindBuffer(HdTokens->surfaceShaderParams,
                                    shaderBarCurrent->GetResource());
            }
            binder.BindBuffer(HdTokens->surfaceShaderParams,
                              shaderBar->GetResource());
            shaderBarCurrent = shaderBar;
        }

        //
        // shader textures
        //
        if (not hasOverrideShader) {
            drawItem->GetSurfaceShader()->BindResources(binder, programId);
        }

        /*
          Drawing coord is a unified cursor which locates a subset of
          aggregated buffer in GPU. The primary role of drawing coord is
          to provide a way to access buffers from glsl shader code.

          We have some aggregated buffers of different granularities.
          They are associated to class/variability specifiers in GL/prman spec.
          ( see http://renderman.pixar.com/view/Appnote22 )

          |   | drawing coord |  hd buffer   |     OpenGL     |     PRMan      |
          ----------------------------------------------------------------------
          | 0 | ModelDC       |  (reserved)  |    uniform     |    constant    |
          | 1 | ConstantDC    |  constantBar |    uniform     |    constant    |
          | 2 | ElementDC     |  elementBar  |       (*)      |    uniform     |
          | 3 | PrimitiveDC   |  indexBar    | gl_PrimitiveID |       (*)      |
          | 4 | FVarDC        |  fvarBar     | gl_PrimitiveID |    facevarying |
          | 5 | InstanceIndex |  inst-idxBar | (gl_InstanceID)|      n/a       |
          | 7 | ShaderDC      |  shaderBar   |    uniform     |                |
          | 8 | InstanceDC[0] |  instanceBar | (gl_InstanceID)|    constant    |
          | 9 | InstanceDC[1] |  instanceBar | (gl_InstanceID)|    constant    |
          |...| ...           |  instanceBar | (gl_InstanceID)|    constant    |
          ----------------------------------------------------------------------
          | - | VertexBase    |  vertexBar   |  gl_VertexID   | vertex/varying |

          We put these offsets into 3 variables,
           - ivec4 drawingCoord0  (ModelDC - PrimitiveDC)
           - ivec3 drawingCoord1  (FVarDC - ShaderDC)
           - int[] drawingCoordI  (InstanceDC)
          so that the shaders can access any of these aggregated data.

          (*) primitiveParam buffer can be used to reinterpret GL-primitive
              ID back to element ID.

         */

        int vertexOffset = 0;
        int vertexCount = 0;
        if (vertexBar) {
            vertexOffset = vertexBar->GetOffset();
            vertexCount = vertexBar->GetNumElements();
        }

        //
        // Get parameters from our buffer range objects to
        // allow drawing to access the correct elements from
        // aggregated buffers.
        //
        int numIndicesPerPrimitive = geometricShader->GetPrimitiveIndexSize();
        int indexCount = indexBar ? indexBar->GetNumElements() * numIndicesPerPrimitive : 0;
        int firstIndex = indexBar ? indexBar->GetOffset() * numIndicesPerPrimitive : 0;
        int baseVertex = vertexOffset;
        int instanceCount = instanceIndexBar
            ? instanceIndexBar->GetNumElements()/instanceIndexWidth : 1;

        // if delegate fails to get vertex primvars, it could be empty.
        // skip the drawitem to prevent drawing uninitialized vertices.
        if (vertexCount == 0) continue;

        // update standalone uniforms
        int drawingCoord0[4] = {
            0, // reserved for modelBar
            constantBar ? constantBar->GetIndex()  : 0,
            elementBar  ? elementBar->GetOffset()  : 0,
            indexBar    ? indexBar->GetOffset()    : 0
        };
        int drawingCoord1[3] = {
            fvarBar          ? fvarBar->GetOffset()          : 0,
            instanceIndexBar ? instanceIndexBar->GetOffset() : 0,
            shaderBar        ? shaderBar->GetIndex()         : 0
        };
        binder.BindUniformi(HdTokens->drawingCoord0, 4, drawingCoord0);
        binder.BindUniformi(HdTokens->drawingCoord1, 3, drawingCoord1);

        // instance coordinates
        std::vector<int> instanceDrawingCoords(instancerNumLevels);
        for (int i = 0; i < instancerNumLevels; ++i) {
            instanceDrawingCoords[i] = instanceBarCurrents[i]
                ? instanceBarCurrents[i]->GetOffset() : 0;
        }
        if (instancerNumLevels > 0) {
            binder.BindUniformArrayi(HdTokens->drawingCoordI,
                                     instancerNumLevels, &instanceDrawingCoords[0]);
        }

        if (indexCount > 0 and indexBar) {
            glDrawElementsInstancedBaseVertex(
                geometricShader->GetPrimitiveMode(),
                indexCount,
                GL_UNSIGNED_INT, // GL_INT is invalid: indexBar->GetResource(HdTokens->indices)->GetGLDataType(),
                (void *)(firstIndex * sizeof(GLuint)),
                instanceCount,
                baseVertex);
        } else if (vertexCount > 0) {
            glDrawArraysInstanced(
                geometricShader->GetPrimitiveMode(),
                baseVertex,
                vertexCount,
                instanceCount);
        }

        if (not hasOverrideShader) {
            drawItem->GetSurfaceShader()->UnbindResources(binder, programId);
        }

        HD_PERF_COUNTER_INCR(HdPerfTokens->drawCalls);
    }

    HD_PERF_COUNTER_ADD(HdTokens->itemsDrawn, numItemsDrawn);

    TF_FOR_ALL(it, shaders) {
        (*it)->UnbindResources(binder, programId);
    }
    geometricShader->UnbindResources(binder, programId);

    // unbind (make non resident all bindless buffers)
    if (constantBarCurrent)
        binder.UnbindConstantBuffer(constantBarCurrent);
    if (vertexBarCurrent)
        binder.UnbindBufferArray(vertexBarCurrent);
    if (elementBarCurrent)
        binder.UnbindBufferArray(elementBarCurrent);
    if (fvarBarCurrent)
        binder.UnbindBufferArray(fvarBarCurrent);
    for (size_t i = 0; i < instanceBarCurrents.size(); ++i) {
        binder.UnbindInstanceBufferArray(instanceBarCurrents[i], (int)i);
    }
    if (instanceIndexBarCurrent)
        binder.UnbindBufferArray(instanceIndexBarCurrent);
    if (indexBarCurrent)
        binder.UnbindBufferArray(indexBarCurrent);
    if (shaderBarCurrent) {
        binder.UnbindBuffer(HdTokens->surfaceShaderParams,
                            shaderBarCurrent->GetResource());
    }

    glUseProgram(0);
}
