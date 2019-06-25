//
// Copyright 2019 Pixar
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

#include "pxr/base/tf/envSetting.h"

#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hdx/oitRenderTask.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/debugCodes.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hdSt/lightingShader.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/bufferArrayRangeGL.h"
#include "pxr/imaging/hdSt/bufferResourceGL.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HDX_ENABLE_OIT, true, 
                      "Enable order independent translucency");

typedef std::vector<HdBufferSourceSharedPtr> HdBufferSourceSharedPtrVector;

// -------------------------------------------------------------------------- //

HdxOitRenderTask::HdxOitRenderTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdxRenderTask(delegate, id)
    , _oitTranslucentRenderPassShader()
    , _oitOpaqueRenderPassShader()
    , _bufferSize(0)
    , _screenSize(1,1)
{
    _oitTranslucentRenderPassShader.reset(
        new HdStRenderPassShader(HdxPackageRenderPassOitShader()));

    _oitOpaqueRenderPassShader.reset(
        new HdStRenderPassShader(HdxPackageRenderPassOitOpaqueShader()));
}

HdxOitRenderTask::~HdxOitRenderTask()
{
}

void
HdxOitRenderTask::Sync(
    HdSceneDelegate* delegate,
    HdTaskContext* ctx,
    HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();

    HdxRenderTask::Sync(delegate, ctx, dirtyBits);
}

void
HdxOitRenderTask::Prepare(HdTaskContext* ctx,
                       HdRenderIndex* renderIndex)
{
    HdxRenderTask::Prepare(ctx, renderIndex);
    _PrepareOitBuffers(ctx, renderIndex); 
}

void
HdxOitRenderTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    //
    // Pre Execute Setup
    //

    HdRenderPassStateSharedPtr renderPassState = _GetRenderPassState(ctx);
    if (!TF_VERIFY(renderPassState)) return;

    HdStRenderPassState* extendedState =
        dynamic_cast<HdStRenderPassState*>(renderPassState.get());
    if (!TF_VERIFY(extendedState, "OIT only works with HdSt")) {
        return;
    }

    extendedState->SetOverrideShader(HdStShaderCodeSharedPtr());

    _ClearOitGpuBuffers(ctx);

    // We render into a SSBO -- not MSSA compatible
    bool oldMSAA = glIsEnabled(GL_MULTISAMPLE);
    glDisable(GL_MULTISAMPLE);
    // XXX When rendering HdStPoints we set GL_POINTS and assume that
    //     GL_POINT_SMOOTH is enabled by default. This renders circles instead
    //     of squares. However, when toggling MSAA off (above) we see GL_POINTS
    //     start to render squares (driver bug?).
    //     For now we always enable GL_POINT_SMOOTH. 
    // XXX Switch points rendering to emit quad with FS that draws circle.
    bool oldPointSmooth = glIsEnabled(GL_POINT_SMOOTH);
    glEnable(GL_POINT_SMOOTH);

    //
    // Opaque pixels pass
    // These pixels are rendered to FB instead of OIT buffers
    //
    extendedState->SetRenderPassShader(_oitOpaqueRenderPassShader);
    renderPassState->SetEnableDepthMask(true);
    renderPassState->SetColorMask(HdRenderPassState::ColorMaskRGBA);
    HdxRenderTask::Execute(ctx);

    //
    // Translucent pixels pass
    //
    extendedState->SetRenderPassShader(_oitTranslucentRenderPassShader);
    renderPassState->SetEnableDepthMask(false);
    renderPassState->SetColorMask(HdRenderPassState::ColorMaskNone);
    HdxRenderTask::Execute(ctx);

    //
    // Post Execute Restore
    //

    if (oldMSAA) {
        glEnable(GL_MULTISAMPLE);
    }

    if (!oldPointSmooth) {
        glDisable(GL_POINT_SMOOTH);
    }
}

static int 
_RoundUp(int number, int roundTo)
{
    int remainder = number % roundTo;
    return number + (roundTo - remainder);
}

