//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/binding.h"
#include "pxr/imaging/hdSt/codeGen.h"
#include "pxr/imaging/hdSt/commandBuffer.h"
#include "pxr/imaging/hdSt/debugCodes.h"
#include "pxr/imaging/hdSt/drawBatch.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/glslfxShader.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/lightingShader.h"
#include "pxr/imaging/hdSt/materialNetworkShader.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/hash.h"

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_ENV_SETTING(HDST_ENABLE_BROKEN_SHADER_VISUAL_FEEDBACK, false,
    "Provide visual feedback for prims when the composed shader fails to "
    "compile or link by using the invalid material shader.");

namespace
{

bool
_ProvideVisualFeedbackForBrokenShaders()
{
    static const bool enabled =
        TfGetEnvSetting(HDST_ENABLE_BROKEN_SHADER_VISUAL_FEEDBACK);
    return enabled;
}

const std::string&
_GetPrimPathSubstringForDebugLogging()
{
    // To aid debugging of shader programs and caching behavior in Storm,
    // use the env var HDST_DEBUG_SHADER_PROGRAM_FOR_PRIM to provide a prim path
    // substring to limit logging of drawing (i.e. non-compute) shader program 
    // caching behavior to just those draw batches with draw items for prims
    // matching the substring.
    //
    static const std::string substring =
        TfGetenv("HDST_DEBUG_SHADER_PROGRAM_FOR_PRIM");
    return substring;
}

bool
_LogShaderCacheLookupForDrawBatch(
    std::vector<HdStDrawItemInstance const*> const &_drawItemInstances)
{
    const std::string &substring = _GetPrimPathSubstringForDebugLogging();
    if (substring.empty()) {
        return true; // log all batches.
    }

    for (auto const &drawItemInstance : _drawItemInstances) {
        HdStDrawItem const *drawItem = drawItemInstance->GetDrawItem();
        if (TF_VERIFY(drawItem)) {
            if (drawItem->GetRprimID().GetString().find(substring)
                    != std::string::npos) {
                
                return true;
            }
        }
    }

    return false;
}

bool
_LogShaderCacheLookup()
{
    return TfDebug::IsEnabled(HDST_LOG_DRAWING_SHADER_PROGRAM_MISSES) ||
           TfDebug::IsEnabled(HDST_LOG_DRAWING_SHADER_PROGRAM_HITS);
}

}


HdSt_DrawBatch::HdSt_DrawBatch(
    HdStDrawItemInstance * drawItemInstance,
    bool const allowTextureResourceRebinding)
    : _allowTextureResourceRebinding(allowTextureResourceRebinding)
    , _shaderHash(0)
{
}

/*virtual*/
void
HdSt_DrawBatch::_Init(HdStDrawItemInstance * drawItemInstance)
{
    _drawItemInstances.push_back(drawItemInstance);

    // Force shader to refresh.
    // XXX: Why is this necessary? If the draw item state changes in a
    // significant way such that the shader needs to be recompiled, that value
    // should be part of the shader hash and this shouldn't be required.
    _shaderHash = 0;
}

HdSt_DrawBatch::~HdSt_DrawBatch()
{
}

void
HdSt_DrawBatch::DrawItemInstanceChanged(HdStDrawItemInstance const* /*instance*/)
{
}

