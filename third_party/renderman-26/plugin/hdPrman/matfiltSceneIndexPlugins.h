//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MAT_FILT_SCENE_INDEX_PLUGINS_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MAT_FILT_SCENE_INDEX_PLUGINS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "hdPrman/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_PreviewMaterialFilteringSceneIndexPlugin
///
/// Plugin that provides a scene index that transforms the underlying material
/// network into Prman equivalents for each material prim that has a
/// UsdPreviewSurface node,
///
/// This plugin is registered with the scene index plugin registry for Prman.
///
class HdPrman_PreviewMaterialFilteringSceneIndexPlugin :
    public HdSceneIndexPlugin
{
public:
    HdPrman_PreviewMaterialFilteringSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

/// ----------------------------------------------------------------------------
/// \class HdPrman_MaterialXFilteringSceneIndexPlugin
///
/// Plugin that provides a scene index that transforms the underlying material
/// network into Prman equivalents for each material prim that has a
/// MaterialX node connected to the 'surface' terminal.
/// XXX: matFiltMaterialX.h doesn't seem to support other terminals
///      (displacmeent, volume)
///
/// This plugin is registered with the scene index plugin registry for Prman.
///
class HdPrman_MaterialXFilteringSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdPrman_MaterialXFilteringSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

/// ----------------------------------------------------------------------------
/// \class HdPrman_VirtualStructResolvingSceneIndexPlugin
///
/// Plugin that provides a scene index that expands "virtual struct"
/// connections, including evaluation of conditional actions.
///
/// This plugin is registered with the scene index plugin registry for Prman.
///
class HdPrman_VirtualStructResolvingSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdPrman_VirtualStructResolvingSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MAT_FILT_SCENE_INDEX_PLUGINS_H