static GfVec2i
_GetScreenSize()
{
    // XXX Ideally we want screenSize to be passed in via the app. 
    // (see Presto Stagecontext/TaskGraph), but for now we query this from GL.
    //
    // Using GL_VIEWPORT here (or viewport from RenderParams) is in-correct!
    //
    // The gl_FragCoord we use in the OIT shaders is relative to the FRAMEBUFFER 
    // size (screen size), not the gl_viewport size.
    // We do various tricks with glViewport for Presto slate mode so we cannot
    // rely on it to determine the 'screenWidth' we need in the gl shaders.
    // 
    // The CounterBuffer is especially fragile to this because in the glsl shdr
    // we calculate a 'screenIndex' based on gl_fragCoord that indexes into
    // the CounterBuffer. If we did not make enough room in the CounterBuffer
    // we may be reading/writing an invalid index into the CounterBuffer.
    //

    GfVec2i s;

    GLint attachType = 0;
    glGetFramebufferAttachmentParameteriv(
        GL_DRAW_FRAMEBUFFER, 
        GL_COLOR_ATTACHMENT0,
        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
        &attachType);

    GLint attachId = 0;
    glGetFramebufferAttachmentParameteriv(
        GL_DRAW_FRAMEBUFFER, 
        GL_COLOR_ATTACHMENT0,
        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
        &attachId);

    // XXX Fallback to gl viewport in case we do not find a non-default FBO for
    // bakends that do not attach a custom FB. This is in-correct, but gl does
    // not let us query size properties of default framebuffer. For this we
    // need the screenSize to be passed in via app (see note above)
    if (attachId<=0) {
        GfVec4i viewport;
        glGetIntegerv(GL_VIEWPORT, &viewport[0]);
        s[0] = viewport[2];
        s[1] = viewport[3];
        return s;
    }

    if (attachType == GL_TEXTURE) {
        glGetTextureLevelParameteriv(attachId, 0, GL_TEXTURE_WIDTH, &s[0]);
        glGetTextureLevelParameteriv(attachId, 0, GL_TEXTURE_HEIGHT, &s[1]);
    } else if (attachType == GL_RENDERBUFFER) {
        glGetNamedRenderbufferParameteriv(attachId,GL_RENDERBUFFER_WIDTH,&s[0]);
        glGetNamedRenderbufferParameteriv(attachId,GL_RENDERBUFFER_HEIGHT,&s[1]);
    } else {
        TF_CODING_ERROR("Unknown framebuffer attachment");
        return s;
    }

    return s;
}

