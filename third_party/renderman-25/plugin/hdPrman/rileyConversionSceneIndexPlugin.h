//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
////////////////////////////////////////////////////////////////////////

#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_CONVERSION_SCENE_INDEX_PLUGIN_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_CONVERSION_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "hdPrman/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_RileyConversionSceneIndexPlugin
///
/// Converts hydra prims to riley prims.
///
/// The implementation is incomplete and only active
/// HD_PRMAN_EXPERIMENTAL_RILEY_SCENE_INDEX_OBSERVER is set to true.
///
/// The limitations are as follows:
/// - it only converts spheres
/// - it always uses the fallback material
/// - it ignores instancers
///
/// An example: Given a sphere /Sphere, the conversion results in:
///
/// /Sphere, type: riley:geometryPrototype
/// /Sphere/RileyConversionGeometryInstance, type: riley:geometryInstance
///
class HdPrman_RileyConversionSceneIndexPlugin
                                  : public HdSceneIndexPlugin
{
public:
    HdPrman_RileyConversionSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_CONVERSION_SCENE_INDEX_PLUGIN_H

