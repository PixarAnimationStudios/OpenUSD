//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_PREVIEW_SURFACE_PRIMVARS_SCENE_INDEX_PLUGIN_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_PREVIEW_SURFACE_PRIMVARS_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"

// Version gated due to dependence on
// HdPrman_MaterialPrimvarTransferSceneIndexPlugin.
#if PXR_VERSION >= 2302

#include "pxr/imaging/hd/sceneIndexPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_PreviewSurfacePrimvarsSceneIndexPlugin
///
/// Prman scene index plugin that adds a primvar to materials if they are using 
/// a UsdPreviewSurface node in their material network. This primvar will get 
/// transferred to any gprims using the material in the 
/// HdsiMaterialPrimvarTransferSceneIndex. This primvar is needed to enable
/// correct displacement if the material is using displacement.
class HdPrman_PreviewSurfacePrimvarsSceneIndexPlugin :
    public HdSceneIndexPlugin
{
public:
    HdPrman_PreviewSurfacePrimvarsSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2302


#endif // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_PREVIEW_SURFACE_PRIMVARS_SCENE_INDEX_PLUGIN_H
