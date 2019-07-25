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
#include "pxr/imaging/glf/contextCaps.h"

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

static bool
_IsOitEnabled()
{
    if (!bool(TfGetEnvSetting(HDX_ENABLE_OIT))) return false;

    GlfContextCaps const &caps = GlfContextCaps::GetInstance();
    if (!caps.shaderStorageBufferEnabled) return false;

    return true;
}

HdxOitRenderTask::HdxOitRenderTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdxRenderTask(delegate, id)
    , _oitTranslucentRenderPassShader()
    , _oitOpaqueRenderPassShader()
    , _bufferSize(0)
    , _isOitEnabled(true)
{
    _isOitEnabled = _IsOitEnabled();

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
    HF_MALLOC_TAG_FUNCTION();

    if (_isOitEnabled) {
        HdxRenderTask::Sync(delegate, ctx, dirtyBits);
    }
}

void
HdxOitRenderTask::Prepare(HdTaskContext* ctx,
                       HdRenderIndex* renderIndex)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (_isOitEnabled) {
        HdxRenderTask::Prepare(ctx, renderIndex);

        // OIT buffers take up significant GPU resources. Skip if there are no
        // oit draw items (i.e. no translucent or volumetric draw items)
        if (_GetDrawItemCount() > 0) {
            _PrepareOitBuffers(ctx, renderIndex); 
        }
    }
}

void
HdxOitRenderTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!_isOitEnabled) return;
    if (_GetDrawItemCount() == 0) return;

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

    GlfContextCaps const &caps = GlfContextCaps::GetInstance();

    if (ARCH_LIKELY(caps.directStateAccessEnabled) && glGetTextureLevelParameteriv) {
        if (attachType == GL_TEXTURE) {
            glGetTextureLevelParameteriv(attachId, 0, GL_TEXTURE_WIDTH, &s[0]);
            glGetTextureLevelParameteriv(attachId, 0, GL_TEXTURE_HEIGHT, &s[1]);
        } else if (attachType == GL_RENDERBUFFER) {
            glGetNamedRenderbufferParameteriv(
                attachId, GL_RENDERBUFFER_WIDTH, &s[0]);
            glGetNamedRenderbufferParameteriv(
                attachId, GL_RENDERBUFFER_HEIGHT, &s[1]);
        }
    } else {
        if (attachType == GL_TEXTURE) {
            int oldBinding;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldBinding);
            glBindTexture(GL_TEXTURE_2D, attachId);
            glGetTexLevelParameteriv(GL_TEXTURE_2D,0, GL_TEXTURE_WIDTH, &s[0]);
            glGetTexLevelParameteriv(GL_TEXTURE_2D,0, GL_TEXTURE_HEIGHT, &s[1]);
            glBindTexture(GL_TEXTURE_2D, oldBinding);
        } else if (attachType == GL_RENDERBUFFER) {
            int oldBinding;
            glGetIntegerv(GL_RENDERBUFFER_BINDING, &oldBinding);
            glBindRenderbuffer(GL_RENDERBUFFER, attachId);
            glGetRenderbufferParameteriv(
                GL_RENDERBUFFER,GL_RENDERBUFFER_WIDTH,&s[0]);
            glGetRenderbufferParameteriv(
                GL_RENDERBUFFER,GL_RENDERBUFFER_HEIGHT,&s[1]);
            glBindRenderbuffer(GL_RENDERBUFFER, oldBinding);
        }
    }

    return s;
}

