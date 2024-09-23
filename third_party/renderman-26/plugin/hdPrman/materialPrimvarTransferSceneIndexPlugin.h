//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATERIAL_PRIMVAR_TRANSFER_SCENE_INDEX_PLUGIN_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATERIAL_PRIMVAR_TRANSFER_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#if PXR_VERSION >= 2302

#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "hdPrman/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_MaterialPrimvarTransferSceneIndexPlugin
///
/// Prman scene index plugin that transfers primvars/attributes
/// from materials to the geometry that binds the material.
///
class HdPrman_MaterialPrimvarTransferSceneIndexPlugin :
    public HdSceneIndexPlugin
{
public:
    HdPrman_MaterialPrimvarTransferSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_VERSION >= 2302

#endif //EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATERIAL_PRIMVAR_TRANSFER_SCENE_INDEX_PLUGIN_H
