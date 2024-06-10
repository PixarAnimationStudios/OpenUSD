//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_LIGHT_H
#define PXR_IMAGING_HD_LIGHT_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/sprim.h"

#include "pxr/base/tf/staticTokens.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

#define HD_LIGHT_TOKENS                                     \
    (angle)                                                 \
    (color)                                                 \
    (colorTemperature)                                      \
    (enableColorTemperature)                                \
    (domeOffset)                                            \
    (exposure)                                              \
    (height)                                                \
    (intensity)                                             \
    (radius)                                                \
    (length)                                                \
    ((textureFile, "texture:file"))                         \
    ((textureFormat, "texture:format"))                     \
    (width)                                                 \
    (ambient)                                               \
    (diffuse)                                               \
    (specular)                                              \
    (normalize)                                             \
    (hasShadow)                                             \
    ((shapingFocus, "shaping:focus"))                       \
    ((shapingFocusTint, "shaping:focusTint"))               \
    ((shapingConeAngle, "shaping:cone:angle"))              \
    ((shapingConeSoftness, "shaping:cone:softness"))        \
    ((shapingIesFile, "shaping:ies:file"))                  \
    ((shapingIesAngleScale, "shaping:ies:angleScale"))      \
    ((shapingIesNormalize, "shaping:ies:normalize"))        \
    ((shadowEnable, "shadow:enable"))                       \
    ((shadowColor, "shadow:color"))                         \
    ((shadowDistance, "shadow:distance"))                   \
    ((shadowFalloff, "shadow:falloff"))                     \
    ((shadowFalloffGamma, "shadow:falloffGamma"))           \
                                                            \
    (params)                                                \
    (shadowCollection)                                      \
    (shadowParams)

TF_DECLARE_PUBLIC_TOKENS(HdLightTokens, HD_API, HD_LIGHT_TOKENS);

class HdSceneDelegate;
using HdLightPtrConstVector = std::vector<class HdLight const *>;

/// \class HdLight
///
/// A light model, used in conjunction with HdRenderPass.
///
class HdLight : public HdSprim
{
public:
    HD_API
    HdLight(SdfPath const & id);
    HD_API
    ~HdLight() override;

    // Change tracking for HdLight
    enum DirtyBits : HdDirtyBits {
        Clean                 = 0,
        DirtyTransform        = 1 << 0,
        // Note: Because DirtyVisibility wasn't added, DirtyParams does double
        //       duty for params and visibility.
        DirtyParams           = 1 << 1,
        DirtyShadowParams     = 1 << 2,
        DirtyCollection       = 1 << 3,
        DirtyResource         = 1 << 4,

        // XXX: This flag is important for instanced lights, and must have
        // the same value as it does for Rprims
        DirtyInstancer        = 1 << 16,
        AllDirty              = (DirtyTransform
                                 |DirtyParams
                                 |DirtyShadowParams
                                 |DirtyCollection
                                 |DirtyResource
                                 |DirtyInstancer)
    };

    HD_API
    static std::string StringifyDirtyBits(HdDirtyBits dirtyBits);

    /// Returns the identifier of the instancer (if any) for this Sprim. If this
    /// Sprim is not instanced, an empty SdfPath will be returned.
    const SdfPath& GetInstancerId() const { return _instancerId; }

    HD_API
    void _UpdateInstancer(
        HdSceneDelegate* sceneDelegate,
        HdDirtyBits* dirtyBits);

private:
    SdfPath _instancerId;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_LIGHT_H