void
HdxOitRenderTask::_PrepareOitBuffers(
    HdTaskContext* ctx, 
    HdRenderIndex* renderIndex)
{
    // XXX OIT can be globally disabled to preserve GPU memory
    if (!bool(TfGetEnvSetting(HDX_ENABLE_OIT))) return;

    // XXX Exit if opengl version too old
    if (!glGetTextureLevelParameteriv) return;

    const int numSamples = 8; // Should match glslfx files

    HdResourceRegistrySharedPtr const& resourceRegistry = 
        renderIndex->GetResourceRegistry();

    GfVec2i s = _GetScreenSize();
    bool screenChanged = s != _screenSize;
    _screenSize = s;

    // Rebuilding the buffers is a slow operation that slows viewport
    // resizing and requires garbage collection. For this reason we only grow 
    // the OIT buffers (in 512^2 steps) and do not shrink them.    
    int newBufferSize = _RoundUp(_screenSize[0] * _screenSize[1], 512*512);
    bool rebuildOitBuffers = (newBufferSize > _bufferSize);

    if (rebuildOitBuffers) {
        // If glew version too old we emit a warning since OIT will not work.
        if (!glClearNamedBufferData) {
            TF_WARN("glClearNamedBufferData missing for OIT (old glew?)");
        }

        _counterBar.reset();
        _dataBar.reset();
        _depthBar.reset();
        _indexBar.reset();
        _bufferSize = newBufferSize;
        renderIndex->GetChangeTracker().SetGarbageCollectionNeeded();
    }

    if (screenChanged) {
        _uniformBar.reset();
        renderIndex->GetChangeTracker().SetGarbageCollectionNeeded();
    }

    //
    // Counter Buffer
    //
    if (!_counterBar) { 
        HdBufferSpecVector specs;
        specs.push_back(HdBufferSpec(
            HdxTokens->hdxOitCounterBuffer, 
            HdTupleType {HdTypeInt32, 1}));
        _counterBar = resourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitCounter,
                                            specs,
                                            HdBufferArrayUsageHint());

        // +1 because element 0 of the counter buffer is used as an atomic
        // counter in the shader to give each fragment a unique index.
        VtIntArray counters;
        size_t countersSize = newBufferSize + 1; 
        int countersValue = -1; 
        counters.assign(countersSize, countersValue);

        HdBufferSourceSharedPtr counterSource(
                new HdVtBufferSource(HdxTokens->hdxOitCounterBuffer,
                                    VtValue(counters)));
        resourceRegistry->AddSource(_counterBar, counterSource);
    }

    (*ctx)[HdxTokens->oitCounterBufferBar] = _counterBar;

    //
    // Index Buffer
    //
    if (!_indexBar) { 
        HdBufferSpecVector specs;
        specs.push_back(HdBufferSpec(
            HdxTokens->hdxOitIndexBuffer,
            HdTupleType {HdTypeInt32, 1}));
        _indexBar = resourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitIndices,
                                            specs,
                                            HdBufferArrayUsageHint());

        VtIntArray indices;
        size_t indicesSize = newBufferSize * numSamples; 
        int indicesValue = -1; 
        indices.assign(indicesSize, indicesValue);

        HdBufferSourceSharedPtr indicesSource(
                new HdVtBufferSource(HdxTokens->hdxOitIndexBuffer,
                                    VtValue(indices)));
        resourceRegistry->AddSource(_indexBar, indicesSource);
    }

    (*ctx)[HdxTokens->oitIndexBufferBar] = _indexBar;

    //
    // Data Buffer
    //
    if (!_dataBar) { 
        HdBufferSpecVector specs;
        specs.push_back(HdBufferSpec(
            HdxTokens->hdxOitDataBuffer, 
            HdTupleType {HdTypeFloatVec4, 1}));
        _dataBar = resourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitData,
                                            specs,
                                            HdBufferArrayUsageHint());

        VtVec4fArray dataArray;
        size_t dataSize = newBufferSize * numSamples;
        GfVec4f dataValue = GfVec4f(0.0f, 0.0f, 0.0f, 0.0f); 
        dataArray.assign(dataSize, dataValue);

        HdBufferSourceSharedPtr dataSource(
                new HdVtBufferSource(HdxTokens->hdxOitDataBuffer,
                                    VtValue(dataArray)));
        resourceRegistry->AddSource(_dataBar, dataSource);
    }

    (*ctx)[HdxTokens->oitDataBufferBar] = _dataBar;

    //
    // Depth Buffer
    //
    if (!_depthBar) { 
        HdBufferSpecVector specs;
        specs.push_back(HdBufferSpec(
            HdxTokens->hdxOitDepthBuffer, 
            HdTupleType {HdTypeFloat, 1}));
        _depthBar = resourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitDepth,
                                            specs,
                                            HdBufferArrayUsageHint());

        VtFloatArray depthArray;
        size_t depthSize = newBufferSize * numSamples; 
        float depthValue = 0.0f; 
        depthArray.assign(depthSize, depthValue);

        HdBufferSourceSharedPtr depthSource(
                new HdVtBufferSource(HdxTokens->hdxOitDepthBuffer,
                                    VtValue(depthArray)));
        resourceRegistry->AddSource(_depthBar, depthSource);
    }

    (*ctx)[HdxTokens->oitDepthBufferBar] = _depthBar;

    //
    // Uniforms
    //
    if (!_uniformBar) {
        HdBufferSpecVector specs;
        specs.push_back( HdBufferSpec(
            HdxTokens->oitBufferSize, HdTupleType {HdTypeInt32, 1}));
        specs.push_back( HdBufferSpec(
            HdxTokens->oitScreenSize,HdTupleType{HdTypeInt32Vec2, 1}));

        _uniformBar = resourceRegistry->AllocateUniformBufferArrayRange(
                                            /*role*/HdxTokens->oitUniforms,
                                            specs,
                                            HdBufferArrayUsageHint());

        HdBufferSourceSharedPtrVector uniformSources;
        uniformSources.push_back(HdBufferSourceSharedPtr(
                new HdVtBufferSource(HdxTokens->oitBufferSize,
                                    VtValue(newBufferSize*numSamples))));
        uniformSources.push_back(HdBufferSourceSharedPtr(
                              new HdVtBufferSource(HdxTokens->oitScreenSize,
                                                   VtValue(_screenSize))));
        resourceRegistry->AddSources(_uniformBar, uniformSources);
    }

    (*ctx)[HdxTokens->oitUniformBar] = _uniformBar;

    //
    // Binding Requests
    //
    if (rebuildOitBuffers) {
        _oitTranslucentRenderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitCounterBufferBar, _counterBar,
                             /*interleave*/false));
        _oitTranslucentRenderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitDataBufferBar, _dataBar,
                             /*interleave*/false));
        _oitTranslucentRenderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitDepthBufferBar, _depthBar,
                             /*interleave*/false));
        _oitTranslucentRenderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitIndexBufferBar, _indexBar,
                             /*interleave*/false));
    }

    if (screenChanged) {
        _oitTranslucentRenderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::UBO, 
                             HdxTokens->oitUniformBar, _uniformBar,
                             /*interleave*/true));
    }
}

