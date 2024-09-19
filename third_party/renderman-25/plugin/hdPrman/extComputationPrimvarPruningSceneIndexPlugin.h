//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_EXT_COMPUTATION_PRIMVAR_PRUNING_SCENE_INDEX_PLUGIN_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_EXT_COMPUTATION_PRIMVAR_PRUNING_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#if PXR_VERSION >= 2402

#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "hdPrman/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_ExtComputationPrimvarPruningSceneIndexPlugin
///
/// Prman scene index plugin that filters out computed primvars and presents
/// them as authored primvars.
///
class HdPrman_ExtComputationPrimvarPruningSceneIndexPlugin :
    public HdSceneIndexPlugin
{
public:
    HdPrman_ExtComputationPrimvarPruningSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2402

#endif //EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_EXT_COMPUTATION_PRIMVAR_PRUNING_SCENE_INDEX_PLUGIN_H
