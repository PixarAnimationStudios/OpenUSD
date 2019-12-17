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
#ifndef PXR_IMAGING_HD_ST_VOLUME_SHADER_H
#define PXR_IMAGING_HD_ST_VOLUME_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hdSt/surfaceShader.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderDelegate;

using HdSt_VolumeShaderSharedPtr = boost::shared_ptr<class HdSt_VolumeShader>;

/// A shader class that (on top of the behaviors of HdStSurfaceShader) querries
/// the render delegate for the raymarching step sizes and binds them to the
/// shader (as uniforms).
///
/// Note that we use HdStSurfaceShader as base class for a volume shader.
/// Despite its name, HdStSurfaceShader is really just a pair of
/// GLSL code and bindings and not specific to surface shading.
///
class HdSt_VolumeShader : public HdStSurfaceShader
{
public:
    explicit HdSt_VolumeShader(HdRenderDelegate * const renderDelegate);
    ~HdSt_VolumeShader() override;

    /// Adds custom bindings for step sizes so that codegen will make them
    /// available as HdGet_stepSize and HdGet_stepSizeLighting
    void AddBindings(HdBindingRequestVector * customBindings) override;
    
    /// Querries render delegate for step sizes and binds the uniforms.
    void BindResources(int program,
                       HdSt_ResourceBinder const &binder,
                       HdRenderPassState const &state) override;
    
private:
    HdRenderDelegate * const _renderDelegate;
    int _lastRenderSettingsVersion;
    float _stepSize;
    float _stepSizeLighting;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
