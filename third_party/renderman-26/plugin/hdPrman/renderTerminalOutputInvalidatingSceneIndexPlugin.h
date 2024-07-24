//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef PXR_IMAGING_HDPRMAN_RENDER_TERMINAL_OUTPUT_INVALIDATING_SCENE_INDEX_PLUGIN_H
#define PXR_IMAGING_HDPRMAN_RENDER_TERMINAL_OUTPUT_INVALIDATING_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "hdPrman/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_RenderTerminalOutputInvalidatingSceneIndexPlugin
///
/// Plugin adds a filtering scene index to the Prman render delegate to
/// dirty the Integrator, Sample and Display Filters connected to the 
/// Render Settings Prim when changed.
///
class HdPrman_RenderTerminalOutputInvalidatingSceneIndexPlugin
    : public HdSceneIndexPlugin
{
public:
    HdPrman_RenderTerminalOutputInvalidatingSceneIndexPlugin();    

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDPRMAN_RENDER_TERMINAL_OUTPUT_INVALIDATING_SCENE_INDEX_PLUGIN_H
