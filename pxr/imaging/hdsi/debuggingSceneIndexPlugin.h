//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_DEBUGGING_SCENE_INDEX_PLUGIN_H
#define PXR_IMAGING_HDSI_DEBUGGING_SCENE_INDEX_PLUGIN_H

#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "pxr/imaging/hdsi/api.h"
#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdsiDebuggingSceneIndexPlugin
///
/// Registers debugging scene index if env var
/// HDSI_DEBUGGING_SCENE_INDEX_INSERTION_PHASE is an integer.
///
class HdsiDebuggingSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HDSI_API
    HdsiDebuggingSceneIndexPlugin();

protected:
    HDSI_API
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_DEBUGGING_SCENE_INDEX_PLUGIN_H
