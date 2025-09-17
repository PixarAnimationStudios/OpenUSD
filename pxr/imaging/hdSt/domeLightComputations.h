//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_DOME_LIGHT_COMPUTATIONS_H
#define PXR_IMAGING_HD_ST_DOME_LIGHT_COMPUTATIONS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/computation.h"

#include "pxr/base/gf/vec3i.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Dome Light texture computations GPU
///
///

using HdStSimpleLightingShaderPtr =
    std::weak_ptr<class HdStSimpleLightingShader>;

////
//// \class HdSt_DomeLightComputationGPU
///
/// Given a source texture at construction time, create a new texture from
/// its contents with the provided name using the corresponding shader source.
///
/// If the texture to be created has several mip levels, the texture
/// will only be created by the computation with level = 0 and the
/// computations with level > 0 will use the same texture.
///
class HdSt_DomeLightComputationGPU : public HdStComputation
{
public:
    /// Constructor
    HDST_API
    HdSt_DomeLightComputationGPU(
        // Name of computation shader to use, also used as texture name.
        const TfToken & shaderToken,
        // Whether to use the dome light cubemap texture or the dome light
        // latlong texture as the input for the computation.
        bool useCubemapAsSourceTexture,
        // Lighting shader for accessing any referenced textures.
        HdStSimpleLightingShaderPtr const &lightingShader,
        // Number of mip levels.
        unsigned int numLevels = 1,
        // Level to be filled (0 means also to allocate texture).
        unsigned int level = 0,
        // Roughness associated with the texture level.
        float roughness = -1.0);

    HDST_API
    void GetBufferSpecs(HdBufferSpecVector *specs) const override {}
   
    HDST_API
    void Execute(HdBufferArrayRangeSharedPtr const &range,
                 HdResourceRegistry *resourceRegistry) override;

    /// This computation doesn't generate buffer source (i.e. 2nd phase)
    /// This is a gpu computation, but no need to resize the destination
    /// since it belongs the same range as src buffer.
    int GetNumOutputElements() const override { return 0; }

private:
    const TfToken _shaderToken;
    const bool _useCubemapAsSourceTexture;
    const HdStSimpleLightingShaderPtr _lightingShader;
    const unsigned int _numLevels;
    const unsigned int _level;
    const float _roughness;
};

////
//// \class HdSt_DomeLightMipmapComputationGPU
///
/// Generate mipmaps for the provided dome light cubemap texture.
///
class HdSt_DomeLightMipmapComputationGPU : public HdStComputation
{
public:
    HDST_API
    HdSt_DomeLightMipmapComputationGPU(
        // Lighting shader for accessing textures.
        HdStSimpleLightingShaderPtr const &lightingShader);

    HDST_API
    void GetBufferSpecs(HdBufferSpecVector *specs) const override {}
   
    HDST_API
    void Execute(HdBufferArrayRangeSharedPtr const &range,
                 HdResourceRegistry *resourceRegistry) override;

    int GetNumOutputElements() const override { return 0; }

private:
    const HdStSimpleLightingShaderPtr _lightingShader;
};

/// Given the dimensions of a 2D dome light texture, compute the resulting
/// dimensions of the cubemap.
HDST_API
GfVec3i HdSt_ComputeDomeLightCubemapDimensions(
    const GfVec3i& srcDim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_DOME_LIGHT_COMPUTATIONS_H
