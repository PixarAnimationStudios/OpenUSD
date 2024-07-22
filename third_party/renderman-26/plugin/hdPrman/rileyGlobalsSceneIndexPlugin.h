//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_GLOBALS_SCENE_INDEX_PLUGIN_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_GLOBALS_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "hdPrman/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_RileyGlobalsSceneIndexPlugin
///
/// Prman scene index plugin that inspects, e.g., HdSceneGlobalsSchema to
/// add a riley:globals prim that calls Riley::SetOptions.
///
class HdPrman_RileyGlobalsSceneIndexPlugin :
    public HdSceneIndexPlugin
{
public:
    HdPrman_RileyGlobalsSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_GLOBALS_SCENE_INDEX_PLUGIN_H