void
HdSt_DrawBatch::SetEnableTinyPrimCulling(bool tinyPrimCulling)
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
HdSt_DrawBatch::Append(HdStDrawItemInstance * drawItemInstance)
{
    if (!TF_VERIFY(!_drawItemInstances.empty())) {
        return false;
    }

    // XXX: we'll soon refactor this function out and centralize batch
    // bucketing and reordering logic in HdStCommandBuffer.

    HdStDrawItem const* drawItem = static_cast<const HdStDrawItem*>(
        drawItemInstance->GetDrawItem());
    HdStDrawItem const* batchItem = static_cast<const HdStDrawItem*>(
        _drawItemInstances.front()->GetDrawItem());
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
HdSt_DrawBatch::_CanAggregateMaterials(HdStDrawItem const *drawItem0,
                                       HdStDrawItem const *drawItem1)
{
    if (drawItem0->GetMaterialIsFinal() !=
        drawItem1->GetMaterialIsFinal()) {
        return false;
    }

    HdStShaderCodeSharedPtr const &
        shaderA = drawItem0->GetMaterialNetworkShader();
    HdStShaderCodeSharedPtr const &
        shaderB = drawItem1->GetMaterialNetworkShader();

    // Can aggregate if the shaders are identical.
    if (shaderA == shaderB) {
        return true;
    }

    HdBufferArrayRangeSharedPtr dataA = shaderA->GetShaderData();
    HdBufferArrayRangeSharedPtr dataB = shaderB->GetShaderData();

    bool dataIsAggregated = (dataA == dataB) ||
                            (dataA && dataA->IsAggregatedWith(dataB));

    // We can't aggregate if the shaders have data buffers that aren't
    // aggregated or if the shaders don't match.
    if (!dataIsAggregated || shaderA->ComputeHash() != shaderB->ComputeHash()) {
        return false;
    }

    return true;
}

bool
HdSt_DrawBatch::_CanAggregateTextures(HdStDrawItem const *drawItem0,
                                      HdStDrawItem const *drawItem1)
{
    return _allowTextureResourceRebinding ||
           (drawItem0->GetMaterialNetworkShader()->ComputeTextureSourceHash() ==
            drawItem1->GetMaterialNetworkShader()->ComputeTextureSourceHash());
}

bool
HdSt_DrawBatch::_IsAggregated(HdStDrawItem const *drawItem0,
                              HdStDrawItem const *drawItem1)
{
    if (!_CanAggregateMaterials(drawItem0, drawItem1)) {
        return false;
    }

    if (!_CanAggregateTextures(drawItem0, drawItem1)) {
        return false;
    }

    if (drawItem0->GetGeometricShader() == drawItem1->GetGeometricShader()
        && drawItem0->GetInstancePrimvarNumLevels() ==
            drawItem1->GetInstancePrimvarNumLevels()
        && isAggregated(drawItem0->GetTopologyRange(),
                         drawItem1->GetTopologyRange())
        && isAggregated(drawItem0->GetTopologyVisibilityRange(),
                         drawItem1->GetTopologyVisibilityRange())
        && isAggregated(drawItem0->GetVertexPrimvarRange(),
                         drawItem1->GetVertexPrimvarRange())
        && isAggregated(drawItem0->GetVaryingPrimvarRange(),
                         drawItem1->GetVaryingPrimvarRange())
        && isAggregated(drawItem0->GetElementPrimvarRange(),
                         drawItem1->GetElementPrimvarRange())
        && isAggregated(drawItem0->GetFaceVaryingPrimvarRange(),
                         drawItem1->GetFaceVaryingPrimvarRange())
        && isAggregated(drawItem0->GetConstantPrimvarRange(),
                         drawItem1->GetConstantPrimvarRange())
        && isAggregated(drawItem0->GetInstanceIndexRange(),
                         drawItem1->GetInstanceIndexRange())) {
        int numLevels = drawItem0->GetInstancePrimvarNumLevels();
        for (int i = 0; i < numLevels; ++i) {
            if (!isAggregated(drawItem0->GetInstancePrimvarRange(i),
                                 drawItem1->GetInstancePrimvarRange(i))) {
                return false;
            }
        }
        return true;
    }

    return false;
}

bool
HdSt_DrawBatch::Rebuild()
{
    std::vector<HdStDrawItemInstance const*> instances;
    instances.swap(_drawItemInstances);
    _drawItemInstances.reserve(instances.size());

    // Ensure all batch state initialized from items/instances is refreshed.
    HdStDrawItemInstance *batchItem =
        const_cast<HdStDrawItemInstance*>(instances.front());
    if (!TF_VERIFY(batchItem->GetDrawItem()->GetGeometricShader())) {
        return false;
    }
    _Init(batchItem);
    if (!TF_VERIFY(!_drawItemInstances.empty())) {
        return false;
    }

    // Start this loop at i=1 because the 0th element was pushed via _Init
    for (size_t i = 1; i < instances.size(); ++i) {
        HdStDrawItemInstance *item =
            const_cast<HdStDrawItemInstance*>(instances[i]);
        if (!TF_VERIFY(item->GetDrawItem()->GetGeometricShader())) {
            return false;
        }
        if (!Append(item)) {
            TF_DEBUG(HDST_DRAW_BATCH).Msg("   Rebuild failed for batch %p\n",
            (void*)(this));
            return false;
        }
    }

    TF_DEBUG(HDST_DRAW_BATCH).Msg("   Rebuild success for batch %p\n",
        (void*)(this));

    return true;
}

static
HdSt_MaterialNetworkShaderSharedPtr
_GetFallbackMaterialNetworkShader()
{
    static std::once_flag once;
    static HdSt_MaterialNetworkShaderSharedPtr fallbackShader;
   
    std::call_once(once, [](){
        HioGlslfxSharedPtr glslfx =
            std::make_shared<HioGlslfx>(
                HdStPackageFallbackMaterialNetworkShader());

        fallbackShader.reset(new HdStGLSLFXShader(glslfx));
    });

    return fallbackShader;
}

static
HdSt_MaterialNetworkShaderSharedPtr
_GetInvalidMaterialNetworkShader()
{
    static std::once_flag once;
    static HdSt_MaterialNetworkShaderSharedPtr invalidShader;
   
    std::call_once(once, [](){
        HioGlslfxSharedPtr glslfx =
            std::make_shared<HioGlslfx>(
                HdStPackageInvalidMaterialNetworkShader());

        invalidShader.reset(new HdStGLSLFXShader(glslfx));
    });

    return invalidShader;
}

HdSt_DrawBatch::_DrawingProgram &
HdSt_DrawBatch::_GetDrawingProgram(HdStRenderPassStateSharedPtr const &state,
                                 HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdStDrawItem const *firstDrawItem = _drawItemInstances[0]->GetDrawItem();

    // Calculate unique hash to detect if the shader (composed) has changed
    // recently and we need to recompile it.
    size_t shaderHash = TfHash::Combine(state->GetShaderHash(),
                        firstDrawItem->GetGeometricShader()->ComputeHash());

    HdSt_MaterialNetworkShaderSharedPtr materialNetworkShader =
        firstDrawItem->GetMaterialNetworkShader();
    
    if (!state->GetUseSceneMaterials() &&
        !firstDrawItem->GetMaterialIsFinal() ) {
        materialNetworkShader = _GetFallbackMaterialNetworkShader();
    }

    size_t materialNetworkShaderHash =
        materialNetworkShader ? materialNetworkShader->ComputeHash() : 0;
    shaderHash = TfHash::Combine(shaderHash, materialNetworkShaderHash);

    bool shaderChanged = (_shaderHash != shaderHash);
    
    // Set shaders (lighting and renderpass) to the program. 
    // We need to do this before checking if the shaderChanged because 
    // it is possible that the shader does not need to 
    // be recompiled but some of the parameters have changed.
    HdStShaderCodeSharedPtrVector shaders = state->GetShaders();
    _program.SetShaders(shaders);
    _program.SetGeometricShader(firstDrawItem->GetGeometricShader());

    // XXX: if this function appears to be expensive, we might consider caching
    //      programs by shaderHash.
    if (!_program.GetGLSLProgram() || shaderChanged) {
        
        _program.SetMaterialNetworkShader(materialNetworkShader);

        const bool logCacheLookup =
            _LogShaderCacheLookup() &&
            _LogShaderCacheLookupForDrawBatch(_drawItemInstances);

        // Try to compile the shader and if it fails to compile we go back
        // to use the specified fallback material network shader.
        if (!_program.CompileShader(
                firstDrawItem, resourceRegistry, logCacheLookup)){

            // While the code should gracefully handle shader compilation
            // failures, it is also undesirable for shaders to silently fail.
            TF_CODING_ERROR("Failed to compile shader for prim %s.",
                            firstDrawItem->GetRprimID().GetText());

            // If we failed to compile the material network, replace it
            // either with the invalid material network shader OR the
            // fallback material network shader and try again.
            // XXX: Note that we only say "material network shader" here
            // because it is currently the only one for which we allow
            // customization.  We expect all the other shaders to compile
            // or else the shipping code is broken and needs to be fixed.
            // When we open up more shaders for customization, we will
            // need to check them as well.

            const HdSt_MaterialNetworkShaderSharedPtr shader =
                _ProvideVisualFeedbackForBrokenShaders()
                ? _GetInvalidMaterialNetworkShader()
                : _GetFallbackMaterialNetworkShader();
                
            _program.SetMaterialNetworkShader(shader);

            bool res = _program.CompileShader(firstDrawItem, 
                                              resourceRegistry,
                                              logCacheLookup);

            // We expect the invalid/fallback shader to always compile.
            TF_VERIFY(res, "Failed to compile with the invalid/fallback "
                           "material network shader.");
        }

        _shaderHash = shaderHash;
    }

    return _program;
}

bool
HdSt_DrawBatch::_DrawingProgram::IsValid() const
{
    return _glslProgram && _glslProgram->Validate();
}

bool
HdSt_DrawBatch::_DrawingProgram::CompileShader(
        HdStDrawItem const *drawItem,
        HdStResourceRegistrySharedPtr const &resourceRegistry,
        bool logCacheLookup /* = false */)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!_geometricShader) {
        TF_CODING_ERROR("Can not compile a shader without a geometric shader");
        return false;
    }

    // determine binding points and populate metaData
    HdStBindingRequestVector customBindings;
    bool instanceDraw = true;
    _GetCustomBindings(&customBindings, &instanceDraw);

    // also (surface, renderPass) shaders use their bindings
    HdStShaderCodeSharedPtrVector shaders = GetComposedShaders();

    TF_FOR_ALL(it, shaders) {
        (*it)->AddBindings(&customBindings);
    }

    std::unique_ptr<HdSt_ResourceBinder::MetaData> metaData =
        std::make_unique<HdSt_ResourceBinder::MetaData>();
    
    // let resourcebinder resolve bindings and populate metadata
    // which is owned by codegen.
    _resourceBinder.ResolveBindings(drawItem,
                                    shaders,
                                    metaData.get(),
                                    _drawingCoordBufferBinding,
                                    instanceDraw,
                                    customBindings,
                                    resourceRegistry->GetHgi()->
                                        GetCapabilities());

    HdSt_CodeGen codeGen(_geometricShader, shaders,
                         drawItem->GetMaterialTag(), std::move(metaData));

    HdStGLSLProgram::ID hash = codeGen.ComputeHash();

    {
        // ask registry to see if there's already compiled program
        HdInstance<HdStGLSLProgramSharedPtr> programInstance =
                                resourceRegistry->RegisterGLSLProgram(hash);

        if (programInstance.IsFirstInstance()) {

            if (TfDebug::IsEnabled(HDST_LOG_DRAWING_SHADER_PROGRAM_MISSES) &&
                logCacheLookup) {

                TfDebug::Helper().Msg(
                    "(MISS) First program instance for batch with head draw "
                    "item %s (hash = %zu)\n",
                    drawItem->GetRprimID().GetText(), hash);
            }

            HdStGLSLProgramSharedPtr glslProgram = codeGen.Compile(
                resourceRegistry.get());
            if (glslProgram && _Link(glslProgram)) {
                // store the program into the program registry.
                programInstance.SetValue(glslProgram);
            }
        } else {
            if (TfDebug::IsEnabled(HDST_LOG_DRAWING_SHADER_PROGRAM_HITS) &&
                logCacheLookup) {
            
                TfDebug::Helper().Msg(
                    "(HIT) Found program instance with hash = %zu for batch "
                    "with head draw item %s\n",
                    hash, drawItem->GetRprimID().GetText());
            }
        }

        _glslProgram = programInstance.GetValue();

        if (!_glslProgram) {
            // Failed to compile and link a valid glsl program.
            return false;
        }
    }
    return true;
}

/* virtual */
void
HdSt_DrawBatch::_DrawingProgram::_GetCustomBindings(
    HdStBindingRequestVector *customBindings,
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
HdSt_DrawBatch::_DrawingProgram::_Link(
        HdStGLSLProgramSharedPtr const & glslProgram)
{
    if (!TF_VERIFY(glslProgram)) return false;

    return glslProgram->Link();
}

PXR_NAMESPACE_CLOSE_SCOPE

