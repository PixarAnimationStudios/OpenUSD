//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDST_SIMPLE_LIGHTING_SHADER_H
#define PXR_IMAGING_HDST_SIMPLE_LIGHTING_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/binding.h"
#include "pxr/imaging/hdSt/lightingShader.h"

#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/base/tf/declarePtrs.h"

#include <memory>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderIndex;
class HdRenderParam;
class HdSceneDelegate;
class HdStRenderBuffer;
struct HdRenderPassAovBinding;
using HdStSimpleLightingShaderSharedPtr =
    std::shared_ptr<class HdStSimpleLightingShader>;
using HdRenderPassAovBindingVector = std::vector<HdRenderPassAovBinding>;

TF_DECLARE_REF_PTRS(GlfBindingMap);

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
                       HdSt_ResourceBinder const &binder) override;
    HDST_API
    void UnbindResources(int program,
                         HdSt_ResourceBinder const &binder) override;

    /// Add a custom binding request for use when this shader executes.
    HDST_API
    void AddBufferBinding(HdStBindingRequest const& req);

    /// Remove \p name from custom binding.
    HDST_API
    void RemoveBufferBinding(TfToken const &name);

    /// Clear all custom bindings associated with this shader.
    HDST_API
    void ClearBufferBindings();

    HDST_API
    void AddBindings(HdStBindingRequestVector *customBindings) override;

    /// Adds computations to create the dome light textures that
    /// are pre-calculated from the environment map texture.
    HDST_API
    void AddResourcesFromTextures(ResourceContext &ctx) const override;

    /// HdStShaderCode overrides
    HDST_API
    HdSt_MaterialParamVector const& GetParams() const override;

    /// HdStLightingShader overrides
    HDST_API
    void SetCamera(
        GfMatrix4d const &worldToViewMatrix,
        GfMatrix4d const &projectionMatrix) override;
    
    GlfSimpleLightingContextRefPtr GetLightingContext() const {
        return _lightingContext;
    };

    /// Allocates texture handles (texture loading happens later during commit)
    /// needed for lights.
    ///
    /// Call after lighting context has been set or updated in Sync-phase.
    ///
    HDST_API
    void AllocateTextureHandles(HdRenderIndex const &renderIndex);

    /// The dome light environment map used as source for the other
    /// dome light textures.
    const HdStTextureHandleSharedPtr &
    GetDomeLightEnvironmentTextureHandle() const {
        return _domeLightEnvironmentTextureHandle;
    }

    /// The textures computed from the dome light environment map that
    /// the shader needs to bind for the dome light shading.
    HDST_API
    NamedTextureHandleVector const &GetNamedTextureHandles() const override;

    /// Get one of the textures that need to be computed from the dome
    /// light environment map.
    HDST_API
    const HdStTextureHandleSharedPtr &GetTextureHandle(
        const TfToken &name) const;

    HdRenderPassAovBindingVector const& GetShadowAovBindings() {
        return _shadowAovBindings;
    }

private:
    SdfPath _GetAovPath(TfToken const &aov, size_t shadowIndex) const;
    void _ResizeOrCreateBufferForAov(size_t shadowIndex) const;
    void _CleanupAovBindings();

    GlfSimpleLightingContextRefPtr _lightingContext; 
    bool _useLighting;
    std::unique_ptr<class HioGlslfx> _glslfx;

    // Lexicographic ordering for stable output between runs.
    std::map<TfToken, HdStBindingRequest> _customBuffers;

    // The environment map used as source for the dome light textures.
    //
    // Handle is allocated in AllocateTextureHandles. Actual loading
    // happens during commit.
    HdStTextureHandleSharedPtr _domeLightEnvironmentTextureHandle;

    // Other dome light textures.
    NamedTextureHandleVector _namedTextureHandles;

    NamedTextureHandleVector _domeLightTextureHandles;
    NamedTextureHandle _shadowTextureHandle;
    
    HdSt_MaterialParamVector _lightTextureParams;

    HdRenderParam *_renderParam;

    HdRenderPassAovBindingVector _shadowAovBindings;
    std::vector<std::unique_ptr<HdStRenderBuffer>> _shadowAovBuffers;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDST_SIMPLE_LIGHTING_SHADER_H
