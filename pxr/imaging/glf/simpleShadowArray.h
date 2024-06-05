//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GLF_SIMPLE_SHADOW_ARRAY_H
#define PXR_IMAGING_GLF_SIMPLE_SHADOW_ARRAY_H

/// \file glf/simpleShadowArray.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/imaging/garch/glApi.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class GlfSimpleShadowArray : public TfRefBase,
                             public TfWeakBase
{
public:
    GLF_API
    GlfSimpleShadowArray();
    GLF_API
    ~GlfSimpleShadowArray() override;

    // Disallow copies
    GlfSimpleShadowArray(const GlfSimpleShadowArray&) = delete;
    GlfSimpleShadowArray& operator=(const GlfSimpleShadowArray&) = delete;

    // Returns the GL texture id of the shadow texture.
    GLF_API
    GLuint GetShadowMapTexture(int shadowIndex) const;

    // Returns the GL sampler id of the sampler object used to read the raw
    // depth values.
    GLF_API
    GLuint GetShadowMapDepthSampler() const;

    // Returns the GL sampler id of the sampler object used for depth comparison
    GLF_API
    GLuint GetShadowMapCompareSampler() const;

    // Set the resolutions of all the shadow maps necessary. The number of
    // resolutions corresponds to the number of shadow map textures necessary,
    // which is currently one per shadow casting light.
    GLF_API
    void SetShadowMapResolutions(std::vector<GfVec2i> const& resolutions);

    // Returns the number of shadow map generation passes required, which is
    // currently one per shadow map (corresponding to a shadow casting light).
    GLF_API
    size_t GetNumShadowMapPasses() const;
    
    // Returns the shadow map resolution for a given pass.
    // this returns the resolution of the corresponding shadow map,
    GLF_API
    GfVec2i GetShadowMapSize(size_t pass) const;

    // Get/Set the view (world to shadow camera) transform to use for a given 
    // shadow map generation pass.
    GLF_API
    GfMatrix4d GetViewMatrix(size_t index) const;
    GLF_API
    void SetViewMatrix(size_t index, GfMatrix4d const & matrix);

    // Get/Set the projection transform to use for a given shadow map generation
    // pass.
    GLF_API
    GfMatrix4d GetProjectionMatrix(size_t index) const;
    GLF_API
    void SetProjectionMatrix(size_t index, GfMatrix4d const & matrix);

    GLF_API
    GfMatrix4d GetWorldToShadowMatrix(size_t index) const;

    // Bind necessary resources for a given shadow map generation pass.
    GLF_API
    void BeginCapture(size_t index, bool clear);
    
    // Unbind necssary resources after a shadow map gneration pass.
    GLF_API
    void EndCapture(size_t index);

    // Sets the GL texture ids of the shadow textures, as opposed to creating
    // them internally with _AllocTextures().
    GLF_API
    void SetTextures(std::vector<GLuint> textureIds);

    // Allocates the shadow samplers.
    GLF_API
    void AllocSamplers();

private:
    void _AllocResources();
    void _AllocTextures();
    void _FreeResources();
    void _FreeTextures();
    bool _ShadowMapExists() const;
    void _BindFramebuffer(size_t index);
    void _UnbindFramebuffer();

private:
    std::vector<GfVec2i> _resolutions;
    std::vector<GLuint> _textures;

    std::vector<GfMatrix4d> _viewMatrix;
    std::vector<GfMatrix4d> _projectionMatrix;

    GLuint _framebuffer;

    GLuint _shadowDepthSampler;
    GLuint _shadowCompareSampler;

    GLuint _unbindRestoreDrawFramebuffer;
    GLuint _unbindRestoreReadFramebuffer;

    GLint  _unbindRestoreViewport[4];

    bool _texturesAllocatedExternally;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
