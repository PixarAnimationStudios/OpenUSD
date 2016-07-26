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
#include "pxr/imaging/hd/drawBatch.h"
#include "pxr/imaging/hd/codeGen.h"
#include "pxr/imaging/hd/commandBuffer.h"
#include "pxr/imaging/hd/geometricShader.h"
#include "pxr/imaging/hd/glslProgram.h"
#include "pxr/imaging/hd/lightingShader.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/renderPassShader.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/shader.h"
#include "pxr/imaging/hd/surfaceShader.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/glf/glslfx.h"

#include "pxr/base/tf/getenv.h"

Hd_DrawBatch::Hd_DrawBatch(HdDrawItemInstance * drawItemInstance)
    : _shaderHash(0)
{
}

/*virtual*/
void
Hd_DrawBatch::_Init(HdDrawItemInstance * drawItemInstance)
{
    _drawItemInstances.push_back(drawItemInstance);

    // Force shader to refresh.
    // XXX: Why is this necessary? If the draw item state changes in a
    // significant way such that the shader needs to be recompiled, that value
    // should be part of the shader hash and this shouldn't be required.
    _shaderHash = 0;
}

Hd_DrawBatch::~Hd_DrawBatch()
{
}

void
Hd_DrawBatch::DrawItemInstanceChanged(HdDrawItemInstance const* /*instance*/)
{
}

namespace {
inline bool isAggregated(HdBufferArrayRangeSharedPtr const &rangeA,
                         HdBufferArrayRangeSharedPtr const &rangeB)
{
    if (rangeA) {
        return rangeA->IsAggregatedWith(rangeB);
    } else {
        if (not rangeB) {
            // can batch together if both ranges are empty.
            return true;
        }
    }
    return false;
}
}

bool
Hd_DrawBatch::Append(HdDrawItemInstance * drawItemInstance)
{
    if (not TF_VERIFY(not _drawItemInstances.empty())) {
        return false;
    }

    // XXX: we'll soon refactor this function out and centralize batch
    // bucketing and reordering logic in HdCommandBuffer.

    HdDrawItem const* drawItem = drawItemInstance->GetDrawItem();

    HdDrawItem const* batchItem = _drawItemInstances.front()->GetDrawItem();
    TF_VERIFY(batchItem);

    if (_IsAggregated(drawItem, batchItem)) {
        drawItemInstance->SetBatchIndex(_drawItemInstances.size());
        drawItemInstance->SetBatch(this);
        _drawItemInstances.push_back(drawItemInstance);
        return true;
    } else {
        return false;
    }
}

/*static*/
bool
Hd_DrawBatch::_IsAggregated(HdDrawItem const *drawItem0,
                            HdDrawItem const *drawItem1)
{
    if (not HdSurfaceShader::CanAggregate(drawItem0->GetSurfaceShader(),
                                          drawItem1->GetSurfaceShader())) {
        return false;
    }

    if (drawItem0->GetGeometricShader() == drawItem1->GetGeometricShader()
        and drawItem0->GetInstancePrimVarNumLevels() ==
            drawItem1->GetInstancePrimVarNumLevels()
        and isAggregated(drawItem0->GetTopologyRange(),
                         drawItem1->GetTopologyRange())
        and isAggregated(drawItem0->GetVertexPrimVarRange(),
                         drawItem1->GetVertexPrimVarRange())
        and isAggregated(drawItem0->GetElementPrimVarRange(),
                         drawItem1->GetElementPrimVarRange())
        and isAggregated(drawItem0->GetConstantPrimVarRange(),
                         drawItem1->GetConstantPrimVarRange())
        and isAggregated(drawItem0->GetInstanceIndexRange(),
                         drawItem1->GetInstanceIndexRange())) {
        int numLevels = drawItem0->GetInstancePrimVarNumLevels();
        for (int i = 0; i < numLevels; ++i) {
            if (not isAggregated(drawItem0->GetInstancePrimVarRange(i),
                                 drawItem1->GetInstancePrimVarRange(i))) {
                return false;
            }
        }
        return true;
    }

    return false;
}

