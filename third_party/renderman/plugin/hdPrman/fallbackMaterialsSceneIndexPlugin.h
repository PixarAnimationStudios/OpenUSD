//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_FALLBACK_MATERIALS_SCENE_INDEX_PLUGIN_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_FALLBACK_MATERIALS_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#if PXR_VERSION >= 2302

#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "hdPrman/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_FallbackMaterialsSceneIndexPlugin
///
/// This plugin defines a fallback material network under the /__HdPrman
/// scene index scope.  Additionally, it underlays a binding to this
/// material, providing it as a fallback binding.
///
class HdPrman_FallbackMaterialsSceneIndexPlugin :
    public HdSceneIndexPlugin
{
public:
    HdPrman_FallbackMaterialsSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2302

#endif // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_FALLBACK_MATERIALS_SCENE_INDEX_PLUGIN_H
