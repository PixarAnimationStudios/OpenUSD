//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DEPENDENCY_SCENE_INDEX_PLUGIN_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DEPENDENCY_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_DependencySceneIndexPlugin
///
/// Plugin adds a scene index that declares hdprman-specific dependencies to
/// trigger the necessary invalidation.
///
/// Currently, the lone usage is for volume prims.
///
/// Specfically, the declaration allows the volumeFieldBinding data source
/// locator of a volume prim to be invalidated if any of the targeted volume
/// fields changes.
/// That is, if, e.g., the filePath of a volume field changes, then the volume
/// using that volume field will be dirtied so that HdPrmanVolume will update
/// which 3d textures it will use.
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
