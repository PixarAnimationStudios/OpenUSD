//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_RENDER_PASS_SHADER_H
#define PXR_IMAGING_HD_ST_RENDER_PASS_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/binding.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hio/glslfx.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/token.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

using HdStRenderPassShaderSharedPtr =
    std::shared_ptr<class HdStRenderPassShader>;
class HdStRenderPassState;
class HdRenderIndex;
using HdRenderPassAovBindingVector =
    std::vector<struct HdRenderPassAovBinding>;

/// \class HdStRenderPassShader
///
/// A shader that supports common renderPass functionality.
///
class HdStRenderPassShader : public HdStShaderCode {
public:
    HDST_API
    HdStRenderPassShader();
    HDST_API
    HdStRenderPassShader(TfToken const &glslfxFile);
    HDST_API
    HdStRenderPassShader(HioGlslfxSharedPtr const &glslfx);
    HDST_API
    ~HdStRenderPassShader() override;

    /// HdShader overrides
    HDST_API
    ID ComputeHash() const override;
    HDST_API
    std::string GetSource(TfToken const &shaderStageKey) const override;
    HDST_API
    void BindResources(int program,
                       HdSt_ResourceBinder const &binder) override;
    HDST_API
    void UnbindResources(int program,
                         HdSt_ResourceBinder const &binder) override;
    HDST_API
    void AddBindings(HdStBindingRequestVector *customBindings) override;
    HDST_API
    HdSt_MaterialParamVector const& GetParams() const override;

    HDST_API
    NamedTextureHandleVector const & GetNamedTextureHandles() const override;

    /// Add a custom binding request for use when this shader executes.
    HDST_API
    void AddBufferBinding(HdStBindingRequest const& req);

    /// Remove \p name from custom binding.
    HDST_API
    void RemoveBufferBinding(TfToken const &name);

    /// Clear all custom bindings associated with this shader.
    HDST_API
    void ClearBufferBindings();

    // Sets the textures and params such that the shader can access
    // the requested aovs with HdGet_AOVNAMEReadback().
    //
    // Needs to be called in task prepare or sync since it is
    // allocating texture handles.
    //
    HDST_API
    void UpdateAovInputTextures(
        HdRenderPassAovBindingVector const &aovInputBindings,
        HdRenderIndex * const renderIndex);

    HDST_API
    static
    HdStRenderPassShaderSharedPtr
    CreateRenderPassShaderFromAovs(
        HdStRenderPassState *renderPassState,
        HdStResourceRegistrySharedPtr const &resourceRegistry,
        HdRenderPassAovBindingVector const &aovBindings);

private:
    HioGlslfxSharedPtr _glslfx;
    mutable size_t  _hash;
    mutable bool    _hashValid;

    // Lexicographic ordering for stable output between runs.
    std::map<TfToken, HdStBindingRequest> _customBuffers;

    NamedTextureHandleVector _namedTextureHandles;

    HdSt_MaterialParamVector _params;

    // No copying
    HdStRenderPassShader(const HdStRenderPassShader &)                     = delete;
    HdStRenderPassShader &operator =(const HdStRenderPassShader &)         = delete;

    HioGlslfx const * _GetGlslfx() const override;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_RENDER_PASS_SHADER_H
