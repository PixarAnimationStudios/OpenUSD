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
#ifndef PXR_IMAGING_HD_ST_SURFACE_SHADER_H
#define PXR_IMAGING_HD_ST_SURFACE_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hdSt/shaderCode.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/vt/value.h"
#include "pxr/base/tf/token.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class HdSceneDelegate;

using HdBufferArrayRangeSharedPtr = std::shared_ptr<class HdBufferArrayRange>;

using HdStSurfaceShaderSharedPtr = std::shared_ptr<class HdStSurfaceShader>;

using HdBufferSpecVector = std::vector<struct HdBufferSpec>;
using HdStResourceRegistrySharedPtr = 
    std::shared_ptr<class HdStResourceRegistry>;

/// \class HdStSurfaceShader
///
/// A scene-based SurfaceShader object.
///
/// When surface shaders are expresed in the scene graph, the HdSceneDelegate
/// can use this object to express these surface shaders in Storm. In addition
/// to the shader itself, a binding from the Rprim to the SurfaceShader must be
/// expressed as well.
class HdStSurfaceShader : public HdStShaderCode {
public:
    HDST_API
    HdStSurfaceShader();
    HDST_API
    ~HdStSurfaceShader() override;


    // ---------------------------------------------------------------------- //
    /// \name HdShader Virtual Interface                                      //
    // ---------------------------------------------------------------------- //
    HDST_API
    std::string GetSource(TfToken const &shaderStageKey) const override;
    HDST_API
    HdSt_MaterialParamVector const& GetParams() const override;
    HDST_API
    void SetEnabledPrimvarFiltering(bool enabled);
    HDST_API
    bool IsEnabledPrimvarFiltering() const override;
    HDST_API
    TfTokenVector const& GetPrimvarNames() const override;
    HDST_API
    HdBufferArrayRangeSharedPtr const& GetShaderData() const override;
    HDST_API
    NamedTextureHandleVector const & GetNamedTextureHandles() const override;
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
    HDST_API
    ID ComputeHash() const override;

    HDST_API
    ID ComputeTextureSourceHash() const override;

    HDST_API
    TfToken GetMaterialTag() const override;

    /// Setter method for prim
    HDST_API
    void SetFragmentSource(const std::string &source);
    HDST_API
    void SetGeometrySource(const std::string &source);
    HDST_API
    void SetParams(const HdSt_MaterialParamVector &params);
    HDST_API
    void SetNamedTextureHandles(const NamedTextureHandleVector &);
    HDST_API
    void SetBufferSources(
        HdBufferSpecVector const &bufferSpecs,
        HdBufferSourceSharedPtrVector &&bufferSources, 
        HdStResourceRegistrySharedPtr const &resourceRegistry);

    /// Called after textures have been committed.
    ///
    /// Shader can return buffer sources for different BARs (most
    /// likely, the shader bar) that require texture metadata such as
    /// the bindless texture handle which is only available after the
    /// commit.
    ///
    HDST_API
    void AddResourcesFromTextures(ResourceContext &ctx) const override;

    HDST_API
    void SetMaterialTag(TfToken const &materialTag);

    /// If the prim is based on asset, reload that asset.
    HDST_API
    virtual void Reload();

    /// Returns if the two shaders can be aggregated into the same draw batch.
    HDST_API
    static bool CanAggregate(HdStShaderCodeSharedPtr const &shaderA,
                             HdStShaderCodeSharedPtr const &shaderB);

    /// Adds the fallback value of the given material param to
    /// buffer specs and sources using the param's name.
    ///
    HDST_API
    static void AddFallbackValueToSpecsAndSources(
        const HdSt_MaterialParam &param,
        HdBufferSpecVector * const specs,
        HdBufferSourceSharedPtrVector * const sources);

protected:
    HDST_API
    void _SetSource(TfToken const &shaderStageKey, std::string const &source);

    HDST_API
    ID _ComputeHash() const;

    HDST_API
    ID _ComputeTextureSourceHash() const;

private:
    std::string _fragmentSource;
    std::string _geometrySource;

    // Shader Parameters
    HdSt_MaterialParamVector       _params;
    HdBufferSpecVector          _paramSpec;
    HdBufferArrayRangeSharedPtr _paramArray;
    TfTokenVector               _primvarNames;
    bool                        _isEnabledPrimvarFiltering;

    mutable size_t              _computedHash;
    mutable bool                _isValidComputedHash;

    mutable size_t              _computedTextureSourceHash;
    mutable bool                _isValidComputedTextureSourceHash;

    NamedTextureHandleVector _namedTextureHandles;

    TfToken _materialTag;

    // No copying
    HdStSurfaceShader(const HdStSurfaceShader &)                     = delete;
    HdStSurfaceShader &operator =(const HdStSurfaceShader &)         = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_SURFACE_SHADER_H
