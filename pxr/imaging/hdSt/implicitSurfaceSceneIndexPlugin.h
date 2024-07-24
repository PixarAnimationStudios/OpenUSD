//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef PXR_IMAGING_HD_ST_IMPLICIT_SURFACE_SCENE_INDEX_PLUGIN_H
#define PXR_IMAGING_HD_ST_IMPLICIT_SURFACE_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdSt_ImplicitSurfaceSceneIndexPlugin
///
/// Storm scene index plugin that configures the implicit surface scene index
/// to generate meshes for various implicit surfaces.
/// \note Storm does _not_ natively support implicit geometry such
/// as spheres or cubes, so they need to be transformed into meshes.
///
class HdSt_ImplicitSurfaceSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdSt_ImplicitSurfaceSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_IMPLICIT_SURFACE_SCENE_INDEX_PLUGIN_H
