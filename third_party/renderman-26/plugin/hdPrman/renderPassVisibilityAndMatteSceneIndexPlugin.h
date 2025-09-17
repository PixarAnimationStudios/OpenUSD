//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_PASS_VISIBILITY_AND_MATTE_SCENE_INDEX_PLUGIN_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_PASS_VISIBILITY_AND_MATTE_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#if PXR_VERSION >= 2408
#include "pxr/imaging/hd/sceneIndexPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_RenderPassVisibilityAndMatteSceneIndexPlugin
///
/// Appends a scene index that applies visibility and matte rules of the active
/// render pass specified in the HdSceneGlobalsSchema.
///
/// \note This scene index assumes that the active render pass is a
///       UsdRenderPass for the purposes of collection naming conventions.
///
class HdPrman_RenderPassVisibilityAndMatteSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdPrman_RenderPassVisibilityAndMatteSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
#endif //PXR_VERSION >= 2408
