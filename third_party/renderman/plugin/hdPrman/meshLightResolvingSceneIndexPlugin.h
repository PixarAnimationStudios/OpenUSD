//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MESHLIGHT_RESOLVING_SCENE_INDEX_PLUGIN_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MESHLIGHT_RESOLVING_SCENE_INDEX_PLUGIN_H

#include "hdPrman/api.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceMaterialNetworkInterface.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "pxr/imaging/hd/materialNetworkInterface.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_MeshLghtResolvingSceneIndexPlugin
///
/// Experimental Prman-specific Hydra scene index to resolve mesh lights.
///
class HdPrman_MeshLightResolvingSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdPrman_MeshLightResolvingSceneIndexPlugin();

    using ExtractMaterialGlowConnectionCallback =
        HdMaterialNetworkInterface::InputConnectionResult (*) (
            const SdfPath& matPrimPath,
            const HdContainerDataSourceHandle& matPrimDS,
            const HdDataSourceMaterialNetworkInterface& matNI);

    HDPRMAN_API
    static void
    RegisterExtractMaterialGlowConnectionCallback(
        const ExtractMaterialGlowConnectionCallback& callback);

    HDPRMAN_API
    static HdMaterialNetworkInterface::InputConnectionResult
    ExtractMaterialGlowConnection(
        const SdfPath& matPrimPath,
        const HdContainerDataSourceHandle& matPrimDS,
        const HdDataSourceMaterialNetworkInterface& matNI);

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MESHLIGHT_RESOLVING_SCENE_INDEX_PLUGIN_H
