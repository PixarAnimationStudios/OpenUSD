//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef PXR_IMAGING_HDPRMAN_IMPLICIT_SURFACE_SCENE_INDEX_PLUGIN_H
#define PXR_IMAGING_HDPRMAN_IMPLICIT_SURFACE_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#if PXR_VERSION >= 2208

#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "hdPrman/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_ImplicitSurfaceSceneIndexPlugin
///
/// Prman scene index plugin that configures the implicit surface scene index to
/// generate meshes for implicit surfaces that aren't natively supported by
/// Prman, and overload the transform (to account for different spine axes) for
/// natively supported quadrics.
///
class HdPrman_ImplicitSurfaceSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdPrman_ImplicitSurfaceSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2208

#endif // PXR_IMAGING_HDPRMAN_IMPLICIT_SURFACE_SCENE_INDEX_PLUGIN_H
