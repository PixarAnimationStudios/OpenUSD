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
#ifndef PXR_IMAGING_HDST_SIMPLE_LIGHTING_SHADER_H
#define PXR_IMAGING_HDST_SIMPLE_LIGHTING_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/lightingShader.h"

#include "pxr/imaging/hd/resource.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/imaging/glf/bindingMap.h"
#include "pxr/imaging/glf/simpleLightingContext.h"

#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/token.h"

#include <boost/shared_ptr.hpp>

#include <memory>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<class HdStSimpleLightingShader> 
    HdStSimpleLightingShaderSharedPtr;

/// \class HdStSimpleLightingShader
///
/// A shader that supports simple lighting functionality.
///
class HdStSimpleLightingShader : public HdStLightingShader 
{
public:
    HDST_API
    HdStSimpleLightingShader();
    HDST_API
    ~HdStSimpleLightingShader() override;

    /// HdShader overrides
    HDST_API
    ID ComputeHash() const override;
    HDST_API
    std::string GetSource(TfToken const &shaderStageKey) const override;
    HDST_API
    void BindResources(int program,
                       HdSt_ResourceBinder const &binder,
                       HdRenderPassState const &state) override;
    HDST_API
    void UnbindResources(int program,
                         HdSt_ResourceBinder const &binder,
                         HdRenderPassState const &state) override;
    HDST_API
    void AddBindings(HdBindingRequestVector *customBindings) override;

    /// HdStShaderCode overrides
    HDST_API
    HdSt_MaterialParamVector const& GetParams() const override;

    /// HdStLightingShader overrides
    HDST_API
    void SetCamera(
        GfMatrix4d const &worldToViewMatrix,
        GfMatrix4d const &projectionMatrix) override;
    HDST_API
    void SetLightingStateFromOpenGL();
    HDST_API
    void SetLightingState(GlfSimpleLightingContextPtr const &lightingContext);

    GlfSimpleLightingContextRefPtr GetLightingContext() {
        return _lightingContext;
    };

private:
    GlfSimpleLightingContextRefPtr _lightingContext; 
    GlfBindingMapRefPtr _bindingMap;
    bool _useLighting;
    std::unique_ptr<HioGlslfx> _glslfx;

    HdSt_MaterialParamVector _lightTextureParams;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDST_SIMPLE_LIGHTING_SHADER_H