void
HdxOitRenderTask::_PrepareOitBuffers(
    HdTaskContext* ctx, 
    HdRenderIndex* renderIndex)
{
    const int numSamples = 8; // Should match glslfx files

    HdResourceRegistrySharedPtr const& resourceRegistry = 
        renderIndex->GetResourceRegistry();

    bool createOitBuffers = !_counterBar;
    if (createOitBuffers) { 
        //
        // Counter Buffer
        //
        HdBufferSpecVector counterSpecs;
        counterSpecs.push_back(HdBufferSpec(
            HdxTokens->hdxOitCounterBuffer, 
            HdTupleType {HdTypeInt32, 1}));
        _counterBar = resourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitCounter,
                                            counterSpecs,
                                            HdBufferArrayUsageHint());

        _oitTranslucentRenderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitCounterBufferBar, _counterBar,
                             /*interleave*/false));

        //
        // Index Buffer
        //
        HdBufferSpecVector indexSpecs;
        indexSpecs.push_back(HdBufferSpec(
            HdxTokens->hdxOitIndexBuffer,
            HdTupleType {HdTypeInt32, 1}));
        _indexBar = resourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitIndices,
                                            indexSpecs,
                                            HdBufferArrayUsageHint());

        _oitTranslucentRenderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitIndexBufferBar, _indexBar,
                             /*interleave*/false));

        //
        // Data Buffer
        //        
        HdBufferSpecVector dataSpecs;
        dataSpecs.push_back(HdBufferSpec(
            HdxTokens->hdxOitDataBuffer, 
            HdTupleType {HdTypeFloatVec4, 1}));
        _dataBar = resourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitData,
                                            dataSpecs,
                                            HdBufferArrayUsageHint());

        _oitTranslucentRenderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitDataBufferBar, _dataBar,
                             /*interleave*/false));

        //
        // Depth Buffer
        //
        HdBufferSpecVector depthSpecs;
        depthSpecs.push_back(HdBufferSpec(
            HdxTokens->hdxOitDepthBuffer, 
            HdTupleType {HdTypeFloat, 1}));
        _depthBar = resourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitDepth,
                                            depthSpecs,
                                            HdBufferArrayUsageHint());

        _oitTranslucentRenderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitDepthBufferBar, _depthBar,
                             /*interleave*/false));

        //
        // Uniforms
        //
        HdBufferSpecVector uniformSpecs;
        uniformSpecs.push_back( HdBufferSpec(
            HdxTokens->oitScreenSize,HdTupleType{HdTypeInt32Vec2, 1}));

        _uniformBar = resourceRegistry->AllocateUniformBufferArrayRange(
                                            /*role*/HdxTokens->oitUniforms,
                                            uniformSpecs,
                                            HdBufferArrayUsageHint());

        _oitTranslucentRenderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::UBO, 
                             HdxTokens->oitUniformBar, _uniformBar,
                             /*interleave*/true));
    }

    // Make sure task context has our buffer each frame (in case its cleared)
    (*ctx)[HdxTokens->oitCounterBufferBar] = _counterBar;
    (*ctx)[HdxTokens->oitIndexBufferBar] = _indexBar;
    (*ctx)[HdxTokens->oitDataBufferBar] = _dataBar;
    (*ctx)[HdxTokens->oitDepthBufferBar] = _depthBar;
    (*ctx)[HdxTokens->oitUniformBar] = _uniformBar;

    // The OIT buffer are sized based on the size of the screen.
    GfVec2i screenSize = _GetScreenSize();
    int newBufferSize = screenSize[0] * screenSize[1];
    bool resizeOitBuffers = (newBufferSize > _bufferSize);

    if (resizeOitBuffers) {
        _bufferSize = newBufferSize;

        // +1 because element 0 of the counter buffer is used as an atomic
        // counter in the shader to give each fragment a unique index.
        _counterBar->Resize(newBufferSize + 1);
        _indexBar->Resize(newBufferSize * numSamples);
        _dataBar->Resize(newBufferSize * numSamples);
        _depthBar->Resize(newBufferSize * numSamples);;

        // Update the values in the uniform buffer
        HdBufferSourceSharedPtrVector uniformSources;
        uniformSources.push_back(HdBufferSourceSharedPtr(
                              new HdVtBufferSource(HdxTokens->oitScreenSize,
                                                   VtValue(screenSize))));
        resourceRegistry->AddSources(_uniformBar, uniformSources);
    }
}

void 
HdxOitRenderTask::_ClearOitGpuBuffers(HdTaskContext* ctx)
{
    // The shader determines what elements in each buffer are used based on
    // finding -1 in the counter buffer. We can skip clearing the other buffers.

    HdStBufferArrayRangeGLSharedPtr stCounterBar =
        boost::dynamic_pointer_cast<HdStBufferArrayRangeGL> (_counterBar);
    HdStBufferResourceGLSharedPtr stCounterResource = 
        stCounterBar->GetResource(HdxTokens->hdxOitCounterBuffer);

    GlfContextCaps const &caps = GlfContextCaps::GetInstance();
    const GLint clearCounter = -1;

    // Old versions of glew may be missing glClearNamedBufferData
    if (ARCH_LIKELY(caps.directStateAccessEnabled) && glClearNamedBufferData) {
        glClearNamedBufferData(stCounterResource->GetId(),
                                GL_R32I,
                                GL_RED_INTEGER,
                                GL_INT,
                                &clearCounter);
    } else {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, stCounterResource->GetId());
        glClearBufferData(
            GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED_INTEGER, GL_INT,
            &clearCounter);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
