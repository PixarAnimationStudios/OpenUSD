//
// Copyright 2022 Pixar
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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MAT_FILT_SCENE_INDEX_PLUGINS_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MAT_FILT_SCENE_INDEX_PLUGINS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "hdPrman/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_PreviewMaterialFilteringSceneIndexPlugin
///
/// Plugin that provides a scene index that transforms the underlying material
/// network into Prman equivalents for each material prim that has a
/// UsdPreviewSurface node,
///
/// This plugin is registered with the scene index plugin registry for Prman.
///
class HdPrman_PreviewMaterialFilteringSceneIndexPlugin :
    public HdSceneIndexPlugin
{
public:
    HdPrman_PreviewMaterialFilteringSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

/// ----------------------------------------------------------------------------
/// \class HdPrman_MaterialXFilteringSceneIndexPlugin
///
/// Plugin that provides a scene index that transforms the underlying material
/// network into Prman equivalents for each material prim that has a
/// MaterialX node connected to the 'surface' terminal.
/// XXX: matFiltMaterialX.h doesn't seem to support other terminals
///      (displacmeent, volume)
///
/// This plugin is registered with the scene index plugin registry for Prman.
///
class HdPrman_MaterialXFilteringSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdPrman_MaterialXFilteringSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

/// ----------------------------------------------------------------------------
/// \class HdPrman_VirtualStructResolvingSceneIndexPlugin
///
/// Plugin that provides a scene index that expands "virtual struct"
/// connections, including evaluation of conditional actions.
///
/// This plugin is registered with the scene index plugin registry for Prman.
///
class HdPrman_VirtualStructResolvingSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdPrman_VirtualStructResolvingSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MAT_FILT_SCENE_INDEX_PLUGINS_H
