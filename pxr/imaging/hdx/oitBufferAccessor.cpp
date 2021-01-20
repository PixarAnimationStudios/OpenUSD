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
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hdx/oitBufferAccessor.h"

#include "pxr/imaging/glf/contextCaps.h"

#include "pxr/imaging/hdSt/bufferArrayRange.h"
#include "pxr/imaging/hdSt/bufferResource.h"
#include "pxr/imaging/hdSt/renderPassShader.h"

// XXX todo tmp needed until we remove direct gl calls below.
#include "pxr/imaging/hgiGL/buffer.h"

#include "pxr/imaging/hdx/tokens.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HDX_ENABLE_OIT, true, 
                      "Enable order independent translucency");

/* static */
bool
HdxOitBufferAccessor::IsOitEnabled()
{
    if (!bool(TfGetEnvSetting(HDX_ENABLE_OIT))) return false;

    GlfContextCaps const &caps = GlfContextCaps::GetInstance();
    if (!caps.shaderStorageBufferEnabled) return false;

    return true;
}

HdxOitBufferAccessor::HdxOitBufferAccessor(HdTaskContext *ctx)
    : _ctx(ctx)
{
}

void
HdxOitBufferAccessor::RequestOitBuffers()
{
    (*_ctx)[HdxTokens->oitRequestFlag] = VtValue(true);
}

HdBufferArrayRangeSharedPtr const &
HdxOitBufferAccessor::_GetBar(const TfToken &name)
{
    const auto it = _ctx->find(name);
    if (it == _ctx->end()) {
        static HdBufferArrayRangeSharedPtr n;
        return n;
    }

    const VtValue &v = it->second;
    return v.Get<HdBufferArrayRangeSharedPtr>();
}

bool
HdxOitBufferAccessor::AddOitBufferBindings(
    const HdStRenderPassShaderSharedPtr &shader)
{
    HdBufferArrayRangeSharedPtr const & counterBar =
        _GetBar(HdxTokens->oitCounterBufferBar);
    HdBufferArrayRangeSharedPtr const & dataBar =
        _GetBar(HdxTokens->oitDataBufferBar);
    HdBufferArrayRangeSharedPtr const & depthBar =
        _GetBar(HdxTokens->oitDepthBufferBar);
    HdBufferArrayRangeSharedPtr const & indexBar =
        _GetBar(HdxTokens->oitIndexBufferBar);
    HdBufferArrayRangeSharedPtr const & uniformBar =
        _GetBar(HdxTokens->oitUniformBar);

    if (counterBar && dataBar && depthBar && indexBar && uniformBar) {
        shader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitCounterBufferBar,
                             counterBar,
                             /*interleave = */ false));

        shader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitDataBufferBar,
                             dataBar,
                             /*interleave = */ false));
        
        shader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitDepthBufferBar,
                             depthBar,
                             /*interleave = */ false));
        
        shader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitIndexBufferBar,
                             indexBar,
                             /*interleave = */ false));
        
        shader->AddBufferBinding(
            HdBindingRequest(HdBinding::UBO, 
                             HdxTokens->oitUniformBar,
                             uniformBar,
                             /*interleave = */ true));
        return true;
    } else {
        shader->RemoveBufferBinding(HdxTokens->oitCounterBufferBar);
        shader->RemoveBufferBinding(HdxTokens->oitDataBufferBar);
        shader->RemoveBufferBinding(HdxTokens->oitDepthBufferBar);
        shader->RemoveBufferBinding(HdxTokens->oitIndexBufferBar);
        shader->RemoveBufferBinding(HdxTokens->oitUniformBar);
        return false;
    }
}

void
HdxOitBufferAccessor::InitializeOitBuffersIfNecessary() 
{
    // If the OIT buffers were already cleared earlier, skip and do not
    // clear them again.
    VtValue &clearFlag = (*_ctx)[HdxTokens->oitClearedFlag];
    if (!clearFlag.IsEmpty()) {
        return;
    }

    // Mark OIT buffers as cleared.
    clearFlag = true;

    // Clear counter buffer.
    
    // The shader determines what elements in each buffer are used based on
    // finding -1 in the counter buffer. We can skip clearing the other buffers.

    HdStBufferArrayRangeSharedPtr stCounterBar =
        std::dynamic_pointer_cast<HdStBufferArrayRange>(
            _GetBar(HdxTokens->oitCounterBufferBar));

    if (!stCounterBar) {
        TF_CODING_ERROR(
            "No OIT counter buffer allocateed when trying to clear it");
        return;
    }

    HdStBufferResourceSharedPtr stCounterResource = 
        stCounterBar->GetResource(HdxTokens->hdxOitCounterBuffer);

    GlfContextCaps const &caps = GlfContextCaps::GetInstance();
    const GLint clearCounter = -1;

    // XXX todo add a Clear() fn on HdStBufferResource so that we do not have
    // to use direct gl calls. below.
    HgiBufferHandle& buffer = stCounterResource->GetId();
    HgiGLBuffer* glBuffer = dynamic_cast<HgiGLBuffer*>(buffer.Get());
    if (!glBuffer) {
        TF_CODING_ERROR("Todo: Add HdStBufferResource::Clear");
        return;
    }

    if (ARCH_LIKELY(caps.directStateAccessEnabled)) {
        glClearNamedBufferData(glBuffer->GetBufferId(),
                                GL_R32I,
                                GL_RED_INTEGER,
                                GL_INT,
                                &clearCounter);
    } else {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, glBuffer->GetBufferId());
        glClearBufferData(
            GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED_INTEGER, GL_INT,
            &clearCounter);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
