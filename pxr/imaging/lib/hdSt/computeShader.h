//
// Copyright 2017 Pixar
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
#ifndef HD_COMPUTEHADER_H
#define HD_COMPUTEHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/shaderCode.h"
#include "pxr/imaging/hd/bufferSource.h"

#include "pxr/imaging/garch/gl.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/vt/value.h"
#include "pxr/base/tf/token.h"

#include <boost/shared_ptr.hpp>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

typedef boost::shared_ptr<class HdStComputeShader> HdStComputeShaderSharedPtr;

/// \class HdStComputeShader
///
/// A scene-based ComputeShader object.
///
/// When compute shaders are expressed in the scene graph, the HdSceneDelegate
/// can use this object to express these compute shaders in Hydra.
/// In addition to the shader itself, a binding from the Computation Sprim
/// to the ComputeShader must be expressed as well.
class HdStComputeShader : public HdShaderCode {
public:
    HD_API
    HdStComputeShader();
    HD_API
    virtual ~HdStComputeShader();


    // ---------------------------------------------------------------------- //
    /// \name HdShader Virtual Interface                                      //
    // ---------------------------------------------------------------------- //
    HD_API
    virtual std::string GetSource(TfToken const &shaderStageKey) const;
    HD_API
    virtual HdMaterialParamVector const& GetParams() const;
    HD_API
    virtual HdBufferArrayRangeSharedPtr const& GetShaderData() const;
    HD_API
    virtual TextureDescriptorVector GetTextures() const;
    HD_API
    virtual void BindResources(Hd_ResourceBinder const &binder, int program);
    HD_API
    virtual void UnbindResources(Hd_ResourceBinder const &binder, int program);
    HD_API
    virtual void AddBindings(HdBindingRequestVector *customBindings);
    HD_API
    virtual ID ComputeHash() const;

    /// Setter method for prim
    HD_API
    void SetComputeSource(const std::string &source);

    /// If the prim is based on asset, reload that asset.
    HD_API
    virtual void Reload();

protected:
    HD_API
    void _SetSource(TfToken const &shaderStageKey, std::string const &source);

private:
    std::string _computeSource;

    // Shader Parameters
    HdMaterialParamVector       _params;
    HdBufferSpecVector          _paramSpec;
    HdBufferArrayRangeSharedPtr _paramArray;

    TextureDescriptorVector _textureDescriptors;
    
    // No copying
    HdStComputeShader(const HdStComputeShader &)                     = delete;
    HdStComputeShader &operator =(const HdStComputeShader &)         = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_SURFACESHADER_H
