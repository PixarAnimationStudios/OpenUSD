//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_LIGHT_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_LIGHT_H

#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/token.h"

#include "pxr/pxr.h"

#include <Riley.h>
#include <RileyIds.h>
#include <RiTypesHelper.h>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdPrman_RenderParam;

/// \class HdPrmanLight
///
/// A representation for lights.
///
class HdPrmanLight final : public HdLight
{
public:
    HdPrmanLight(SdfPath const& id, TfToken const& lightType);
    ~HdPrmanLight() override;

    /// Synchronizes state from the delegate to this object.
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    void Finalize(HdRenderParam *renderParam) override;

private:

    const TfToken _hdLightType;
    riley::LightShaderId _shaderId;
    riley::LightInstanceId _instanceId;
    
    RtUString _lightShaderType;
    TfToken _lightLink;
    TfToken _shadowLink;
    SdfPathVector _lightFilterPaths;
    std::vector<TfToken> _lightFilterLinks;

    // state for mesh light change tracking
    riley::GeometryPrototypeId _geometryPrototypeId;
    SdfPath _sourceGeomPath;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_LIGHT_H
