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
#ifndef HD_SURFACESHADER_H
#define HD_SURFACESHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/shaderCode.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/value.h"

#include "pxr/imaging/garch/gl.h"
#include "pxr/base/tf/token.h"

#include <boost/shared_ptr.hpp>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class HdSceneDelegate;

typedef boost::shared_ptr<class HdBufferArrayRange> HdBufferArrayRangeSharedPtr;
typedef boost::shared_ptr<class HdTextureResource> HdTextureResourceSharedPtr;
typedef std::vector<HdTextureResourceSharedPtr> HdTextureResourceSharedPtrVector;
typedef boost::shared_ptr<class HdSurfaceShader> HdSurfaceShaderSharedPtr;

/// \class HdSurfaceShader
///
/// A scene-based SurfaceShader object.
///
/// When surface shaders are expresed in the scene graph, the HdSceneDelegate
/// can use this object to express these surface shaders in Hydra. In addition
/// to the shader itself, a binding from the Rprim to the SurfaceShader must be
/// expressed as well.
class HdSurfaceShader : public HdShaderCode {
public:
    HdSurfaceShader();
    virtual ~HdSurfaceShader();


    // ---------------------------------------------------------------------- //
    /// \name HdShader Virtual Interface                                      //
    // ---------------------------------------------------------------------- //
    virtual std::string GetSource(TfToken const &shaderStageKey) const;
    virtual HdShaderParamVector const& GetParams() const;
    virtual HdBufferArrayRangeSharedPtr const& GetShaderData() const;
    virtual TextureDescriptorVector GetTextures() const;
    virtual void BindResources(Hd_ResourceBinder const &binder, int program);
    virtual void UnbindResources(Hd_ResourceBinder const &binder, int program);
    virtual void AddBindings(HdBindingRequestVector *customBindings);
    virtual ID ComputeHash() const;

    /// Setter method for prim
    void SetFragmentSource(const std::string &source);
    void SetGeometrySource(const std::string &source);
    void SetParams(const HdShaderParamVector &params);
    void SetTextureDescriptors(const TextureDescriptorVector &texDesc);
    void SetBufferSources(HdBufferSourceVector &bufferSources);

    /// If the prim is based on asset, reload that asset.
    virtual void Reload();

protected:
    void _SetSource(TfToken const &shaderStageKey, std::string const &source);

private:
    std::string _fragmentSource;
    std::string _geometrySource;

    // Shader Parameters
    HdShaderParamVector         _params;
    HdBufferSpecVector          _paramSpec;
    HdBufferArrayRangeSharedPtr _paramArray;

    TextureDescriptorVector _textureDescriptors;

    // No copying
    HdSurfaceShader(const HdSurfaceShader &)                     = delete;
    HdSurfaceShader &operator =(const HdSurfaceShader &)         = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_SURFACESHADER_H
