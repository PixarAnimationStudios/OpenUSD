//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VELOCITY_MOTION_RESOLVING_SCENE_INDEX_PLUGIN_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VELOCITY_MOTION_RESOLVING_SCENE_INDEX_PLUGIN_H

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_VelocityMotionResolvingSceneIndexPlugin
///
/// Plugin that resolves velocity-based motion accounting for the
/// ri:object:vblur attribute.
///
class HdPrman_VelocityMotionResolvingSceneIndexPlugin
  : public HdSceneIndexPlugin
{
public:
    static constexpr
    HdSceneIndexPluginRegistry::InsertionPhase
    GetInsertionPhase()
    {
        // After extcomp but before motion blur.
        return 2;
    }

    static void
    SetFPS(float fps);
    
    HdPrman_VelocityMotionResolvingSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr& inputScene,
        const HdContainerDataSourceHandle& inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VELOCITY_MOTION_RESOLVING_SCENE_INDEX_PLUGIN_H
