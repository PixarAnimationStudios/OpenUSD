//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef PXR_IMAGING_HDPRMAN_COORD_SYS_PRIM_SCENE_INDEX_PLUGIN_H
#define PXR_IMAGING_HDPRMAN_COORD_SYS_PRIM_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#if PXR_VERSION >= 2308

#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "hdPrman/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_CoordSysPrimSceneIndexPlugin
///
/// Plugin adds a scene index that adds coord system prims.
///
class HdPrman_CoordSysPrimSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdPrman_CoordSysPrimSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2308

#endif // PXR_IMAGING_HDPRMAN_COORD_SYS_PRIM_SCENE_INDEX_PLUGIN_H
