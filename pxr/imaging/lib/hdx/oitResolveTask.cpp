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

#include "pxr/imaging/hdx/oitResolveTask.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/debugCodes.h"
#include "pxr/imaging/hdx/package.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/imageShaderRenderPass.h"
#include "pxr/imaging/hdSt/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

class HdxOitResolveRenderPassShader : public HdStRenderPassShader {
public:
    HdxOitResolveRenderPassShader(int numSamples) :
        HdStRenderPassShader(HdxPackageOitResolveImageShader()),
        _numSamples(numSamples)
    {
        // The hash of this shader is constant, no custom bindings and the
        // input parameters are constant.
        _hash = HdStRenderPassShader::ComputeHash();
        boost::hash_combine(_hash, numSamples);
    }

    std::string GetSource(const TfToken& shaderStageKey) const override
    {
        const auto src = HdStRenderPassShader::GetSource(shaderStageKey);

        std::stringstream defines;
        defines << "#define OIT_NUM_SAMPLES "
                << _numSamples
                << "\n";

        return defines.str() + src;
    }

    ID ComputeHash() const override
    {
        return _hash;
    }

    ~HdxOitResolveRenderPassShader() override = default;
private:
    HdxOitResolveRenderPassShader(const HdxOitResolveRenderPassShader&)             = delete;
    HdxOitResolveRenderPassShader& operator=(const  HdxOitResolveRenderPassShader&) = delete;

    const int _numSamples;
    ID _hash;
};

}

HdxOitResolveTask::HdxOitResolveTask(
    HdSceneDelegate* delegate, 
    SdfPath const& id)
    : HdTask(id)
{
}

HdxOitResolveTask::~HdxOitResolveTask()
{
}

void
HdxOitResolveTask::Sync(
    HdSceneDelegate* delegate,
    HdTaskContext*   ctx,
    HdDirtyBits*     dirtyBits)
{
    HD_TRACE_FUNCTION();
    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxOitResolveTask::Prepare(HdTaskContext* ctx,
                       HdRenderIndex* renderIndex)
{
    HdRenderDelegate* renderDelegate = renderIndex->GetRenderDelegate();
    if (!TF_VERIFY(dynamic_cast<HdStRenderDelegate*>(renderDelegate),
                   "OIT Task only works with HdSt")) {
        return;
    }
    VtValue oitNumSamples = renderDelegate
        ->GetRenderSetting(HdStRenderSettingsTokens->oitNumSamples);
    if (!TF_VERIFY(oitNumSamples.IsHolding<int>(),
        "OIT Number of Samples is not an integer!")) {
        return;
    }
    const int numSamples = std::max(1, oitNumSamples.UncheckedGet<int>());
    bool rebuildShader = false;
    if (numSamples != _numSamples) {
        _numSamples = numSamples;
        rebuildShader = true;
    }

    if (!_renderPass) {
        HdRprimCollection collection;

        _renderPass = HdRenderPassSharedPtr(
            new HdSt_ImageShaderRenderPass(renderIndex, collection));

        // We do not use renderDelegate->CreateRenderPassState because
        // ImageShaders always use HdSt
        _renderPassState = boost::make_shared<HdStRenderPassState>();
        _renderPassState->SetEnableDepthMask(false);
        _renderPassState->SetColorMask(HdRenderPassState::ColorMaskRGBA);
        _renderPassState->SetBlendEnabled(true);
        _renderPassState->SetBlend(
            HdBlendOp::HdBlendOpAdd,
            HdBlendFactor::HdBlendFactorOne,
            HdBlendFactor::HdBlendFactorOneMinusSrcAlpha,
            HdBlendOp::HdBlendOpAdd,
            HdBlendFactor::HdBlendFactorOne,
            HdBlendFactor::HdBlendFactorOne);

        _renderPassShader.reset(new HdxOitResolveRenderPassShader(
            _numSamples));

        HdStRenderPassState* stRenderPassState =
            dynamic_cast<HdStRenderPassState*>(_renderPassState.get());
        stRenderPassState->SetRenderPassShader(_renderPassShader);
    } else if (rebuildShader) {
        _renderPassShader.reset(new HdxOitResolveRenderPassShader(
            _numSamples));
        HdStRenderPassState* stRenderPassState =
            dynamic_cast<HdStRenderPassState*>(_renderPassState.get());
        stRenderPassState->SetRenderPassShader(_renderPassShader);
    }
}

void
HdxOitResolveTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(_renderPassState)) return;

    HdStRenderPassState* stRenderPassState =
            dynamic_cast<HdStRenderPassState*>(_renderPassState.get());

    // Resolve Setup
    // Note that resolveTask comes after render + oitRender tasks,
    // so it can blend their data
    VtValue vc = (*ctx)[HdxTokens->oitCounterBufferBar];
    VtValue vda = (*ctx)[HdxTokens->oitDataBufferBar];
    VtValue vde = (*ctx)[HdxTokens->oitDepthBufferBar];
    VtValue vi = (*ctx)[HdxTokens->oitIndexBufferBar];
    VtValue vu = (*ctx)[HdxTokens->oitUniformBar];

    HdStRenderPassShaderSharedPtr renderPassShader
        = stRenderPassState->GetRenderPassShader();

    if (!vda.IsEmpty()) {
        HdBufferArrayRangeSharedPtr cbar
            = vc.Get<HdBufferArrayRangeSharedPtr>();
        HdBufferArrayRangeSharedPtr dabar
            = vda.Get<HdBufferArrayRangeSharedPtr>();
        HdBufferArrayRangeSharedPtr debar
            = vde.Get<HdBufferArrayRangeSharedPtr>();
        HdBufferArrayRangeSharedPtr ibar
            = vi.Get<HdBufferArrayRangeSharedPtr>();
        HdBufferArrayRangeSharedPtr ubar
            = vu.Get<HdBufferArrayRangeSharedPtr>();

        renderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitCounterBufferBar, cbar,
                             /*interleave*/false));
        renderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitDataBufferBar, dabar,
                             /*interleave*/false));
        renderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitDepthBufferBar, debar,
                             /*interleave*/false));
        renderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->oitIndexBufferBar, ibar,
                             /*interleave*/false));
        renderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::UBO, 
                             HdxTokens->oitUniformBar, ubar,
                             /*interleave*/true));
    } else {
        renderPassShader->RemoveBufferBinding(HdxTokens->oitCounterBufferBar);
        renderPassShader->RemoveBufferBinding(HdxTokens->oitDataBufferBar);
        renderPassShader->RemoveBufferBinding(HdxTokens->oitDepthBufferBar);
        renderPassShader->RemoveBufferBinding(HdxTokens->oitIndexBufferBar);
        renderPassShader->RemoveBufferBinding(HdxTokens->oitUniformBar);
    }

    _renderPassState->Bind(); 

    glDisable(GL_DEPTH_TEST);

    _renderPass->Execute(_renderPassState, GetRenderTags());

    glEnable(GL_DEPTH_TEST);

    _renderPassState->Unbind();
}


PXR_NAMESPACE_CLOSE_SCOPE
