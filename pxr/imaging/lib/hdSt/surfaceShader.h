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
#ifndef HDST_SURFACESHADER_H
#define HDST_SURFACESHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hd/bufferSource.h"

#include "pxr/imaging/garch/gl.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/vt/value.h"
#include "pxr/base/tf/token.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class HdSceneDelegate;

typedef std::shared_ptr<class HdBufferArrayRange> HdBufferArrayRangeSharedPtr;
typedef std::shared_ptr<class HdStSurfaceShader> HdStSurfaceShaderSharedPtr;
typedef std::shared_ptr<class HdResourceRegistry> HdResourceRegistrySharedPtr;

/// \class HdStSurfaceShader
///
/// A scene-based SurfaceShader object.
///
/// When surface shaders are expresed in the scene graph, the HdSceneDelegate
/// can use this object to express these surface shaders in Hydra. In addition
/// to the shader itself, a binding from the Rprim to the SurfaceShader must be
/// expressed as well.
class HdStSurfaceShader : public HdStShaderCode {
public:
    HDST_API
    HdStSurfaceShader();
    HDST_API
    virtual ~HdStSurfaceShader();


    // ---------------------------------------------------------------------- //
    /// \name HdShader Virtual Interface                                      //
    // ---------------------------------------------------------------------- //
    HDST_API
    virtual std::string GetSource(TfToken const &shaderStageKey) const;
    HDST_API
    virtual HdMaterialParamVector const& GetParams() const;
    HDST_API
    virtual HdBufferArrayRangeSharedPtr const& GetShaderData() const;
    HDST_API
    virtual TextureDescriptorVector GetTextures() const;
    HDST_API
    virtual void BindResources(HdSt_ResourceBinder const &binder, int program);
    HDST_API
    virtual void UnbindResources(HdSt_ResourceBinder const &binder, int program);
    HDST_API
    virtual void AddBindings(HdBindingRequestVector *customBindings);
    HDST_API
    virtual ID ComputeHash() const;

    /// Setter method for prim
    HDST_API
    void SetFragmentSource(const std::string &source);
    HDST_API
    void SetGeometrySource(const std::string &source);
    HDST_API
    void SetParams(const HdMaterialParamVector &params);
    HDST_API
    void SetTextureDescriptors(const TextureDescriptorVector &texDesc);
    HDST_API
    void SetBufferSources(HdBufferSourceVector &bufferSources, 
                          HdResourceRegistrySharedPtr const &resourceRegistry);

    /// If the prim is based on asset, reload that asset.
    HDST_API
    virtual void Reload();

protected:
    HDST_API
    void _SetSource(TfToken const &shaderStageKey, std::string const &source);

private:
    std::string _fragmentSource;
    std::string _geometrySource;

    // Shader Parameters
    HdMaterialParamVector      _params;
    HdBufferSpecVector          _paramSpec;
    HdBufferArrayRangeSharedPtr _paramArray;

    TextureDescriptorVector _textureDescriptors;

    // No copying
    HdStSurfaceShader(const HdStSurfaceShader &)                     = delete;
    HdStSurfaceShader &operator =(const HdStSurfaceShader &)         = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HDST_SURFACESHADER_H
