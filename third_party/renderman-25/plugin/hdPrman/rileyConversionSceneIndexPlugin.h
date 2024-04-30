//
// Copyright 2024 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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

