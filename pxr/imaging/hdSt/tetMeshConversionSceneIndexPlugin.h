//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef PXR_IMAGING_HD_ST_TET_MESH_CONVERSION_SCENE_INDEX_PLUGIN_H
#define PXR_IMAGING_HD_ST_TET_MESH_CONVERSION_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdSt_TetMeshConversionSceneIndexPlugin
///
/// Storm scene index plugin that configures the Tet Mesh Conversion scene index
/// to generate meshes for Tet Meshes.
/// \note Storm does _not_ natively support Tet Meshes, so they need to
/// be transformed into meshes.
///
class HdSt_TetMeshConversionSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdSt_TetMeshConversionSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_TET_MESH_CONVERSION_SCENE_INDEX_PLUGIN_H
