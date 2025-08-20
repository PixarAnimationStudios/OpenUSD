//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef PXR_IMAGING_HD_ST_RENDER_PASS_PRUNE_SCENE_INDEX_PLUGIN_H
#define PXR_IMAGING_HD_ST_RENDER_PASS_PRUNE_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#if PXR_VERSION >= 2408
#include "pxr/imaging/hd/sceneIndexPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdSt_RenderPassPruneSceneIndexPlugin
///
/// Appends the HdsiRenderPassPruneSceneIndex.
///
class HdSt_RenderPassPruneSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdSt_RenderPassPruneSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
#endif //PXR_VERSION >= 2408
