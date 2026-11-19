//
// Copyright 2026 Pixar
//
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RELATIONSHIP_PATH_VECTOR_CONVERTING_SCENE_INDEX_PLUGIN_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RELATIONSHIP_PATH_VECTOR_CONVERTING_SCENE_INDEX_PLUGIN_H

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "pxr/imaging/hd/version.h"


#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrmanRelationshipPathVectorConvertingSceneIndexPlugin
///
/// Converts incorrectly-typed unschematized path vector data sources drawn from
/// USD relationships from SdfPathVector to VtArray<SdfPath>. Beginning with
/// PXR_VERSION 2702 / USDIMAGING_API_VERSION 28, UsdImaging emits these
/// as VtArray<SdfPath>. When the hdPrman support matrix drops earlier USD
/// versions, this plugin can be removed.
///
class HdPrmanRelationshipPathVectorConvertingSceneIndexPlugin
  : public HdSceneIndexPlugin
{
public:
    HdPrmanRelationshipPathVectorConvertingSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr& inputScene,
        const HdContainerDataSourceHandle& inputArgs) override;
#if HD_API_VERSION >= 94
    bool _IsEnabled(
        const HdContainerDataSourceHandle &inputArgs) const override;
#endif
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RELATIONSHIP_PATH_VECTOR_CONVERTING_SCENE_INDEX_PLUGIN_H
