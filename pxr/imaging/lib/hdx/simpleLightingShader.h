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
#ifndef HDX_SIMPLE_LIGHTING_SHADER_H
#define HDX_SIMPLE_LIGHTING_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/resource.h"
#include "pxr/imaging/hdSt/lightingShader.h"

#include "pxr/imaging/glf/bindingMap.h"
#include "pxr/imaging/glf/glslfx.h"
#include "pxr/imaging/glf/simpleLightingContext.h"

#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/token.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<class HdxSimpleLightingShader> HdxSimpleLightingShaderSharedPtr;

/// \class HdxSimpleLightingShader
///
/// A shader that supports simple lighting functionality.
///
class HdxSimpleLightingShader : public HdStLightingShader {
public:
    HDX_API
    HdxSimpleLightingShader();
    HDX_API
    virtual ~HdxSimpleLightingShader();

    /// HdShader overrides
    HDX_API
    virtual ID ComputeHash() const;
    HDX_API
    virtual std::string GetSource(TfToken const &shaderStageKey) const;
    HDX_API
    virtual void BindResources(Hd_ResourceBinder const &binder, int program);
    HDX_API
    virtual void UnbindResources(Hd_ResourceBinder const &binder, int program);
    HDX_API
    virtual void AddBindings(HdBindingRequestVector *customBindings);

    /// HdStLightingShader overrides
    HDX_API
    virtual void SetCamera(GfMatrix4d const &worldToViewMatrix,
                           GfMatrix4d const &projectionMatrix);

    HDX_API
    void SetLightingStateFromOpenGL();
    HDX_API
    void SetLightingState(GlfSimpleLightingContextPtr const &lightingContext);

    GlfSimpleLightingContextRefPtr GetLightingContext() {
        return _lightingContext;
    };

private:
    GlfSimpleLightingContextRefPtr _lightingContext; 
    GlfBindingMapRefPtr _bindingMap;
    bool _useLighting;
    boost::scoped_ptr<GlfGLSLFX> _glslfx;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDX_SIMPLE_LIGHTING_SHADER_H
