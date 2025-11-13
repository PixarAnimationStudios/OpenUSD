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

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_VelocityMotionResolvingSceneIndexPlugin
///
///
class HdPrman_VelocityMotionResolvingSceneIndexPlugin
  : public HdSceneIndexPlugin
{
public:
    HdPrman_VelocityMotionResolvingSceneIndexPlugin();

    static void
    SetFPS(float fps);

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr& inputScene,
        const HdContainerDataSourceHandle& inputArgs) override;
};

/// \class HdPrman_VblurInterpretingSceneIndexPlugin
///
class HdPrman_VblurInterpretingSceneIndexPlugin
  : public HdSceneIndexPlugin
{
public:
    HdPrman_VblurInterpretingSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr& inputScene,
        const HdContainerDataSourceHandle& inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VELOCITY_MOTION_RESOLVING_SCENE_INDEX_PLUGIN_H
