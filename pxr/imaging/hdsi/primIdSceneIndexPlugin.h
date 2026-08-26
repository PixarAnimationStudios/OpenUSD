//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_PRIM_ID_SCENE_INDEX_PLUGIN_H
#define PXR_IMAGING_HDSI_PRIM_ID_SCENE_INDEX_PLUGIN_H

#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "pxr/imaging/hdsi/api.h"
#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdsiPrimIdSceneIndexPlugin
///
/// If the env var HDSI_ENABLE_PRIM_ID_SCENE_INDEX is true, appends an
/// HdsiPrimIdSceneIndex as one of the last filtering scene indices for every
/// renderer, so that prim ids are assigned to the final set of renderable
/// prims.
///
class HdsiPrimIdSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HDSI_API
    HdsiPrimIdSceneIndexPlugin();

protected:
    HDSI_API
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDSI_PRIM_ID_SCENE_INDEX_PLUGIN_H
