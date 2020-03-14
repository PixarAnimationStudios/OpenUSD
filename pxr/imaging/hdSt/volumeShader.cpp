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
#include "pxr/imaging/hdSt/volumeShader.h"

#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdSt/volume.h"
#include "pxr/imaging/hd/renderDelegate.h"

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (stepSize)
    (stepSizeLighting)
);

HdSt_VolumeShader::HdSt_VolumeShader(HdRenderDelegate * const renderDelegate)
    : _renderDelegate(renderDelegate),
      _lastRenderSettingsVersion(0),
      _stepSize(HdStVolume::defaultStepSize),
      _stepSizeLighting(HdStVolume::defaultStepSizeLighting)
{
}

HdSt_VolumeShader::~HdSt_VolumeShader() = default;

void 
HdSt_VolumeShader::AddBindings(HdBindingRequestVector * const customBindings)
{
    HdStSurfaceShader::AddBindings(customBindings);
    customBindings->push_back(
        HdBindingRequest(
            HdBinding::UNIFORM,
            _tokens->stepSize,
            HdTypeFloat));
    customBindings->push_back(
        HdBindingRequest(
            HdBinding::UNIFORM,
            _tokens->stepSizeLighting,
            HdTypeFloat));
}

void 
HdSt_VolumeShader::BindResources(const int program,
                                 HdSt_ResourceBinder const &binder,
                                 HdRenderPassState const &state)
{
    HdStSurfaceShader::BindResources(program, binder, state);
    
    const int currentRenderSettingsVersion =
        _renderDelegate->GetRenderSettingsVersion();
    
    if (_lastRenderSettingsVersion != currentRenderSettingsVersion) {
        _lastRenderSettingsVersion = currentRenderSettingsVersion;
        _stepSize = _renderDelegate->GetRenderSetting<float>(
            HdStRenderSettingsTokens->volumeRaymarchingStepSize,
            HdStVolume::defaultStepSize);
        _stepSizeLighting = _renderDelegate->GetRenderSetting<float>(
            HdStRenderSettingsTokens->volumeRaymarchingStepSizeLighting,
            HdStVolume::defaultStepSizeLighting);
    }
    
    binder.BindUniformf(_tokens->stepSize, 1, &_stepSize);
    binder.BindUniformf(_tokens->stepSizeLighting, 1, &_stepSizeLighting);
}

PXR_NAMESPACE_CLOSE_SCOPE
