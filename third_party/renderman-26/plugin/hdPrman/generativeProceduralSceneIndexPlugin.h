//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_GENERATIVE_PROCEDURAL_INDEX_PLUGIN_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_GENERATIVE_PROCEDURAL_INDEX_PLUGIN_H

#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "hdPrman/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_GenerativeProceduralSceneIndexPlugin
/// 
class HdPrman_GenerativeProceduralSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    static const HdSceneIndexPluginRegistry::InsertionPhase
    GetInsertionPhase()
    {
        // XXX Until we have a better way to declare ordering/dependencies b/w
        //     scene index plugins, allow plugins to run before and after this
        //     plugin (i.e., don't use 0).
        return 2;
    }

    HdPrman_GenerativeProceduralSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