bool
Hd_DrawBatch::Rebuild()
{
    std::vector<HdDrawItemInstance const*> instances;
    instances.swap(_drawItemInstances);
    _drawItemInstances.reserve(instances.size());

    // Ensure all batch state initialized from items/instances is refreshed.
    _Init(const_cast<HdDrawItemInstance*>(instances.front()));

    // Start this loop at i=1 because the 0th element was pushed via _Init
    for (size_t i = 1; i < instances.size(); ++i) {
        if (TF_VERIFY(not _drawItemInstances.empty())) {
            if (not Append(const_cast<HdDrawItemInstance*>(instances[i]))) {
                return false;
            }
        } else {
            return false;
        }
    }

    return true;
}

Hd_DrawBatch::_DrawingProgram &
Hd_DrawBatch::_GetDrawingProgram(HdRenderPassStateSharedPtr const &state,
                                 bool indirect)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    HdDrawItem const *firstDrawItem = _drawItemInstances[0]->GetDrawItem();

    size_t shaderHash = state->GetShaderHash(); // overrideShader is taken into account
    boost::hash_combine(shaderHash,
                        firstDrawItem->GetGeometricShader()->ComputeHash());
    bool shaderChanged = (_shaderHash != shaderHash);

    // XXX: if this function appears to be expensive, we might consider caching
    //      programs by shaderHash.
    if (not _program.GetGLSLProgram() or shaderChanged) {

        // compose shaders
        HdShaderSharedPtrVector shaders(3);
        shaders[0] = state->GetLightingShader();
        shaders[1] = state->GetRenderPassShader();
        HdShaderSharedPtr overrideShader = state->GetOverrideShader();
        shaders[2] = overrideShader ? overrideShader
                                    : firstDrawItem->GetSurfaceShader();

        _program.CompileShader(firstDrawItem,
                               firstDrawItem->GetGeometricShader(),
                               shaders, indirect);

        _shaderHash = shaderHash;
    }

    return _program;
}

void
Hd_DrawBatch::_DrawingProgram::CompileShader(
        HdDrawItem const *drawItem,
        Hd_GeometricShaderPtr const &geometricShader,
        HdShaderSharedPtrVector const &shaders,
        bool indirect)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    // glew has to be intialized
    if (not glLinkProgram)
        return;

    // determine binding points and populate metaData
    HdBindingRequestVector customBindings;
    bool instanceDraw = true;
    _GetCustomBindings(&customBindings, &instanceDraw);

    // also (surface, renderPass) shaders use their bindings
    TF_FOR_ALL(it, shaders) {
        (*it)->AddBindings(&customBindings);
    }

    Hd_CodeGen codeGen(geometricShader, shaders);

    // let resourcebinder resolve bindings and populate metadata
    // which is owned by codegen.
    _resourceBinder.ResolveBindings(drawItem,
                                    shaders,
                                    codeGen.GetMetaData(),
                                    indirect,
                                    instanceDraw,
                                    customBindings);

    HdGLSLProgram::ID hash = codeGen.ComputeHash();

    HdResourceRegistry *resourceRegistry = &HdResourceRegistry::GetInstance();

    {
        HdInstance<HdGLSLProgram::ID, HdGLSLProgramSharedPtr> programInstance;

        // ask registry to see if there's already compiled program
        std::unique_lock<std::mutex> regLock = 
            resourceRegistry->RegisterGLSLProgram(hash, &programInstance);

        if (programInstance.IsFirstInstance()) {
            HdGLSLProgramSharedPtr glslProgram = codeGen.Compile();
            if (_Link(glslProgram)) {
                // store the program into the program registry.
                programInstance.SetValue(glslProgram);
            }
        }

        _glslProgram = programInstance.GetValue();

        if (_glslProgram) {
            _resourceBinder.IntrospectBindings(_glslProgram->GetProgram().GetId());
        }
    }
}

/* virtual */
void
Hd_DrawBatch::_DrawingProgram::_GetCustomBindings(
    HdBindingRequestVector *customBindings,
    bool *enableInstanceDraw) const
{
    if (not TF_VERIFY(enableInstanceDraw)) return;

    // set enableInstanceDraw true by default, which means the shader is
    // expected to be invoked by instanced-draw call.
    // XFB culling is an exception, which uses glDrawArrays.
    *enableInstanceDraw = true;
}

/* virtual */
bool
Hd_DrawBatch::_DrawingProgram::_Link(
        HdGLSLProgramSharedPtr const & glslProgram)
{
    if (not TF_VERIFY(glslProgram)) return false;

    return glslProgram->Link();
}
