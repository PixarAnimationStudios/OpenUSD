//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DEPENDENCY_SCENE_INDEX_PLUGIN_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DEPENDENCY_SCENE_INDEX_PLUGIN_H

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_DependencySceneIndexPlugin
///
/// Plugin adds a scene index that declares hdprman-specific dependencies to
/// trigger the necessary invalidation.
///
/// It currently declares 2 dependencies:
/// (a -> b to be read as "a depends on b")
/// 1. Volume -> Volume Field Binding
/// This declaration registers the volumeFieldBinding data source
/// locator of a volume prim to be invalidated if any of the targeted volume
/// fields changes.
/// That is, if, e.g., the filePath of a volume field changes, then the volume
/// using that volume field will be dirtied so that HdPrmanVolume will update
/// which 3d textures it will use.
///
/// 2. Light -> Light Filter
/// This declaration registers the light data source locator of a light prim
/// to be invalidated if any locator of a targeted light filter changes.
///
class HdPrman_DependencySceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdPrman_DependencySceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DEPENDENCY_SCENE_INDEX_PLUGIN_H
