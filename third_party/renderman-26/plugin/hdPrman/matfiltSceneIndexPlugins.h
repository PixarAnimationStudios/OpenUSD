//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MAT_FILT_SCENE_INDEX_PLUGINS_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MAT_FILT_SCENE_INDEX_PLUGINS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "pxr/imaging/hd/version.h"
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

#if HD_API_VERSION >= 76
/// ----------------------------------------------------------------------------
/// \class HdPrman_NodeIdentifierResolvingSceneIndexPlugin
///
/// Plugin that provides an HdSiNodeIdentifierResolvingSceneIndex. This 
/// finds shaders without nodeID's and attempts to resolve their identifier
/// via UsdShade sourceAsset or sourceCode properties.
///
/// This plugin is registered with the scene index plugin registry for Prman.
///
class HdPrman_NodeIdentifierResolvingSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdPrman_NodeIdentifierResolvingSceneIndexPlugin();

protected: // HdSceneIndexPlugin overrides
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr& inputScene,
        const HdContainerDataSourceHandle& inputArgs) override;
};
#endif

PXR_NAMESPACE_CLOSE_SCOPE

#endif //EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MAT_FILT_SCENE_INDEX_PLUGINS_H
