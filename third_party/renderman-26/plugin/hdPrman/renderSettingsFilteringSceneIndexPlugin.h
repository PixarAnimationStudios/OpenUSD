//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_SETTINGS_FILTERING_SCENE_INDEX_PLUGIN_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_SETTINGS_FILTERING_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#if PXR_VERSION >= 2308

#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "hdPrman/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_RenderSettingsFilteringSceneIndexPlugin
///
/// Prman scene index plugin that
/// * filters in settings properties based on conventions used in
///   UsdRiPxr-schemas auto-applied to RenderSettings prims.
/// * adds a fallback render settings prim (which will be used for scenes
///   that don't have one).
///
class HdPrman_RenderSettingsFilteringSceneIndexPlugin :
    public HdSceneIndexPlugin
{
public:
    HdPrman_RenderSettingsFilteringSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2308

#endif //EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_SETTINGS_FILTERING_SCENE_INDEX_PLUGIN_H
