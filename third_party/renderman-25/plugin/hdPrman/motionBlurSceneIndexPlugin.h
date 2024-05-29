//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MOTION_BLUR_SCENE_INDEX_PLUGIN_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MOTION_BLUR_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_MotionBlurSceneIndexPlugin
/// 
/// Plugin provides a scene index that interprets and reshapes the upstream
/// scene for motion blur according to Prman's capabilities. This scene index
/// plugin handles all types of motion blur, including:
///  * transform motion blur, achieved by animating a prim's xform property or
///    by animating individual instance positions, orientations, scales, or
///    transforms,
///  * deformation motion blur, achieved by animating a points-based prim's
///    points primvar, and
///  * velocity motion blur, achieved by providing velocities, angular
///    velocities, and accelerations for a points-based or point instancer prim.
/// This plugin is exclusively responsible for reshaping data sources for motion
/// blur. Downstream consumers do not need to consider whether motion blur is
/// enabled or any other details of whether or how motion blur should be
/// applied.
///
/// Note that the fps (needed because the shutter offset is in frames and
/// the velocity in length/second) is hard-coded to 24.0.
///
/// The plugin is registered with the scene index plugin registry for Prman.
///
class HdPrman_MotionBlurSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdPrman_MotionBlurSceneIndexPlugin();

    static void SetShutterInterval(float shutterOpen, float shutterClose);

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MOTION_BLUR_SCENE_INDEX_PLUGIN_H
