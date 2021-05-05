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
#include "pxr/imaging/hdx/colorChannelTask.h"
#include "pxr/imaging/hdx/fullscreenShader.h"
#include "pxr/imaging/hdx/package.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hf/perfLog.h"
#include "pxr/imaging/hio/glslfx.h"

#include "pxr/imaging/hgi/blitCmds.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((colorChannelFrag, "ColorChannelFragment"))
    (colorIn)
);

HdxColorChannelTask::HdxColorChannelTask(
    HdSceneDelegate* delegate, 
    SdfPath const& id)
  : HdxTask(id)
  , _channel(HdxColorChannelTokens->color)
{
}

HdxColorChannelTask::~HdxColorChannelTask() = default;

void
HdxColorChannelTask::_Sync(HdSceneDelegate* delegate,
                           HdTaskContext* ctx,
                           HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!_compositor) {
        _compositor = std::make_unique<HdxFullscreenShader>(
            _GetHgi(), "ColorChannel");
    }

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        HdxColorChannelTaskParams params;

        if (_GetTaskParams(delegate, &params)) {
            _channel = params.channel;
        }
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxColorChannelTask::Prepare(HdTaskContext* ctx,
                             HdRenderIndex* renderIndex)
{
}

void
HdxColorChannelTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HgiTextureHandle aovTexture;
    _GetTaskContextData(ctx, HdAovTokens->color, &aovTexture);
    
    HgiShaderFunctionDesc fragDesc;
    fragDesc.debugName = _tokens->colorChannelFrag.GetString();
    fragDesc.shaderStage = HgiShaderStageFragment;
    HgiShaderFunctionAddStageInput(
        &fragDesc, "uvOut", "vec2");
    HgiShaderFunctionAddTexture(
        &fragDesc, "colorIn");
    HgiShaderFunctionAddStageOutput(
        &fragDesc, "hd_FragColor", "vec4", "color");

    // The order of the constant parameters has to match the order in the 
    // _ParameterBuffer struct
    HgiShaderFunctionAddConstantParam(
        &fragDesc, "screenSize", "vec2");
    HgiShaderFunctionAddConstantParam(
        &fragDesc, "channel", "int");

    _compositor->SetProgram(
        HdxPackageColorChannelShader(), 
        _tokens->colorChannelFrag,
        fragDesc);
    const auto &aovDesc = aovTexture->GetDescriptor();
    if (_UpdateParameterBuffer(
            static_cast<float>(aovDesc.dimensions[0]),
            static_cast<float>(aovDesc.dimensions[1]))) {
        size_t byteSize = sizeof(_ParameterBuffer);
        _compositor->SetShaderConstants(byteSize, &_parameterData);
    }

    _compositor->BindTextures(
        {_tokens->colorIn}, 
        {aovTexture});

    _compositor->Draw(aovTexture, /*no depth*/HgiTextureHandle());
}

bool
HdxColorChannelTask::_UpdateParameterBuffer(
    float screenSizeX, float screenSizeY)
{
    _ParameterBuffer pb;

    // Get an integer that represents the color channel. This can be used to
    // pass the color channel option as into the glsl shader.
    // (see the `#define CHANNEL_*` lines in the shader). 
    // If _channel contains an invalid entry the shader will return 'color'.
    int i = 0;
    for(const TfToken& channelToken : HdxColorChannelTokens->allTokens) {
        if (channelToken == _channel) break;
        ++i;
    }
    pb.channel = i;
    pb.screenSize[0] = screenSizeX;
    pb.screenSize[1] = screenSizeY;

    // All data is still the same, no need to update the storage buffer
    if (pb == _parameterData) {
        return false;
    }

    _parameterData = pb;

    return true;
}


// -------------------------------------------------------------------------- //
// VtValue Requirements
// -------------------------------------------------------------------------- //

std::ostream& operator<<(
    std::ostream& out, 
    const HdxColorChannelTaskParams& pv)
{
    out << "ColorChannelTask Params: (...) "
        << pv.channel << " "
    ;
    return out;
}

bool operator==(const HdxColorChannelTaskParams& lhs,
                const HdxColorChannelTaskParams& rhs)
{
    return lhs.channel == rhs.channel;
}

bool operator!=(const HdxColorChannelTaskParams& lhs,
                const HdxColorChannelTaskParams& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
