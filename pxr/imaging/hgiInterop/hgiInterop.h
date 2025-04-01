//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIINTEROP_HGIINTEROP_H
#define PXR_IMAGING_HGIINTEROP_HGIINTEROP_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/gf/rect2i.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hgiInterop/api.h"
#include "pxr/imaging/hgi/texture.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE
class Hgi;
class VtValue;

struct HgiInteropImpl;

/// Composition parameters. Provides a way to "merge"
/// the rendered content into existing framebuffer contents.
/// Default values disable composition and overwrite the framebuffer contents.
struct HgiInteropCompositionParams
{
    /// Alpha blending options if the destination supports alpha.
    HgiBlendFactor colorSrcBlendFactor{HgiBlendFactorOne};
    HgiBlendFactor colorDstBlendFactor{HgiBlendFactorZero};
    HgiBlendOp colorBlendOp{HgiBlendOpAdd};
    HgiBlendFactor alphaSrcBlendFactor{HgiBlendFactorOne};
    HgiBlendFactor alphaDstBlendFactor{HgiBlendFactorZero};
    HgiBlendOp alphaBlendOp{HgiBlendOpAdd};
    /// If a depth buffer is available in the destination,
    /// only copy pixels that pass the depth comparison.
    HgiCompareFunction depthFunc{HgiCompareFunctionAlways};
    /// Copy into a subregion of the destination framebuffer.
    GfRect2i dstRegion{};

    bool operator==(const HgiInteropCompositionParams& other) const
    {
        return colorSrcBlendFactor == other.colorSrcBlendFactor &&
            colorDstBlendFactor == other.colorDstBlendFactor &&
            colorBlendOp == other.colorBlendOp &&
            alphaSrcBlendFactor == other.alphaSrcBlendFactor &&
            alphaDstBlendFactor == other.alphaDstBlendFactor &&
            alphaBlendOp == other.alphaBlendOp &&
            depthFunc == other.depthFunc &&
            dstRegion == other.dstRegion;
    }

    bool operator!=(const HgiInteropCompositionParams& other) const
    {
        return !(*this == other);
    }
};

/// \class HgiInterop
///
/// Hydra Graphics Interface Interop.
///
/// HgiInterop provides functionality to transfer render targets between
/// supported APIs as efficiently as possible.
///
class HgiInterop final
{
public:
    HGIINTEROP_API
    HgiInterop();

    HGIINTEROP_API
    ~HgiInterop();

    /// Composite the provided textures over the application / viewer's
    /// framebuffer contents.
    /// `srcHgi`: 
    ///     Determines the source format/platform of the textures.
    ///     Eg. if hgi is of type HgiMetal, the textures are HgiMetalTexture.
    /// `srcColor`: is the source color aov texture to composite to screen.
    /// `srcDepth`: (optional) is the depth aov texture to composite to screen.
    /// `compositionParams`:
    ///     Parameters for compositing into the destination framebuffer.
    /// `dstFramebuffer`:
    ///     The framebuffer that the source textures are presented into.
    ///     An uint32_t (aka GLuint) FBO name. For backwards compatibility,
    ///     the currently bound framebuffer is used when the value is 0.
    HGIINTEROP_API
    void TransferToApp(
        Hgi *srcHgi,
        HgiTextureHandle const &srcColor,
        HgiTextureHandle const &srcDepth,
        HgiInteropCompositionParams const &compositionParams,
        uint32_t dstFramebuffer);

private:
    HgiInterop & operator=(const HgiInterop&) = delete;
    HgiInterop(const HgiInterop&) = delete;

    std::unique_ptr<HgiInteropImpl> _hgiInteropImpl;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
