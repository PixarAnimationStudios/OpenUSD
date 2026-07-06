//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef PXR_IMAGING_HD_ST_RENDER_PASS_VISIBILITY_SCENE_INDEX_PLUGIN_H
#define PXR_IMAGING_HD_ST_RENDER_PASS_VISIBILITY_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#if PXR_VERSION >= 2408
#include "pxr/imaging/hd/sceneIndexPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdSt_RenderPassVisibilitySceneIndexPlugin
///
/// Appends a scene index that applies render visibility rules of the active
/// render pass specified in the HdSceneGlobalsSchema.
///
/// For instancers with instancerTopology/instanceLocations, the collections
/// will be evaluated against the instanceLocations, and instance-rate
/// primvars will be set as appropriate.  For the render visibility collection,
/// this will modify the instancerTopology/mask value rather than a primvar.
///
/// \note This scene index assumes that the active render pass is a
///       UsdRenderPass for the purposes of collection naming conventions.
///
class HdSt_RenderPassVisibilitySceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdSt_RenderPassVisibilitySceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
#endif //PXR_VERSION >= 2408