void 
HdxOitRenderTask::_ClearOitGpuBuffers(HdTaskContext* ctx)
{
    // Exit if glew version used by app is too old
    if (!glClearNamedBufferData) return;
    if (!_counterBar) return;

    //
    // Counter Buffer
    //
    HdStBufferArrayRangeGLSharedPtr stCounterBar =
        boost::dynamic_pointer_cast<HdStBufferArrayRangeGL> (_counterBar);
    HdStBufferResourceGLSharedPtr stCounterResource = 
        stCounterBar->GetResource(HdxTokens->hdxOitCounterBuffer);

    const GLint clearCounter = -1;
    glClearNamedBufferData(stCounterResource->GetId(),
                            GL_R32I,
                            GL_RED_INTEGER,
                            GL_INT,
                            &clearCounter);

    //
    // Index Buffer
    //
    HdStBufferArrayRangeGLSharedPtr stIndexBar =
        boost::dynamic_pointer_cast<HdStBufferArrayRangeGL> (_indexBar);
    HdStBufferResourceGLSharedPtr stIndexResource = 
        stIndexBar->GetResource(HdxTokens->hdxOitIndexBuffer);
    const GLint clearIndex = -1;
    glClearNamedBufferData(stIndexResource->GetId(),
                            GL_R32I,
                            GL_RED_INTEGER,
                            GL_INT,
                            &clearIndex);

    //
    // Data Buffer
    //
    HdStBufferArrayRangeGLSharedPtr stDataBar =
        boost::dynamic_pointer_cast<HdStBufferArrayRangeGL> (_dataBar);
    HdStBufferResourceGLSharedPtr stDataResource = 
        stDataBar->GetResource(HdxTokens->hdxOitDataBuffer);
    const GLfloat clearData = 0.0f;
    glClearNamedBufferData(stDataResource->GetId(),
                            GL_RGBA32F,
                            GL_RED,
                            GL_FLOAT,
                            &clearData);

    //
    // Depth Buffer
    //
    HdStBufferArrayRangeGLSharedPtr stDepthBar =
        boost::dynamic_pointer_cast<HdStBufferArrayRangeGL> (_depthBar);
    HdStBufferResourceGLSharedPtr stDepthResource = 
        stDepthBar->GetResource(HdxTokens->hdxOitDepthBuffer);
    const GLfloat clearDepth = 0.0f;
    glClearNamedBufferData(stDepthResource->GetId(),
                            GL_R32F,
                            GL_RED,
                            GL_FLOAT,
                            &clearDepth);
}


PXR_NAMESPACE_CLOSE_SCOPE
