//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GLF_SIMPLE_LIGHTING_CONTEXT_H
#define PXR_IMAGING_GLF_SIMPLE_LIGHTING_CONTEXT_H

/// \file glf/simpleLightingContext.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleMaterial.h"
#include "pxr/imaging/glf/simpleShadowArray.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec4f.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/token.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_WEAK_AND_REF_PTRS(GlfBindingMap);
TF_DECLARE_WEAK_AND_REF_PTRS(GlfUniformBlock);
TF_DECLARE_WEAK_AND_REF_PTRS(GlfSimpleLightingContext);
TF_DECLARE_WEAK_AND_REF_PTRS(GlfSimpleShadowArray);

class GlfSimpleLightingContext : public TfRefBase, public TfWeakBase {
public:
    typedef GlfSimpleLightingContext This;

    GLF_API
    static GlfSimpleLightingContextRefPtr New();

    GLF_API
    void SetLights(GlfSimpleLightVector const & lights);
    GLF_API
    GlfSimpleLightVector const & GetLights() const;

    // returns the effective number of lights taken into account
    // in composable/compatible shader constraints
    GLF_API
    int GetNumLightsUsed() const;

    // returns the number of shadow maps needed, by summing shadow maps
    // allocated to each light.
    GLF_API
    int ComputeNumShadowsUsed() const;

    GLF_API
    void SetShadows(GlfSimpleShadowArrayRefPtr const & shadows);
    GLF_API
    GlfSimpleShadowArrayRefPtr const & GetShadows() const;

    GLF_API
    void SetMaterial(GlfSimpleMaterial const & material);
    GLF_API
    GlfSimpleMaterial const & GetMaterial() const;

    GLF_API
    void SetSceneAmbient(GfVec4f const & sceneAmbient);
    GLF_API
    GfVec4f const & GetSceneAmbient() const;

    GLF_API
    void SetCamera(GfMatrix4d const &worldToViewMatrix,
                   GfMatrix4d const &projectionMatrix);

    GLF_API
    void SetUseLighting(bool val);
    GLF_API
    bool GetUseLighting() const;

    // returns true if any light has shadow enabled.
    GLF_API
    bool GetUseShadows() const;

    GLF_API
    void SetUseColorMaterialDiffuse(bool val);
    GLF_API
    bool GetUseColorMaterialDiffuse() const;

    GLF_API
    void InitUniformBlockBindings(GlfBindingMapPtr const &bindingMap) const;
    GLF_API
    void InitSamplerUnitBindings(GlfBindingMapPtr const &bindingMap) const;

    GLF_API
    void BindUniformBlocks(GlfBindingMapPtr const &bindingMap);
    GLF_API
    void BindSamplers(GlfBindingMapPtr const &bindingMap);

    GLF_API
    void UnbindSamplers(GlfBindingMapPtr const &bindingMap);

    GLF_API
    void SetStateFromOpenGL();

    /// \name Post Surface Lighting
    ///
    /// This context can provide additional shader source, currently
    /// used to implement post surface lighting, along with a hash
    /// to help de-duplicate use by client shader programs.
    ///
    /// @{

    GLF_API
    size_t ComputeShaderSourceHash();

    GLF_API
    std::string const & ComputeShaderSource(TfToken const &shaderStageKey);

    /// @}

protected:
    GLF_API
    GlfSimpleLightingContext();
    GLF_API
    ~GlfSimpleLightingContext();

    void _ComputePostSurfaceShaderState();
    void _BindPostSurfaceShaderParams(GlfBindingMapPtr const &bindingMap);

private:
    GlfSimpleLightVector _lights;
    GlfSimpleShadowArrayRefPtr _shadows;

    GfMatrix4d _worldToViewMatrix;
    GfMatrix4d _projectionMatrix;

    GlfSimpleMaterial _material;
    GfVec4f _sceneAmbient;

    bool _useLighting;
    bool _useShadows;
    bool _useColorMaterialDiffuse;

    GlfUniformBlockRefPtr _lightingUniformBlock;
    GlfUniformBlockRefPtr _shadowUniformBlock;
    GlfUniformBlockRefPtr _materialUniformBlock;
    GlfUniformBlockRefPtr _bindlessShadowlUniformBlock;

    class _PostSurfaceShaderState;
    std::unique_ptr<_PostSurfaceShaderState> _postSurfaceShaderState;

    bool _lightingUniformBlockValid;
    bool _shadowUniformBlockValid;
    bool _materialUniformBlockValid;
    bool _postSurfaceShaderStateValid;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
