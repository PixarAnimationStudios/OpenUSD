//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MAT_FILT_SCENE_INDEX_PLUGINS_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MAT_FILT_SCENE_INDEX_PLUGINS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/version.h"
#include "hdPrman/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// XXX The declarations below se can be moved to the cpp.
///
/// \class HdPrman_MatFiltSceneIndexPlugin
///
/// Plugin that chains a scene index for each of the following ops:
/// 1. Resolve connections: Expand "virtual struct" connections, including
///    evaluation of conditional actions.
/// 2. Node translation: Transform materials with a UsdPreviewSurface node or
///    MaterialX node into Prman equivalents.
///    XXX For MaterialX nodes, current support is limited to those connected to
///    the 'surface' terminal.
/// 3. Node identifier resolution: Find shaders without nodeID's and attempt to
///    resolve their identifier via UsdShade sourceAsset or sourceCode
///    properties.
///
/// This plugin is registered with the scene index plugin registry for each of
/// the Prman variants.
///
class HdPrman_MatFiltSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdPrman_MatFiltSceneIndexPlugin();

    // Returns the insertion phase to avoid hard-coding it in plugins that need
    // to go before or after material filtering.
    HDPRMAN_API
    static
    HdSceneIndexPluginRegistry::InsertionPhase GetInsertionPhase();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MAT_FILT_SCENE_INDEX_PLUGINS_H
