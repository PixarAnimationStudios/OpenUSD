//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDERING_COLOR_SPACE_SCENE_INDEX_PLUGIN_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDERING_COLOR_SPACE_SCENE_INDEX_PLUGIN_H

#include "pxr/imaging/hd/materialNetworkInterface.h"
#include "pxr/base/gf/colorSpace.h"
#include "pxr/base/tf/token.h"
#include "pxr/pxr.h"
#if PXR_VERSION >= 2308

#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "hdPrman/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_RenderingColorSpaceSceneIndexPlugin
///
/// Prman scene index plugin that converts authored color attributes on
/// materials and lights from the source color space into the rendering color
/// space.
///
class HdPrman_RenderingColorSpaceSceneIndexPlugin :
    public HdSceneIndexPlugin
{
public:
    HdPrman_RenderingColorSpaceSceneIndexPlugin();

    // Callback to allow for remapping of any custom color space names to ones
    // the ColorSpaceAPI and GfColorSpace can recognize. See GfColorSpace to 
    // see the Color Space names suppored by the ColorSpaceAPI and Gf.
    using ColorSpaceRemappingCallback = TfToken (*) (const TfToken& csName);
    static void
    RegisterColorSpaceRemappingCallback(
        const ColorSpaceRemappingCallback& callback);
    static TfToken RemapColorSpaceName(const TfToken& csName);

    // Callback should return true if the material should be excluded from 
    // the Color Space transformations. Otherwise it should return false.
    using ExcludeMaterialCallback = bool (*)
        (const HdSceneIndexBaseRefPtr &si,
        HdMaterialNetworkInterface* interface);
    static void
    RegisterExcludeMaterialCallback(const ExcludeMaterialCallback& callback);
    static bool ExcludeMaterial(
        const HdSceneIndexBaseRefPtr &si,
        HdMaterialNetworkInterface* interface);

    // Callback to add any custom material filtering. It should return true if
    // the material network interface has been modified and should no longer 
    // be modified. Otherwise it should return false.
    using FilterMaterialCallback = bool (*) (
        HdMaterialNetworkInterface* interface,
        const TfToken& terminalNodeName,
        const GfColorSpace& renderingCS);
    static void
    RegisterFilterMaterialCallback(const FilterMaterialCallback& callback);
    static bool
    FilterMaterial(
        HdMaterialNetworkInterface* interface,
        const TfToken& terminalNodeName,
        const GfColorSpace& renderingColorSpace);

    // Callback to add any custom light filtering. It should return true if
    // the material network interface has been modified and should no longer 
    // be modified. Otherwise it should return false.
    using FilterLightCallback = bool (*) (
        HdMaterialNetworkInterface* interface,
        const SdfPath& primPath,
        const TfToken& nodeName,
        const GfColorSpace& renderingCS);
    static void
    RegisterFilterLightCallback(const FilterLightCallback& callback);
    static bool
    FilterLight(
        HdMaterialNetworkInterface* interface,
        const SdfPath& primPath,
        const TfToken& nodeName,
        const GfColorSpace& renderingColorSpace);

    // Callback should return true if the color space transformation applied by
    // this scene index should be disabled. Otherwise it should return false.
    using DisableTransformCallback = bool (*) (const HdSceneIndexBaseRefPtr &si);
    static void
    RegisterDisableTransformCallback(const DisableTransformCallback& callback);
    static bool DisableTransform(const HdSceneIndexBaseRefPtr& si);


protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2308

#endif //EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDERING_COLOR_SPACE_SCENE_INDEX_PLUGIN_H
