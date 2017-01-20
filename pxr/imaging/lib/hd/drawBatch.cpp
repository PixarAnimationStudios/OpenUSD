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
#include "pxr/imaging/hd/glslfxShader.h"
#include "pxr/imaging/hd/glslProgram.h"
#include "pxr/imaging/hd/lightingShader.h"
#include "pxr/imaging/hd/package.h"
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
        if (!rangeB) {
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
    if (!TF_VERIFY(!_drawItemInstances.empty())) {
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
    if (!HdSurfaceShader::CanAggregate(drawItem0->GetSurfaceShader(),
                                          drawItem1->GetSurfaceShader())) {
        return false;
    }

    if (drawItem0->GetGeometricShader() == drawItem1->GetGeometricShader()
        && drawItem0->GetInstancePrimVarNumLevels() ==
            drawItem1->GetInstancePrimVarNumLevels()
        && isAggregated(drawItem0->GetTopologyRange(),
                         drawItem1->GetTopologyRange())
        && isAggregated(drawItem0->GetVertexPrimVarRange(),
                         drawItem1->GetVertexPrimVarRange())
        && isAggregated(drawItem0->GetElementPrimVarRange(),
                         drawItem1->GetElementPrimVarRange())
        && isAggregated(drawItem0->GetConstantPrimVarRange(),
                         drawItem1->GetConstantPrimVarRange())
        && isAggregated(drawItem0->GetInstanceIndexRange(),
                         drawItem1->GetInstanceIndexRange())) {
        int numLevels = drawItem0->GetInstancePrimVarNumLevels();
        for (int i = 0; i < numLevels; ++i) {
            if (!isAggregated(drawItem0->GetInstancePrimVarRange(i),
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
        if (TF_VERIFY(!_drawItemInstances.empty())) {
            if (!Append(const_cast<HdDrawItemInstance*>(instances[i]))) {
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
    HF_MALLOC_TAG_FUNCTION();

    HdDrawItem const *firstDrawItem = _drawItemInstances[0]->GetDrawItem();

    // Calculate unique hash to detect if the shader (composed) has changed
    // recently and we need to recompile it.
    size_t shaderHash = state->GetShaderHash();
    boost::hash_combine(shaderHash,
                        firstDrawItem->GetGeometricShader()->ComputeHash());
    HdShaderSharedPtr overrideShader = state->GetOverrideShader();
    HdShaderSharedPtr surfaceShader  = overrideShader ? overrideShader
                                       : firstDrawItem->GetSurfaceShader();
    boost::hash_combine(shaderHash, surfaceShader->ComputeHash());
    bool shaderChanged = (_shaderHash != shaderHash);
    
    // Set shaders (lighting and renderpass) to the program. 
    // We need to do this before checking if the shaderChanged because 
    // it is possible that the shader does not need to 
    // be recompiled but some of the parameters have changed.
    HdShaderSharedPtrVector shaders = state->GetShaders();
    _program.SetShaders(shaders);
    _program.SetGeometricShader(firstDrawItem->GetGeometricShader());

    // XXX: if this function appears to be expensive, we might consider caching
    //      programs by shaderHash.
    if (!_program.GetGLSLProgram() || shaderChanged) {
        
        _program.SetSurfaceShader(surfaceShader);

        // Try to compile the shader and if it fails to compile we go back
        // to use the specified fallback surface shader.
        if (!_program.CompileShader(firstDrawItem, indirect)) {

            // If we failed to compile the surface shader, replace it with the
            // fallback surface shader and try again.
            // XXX: Note that we only say "surface shader" here because it is
            // currently the only one that we allow customization for.  We
            // expect all the other shaders to compile or else the shipping
            // code is broken and needs to be fixed.  When we open up more
            // shaders for customization, we will need to check them as well.
            
            typedef boost::shared_ptr<class GlfGLSLFX> GlfGLSLFXSharedPtr;
            typedef boost::shared_ptr<class HdSurfaceShader> 
                HdSurfaceShaderSharedPtr;

            GlfGLSLFXSharedPtr glslSurfaceFallback = 
                GlfGLSLFXSharedPtr(
                        new GlfGLSLFX(HdPackageFallbackSurfaceShader()));

            HdSurfaceShaderSharedPtr fallbackSurface = 
                HdSurfaceShaderSharedPtr(
                    new HdGLSLFXShader(glslSurfaceFallback));

            _program.SetSurfaceShader(fallbackSurface);

            bool res = _program.CompileShader(firstDrawItem, indirect);
            // We expect the fallback shader to always compile.
            TF_VERIFY(res);
        }

        _shaderHash = shaderHash;
    }

    return _program;
}

bool
Hd_DrawBatch::_DrawingProgram::CompileShader(
        HdDrawItem const *drawItem,
        bool indirect)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // glew has to be intialized
    if (!glLinkProgram) {
        return false;
    }

    if (!_geometricShader) {
        TF_CODING_ERROR("Can not compile a shader without a geometric shader");
        return false;
    }

    // determine binding points and populate metaData
    HdBindingRequestVector customBindings;
    bool instanceDraw = true;
    _GetCustomBindings(&customBindings, &instanceDraw);

    // also (surface, renderPass) shaders use their bindings
    HdShaderSharedPtrVector shaders = GetComposedShaders();

    TF_FOR_ALL(it, shaders) {
        (*it)->AddBindings(&customBindings);
    }

    Hd_CodeGen codeGen(_geometricShader, shaders);

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
        } else {
            // Failed to compile and link a valid glsl program.
            return false;
        }
    }
    return true;
}

/* virtual */
void
Hd_DrawBatch::_DrawingProgram::_GetCustomBindings(
    HdBindingRequestVector *customBindings,
    bool *enableInstanceDraw) const
{
    if (!TF_VERIFY(enableInstanceDraw)) return;

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
    if (!TF_VERIFY(glslProgram)) return false;

    return glslProgram->Link();
}
