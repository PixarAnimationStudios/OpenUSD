//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef LIBGAUSSIANSPLATSRENDERER_GAUSSIANSPLATSRENDERER_H
#define LIBGAUSSIANSPLATSRENDERER_GAUSSIANSPLATSRENDERER_H

#include "pxr/pxr.h"
#include "pxr/base/gf/quatf.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/gf/matrix4f.h"

#include "renderBuffer.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

struct GaussianSplats {
    using Ptr = std::shared_ptr<GaussianSplats>;
    static Ptr create() { return std::make_shared<GaussianSplats>(); }

    GfMatrix4f xform;
    VtVec3fArray positions;
    VtQuatfArray rotations;
    VtVec3fArray scales;
    VtFloatArray opacities;
    VtVec3fArray sphericalHarmonics;
    int primID;
    int sphericalHarmonicsDegree;
};

class GaussianSplatsRenderer {
  public:
    GaussianSplatsRenderer();
    ~GaussianSplatsRenderer();

    void setWorldToViewMatrix(const GfMatrix4f& m);
    void setProjMatrix(const GfMatrix4f& m);
    void addGaussianSplats(const std::string& splatName,
                           GaussianSplats::Ptr newSplats);
    void removeGaussianSplats(const std::string& splatName);
    bool renderGaussianSplatScene(
        HdParticleFieldRenderBuffer* colorRenderBuffer,
        HdParticleFieldRenderBuffer* depthRenderBuffer,
        HdParticleFieldRenderBuffer* primIDRenderBuffer) const;

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl{nullptr};

    /// Cannot copy.
    GaussianSplatsRenderer(const GaussianSplatsRenderer&)            = delete;
    GaussianSplatsRenderer& operator=(const GaussianSplatsRenderer&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // LIBGAUSSIANSPLATSRENDERER_GAUSSIANSPLATSRENDERER_H
