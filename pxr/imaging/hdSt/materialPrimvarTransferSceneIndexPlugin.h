//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_MATERIAL_PRIMVAR_TRANSFER_SCENE_INDEX_PLUGIN_H
#define PXR_IMAGING_HD_ST_MATERIAL_PRIMVAR_TRANSFER_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdSt_MaterialPrimvarTransferSceneIndexPlugin
///
/// Prman scene index plugin that transfers primvars/attributes
/// from materials to the geometry that binds the material.
///
class HdSt_MaterialPrimvarTransferSceneIndexPlugin :
    public HdSceneIndexPlugin
{
public:
    HdSt_MaterialPrimvarTransferSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_MATERIAL_PRIMVAR_TRANSFER_SCENE_INDEX_PLUGIN_H
