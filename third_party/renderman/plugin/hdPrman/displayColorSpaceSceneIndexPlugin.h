//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DISPLAY_COLOR_SPACE_SCENE_INDEX_PLUGIN_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DISPLAY_COLOR_SPACE_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#if PXR_VERSION >= 2308

#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "hdPrman/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_DisplayColorSpaceSceneIndexPlugin
///
/// Prman scene index plugin that adds a display filter that converts the 
/// rendered pixels from the rendering color space to the display color space
/// to compensate for the transformation of the colors from lights and 
/// materials.
///
/// This is achieved by creating a Color Transform display filter prim as a
/// child of every RenderSettings prim and prepending it to the list of 
/// connected display filters connected.
///
class HdPrman_DisplayColorSpaceSceneIndexPlugin :
    public HdSceneIndexPlugin
{
public:
    HdPrman_DisplayColorSpaceSceneIndexPlugin();

    // Callback to allow for remapping of any custom color space names to ones
    // the ColorSpaceAPI and GfColorSpace can recognize. See GfColorSpace to 
    // see the Color Space names suppored by the ColorSpaceAPI and Gf.
    using ColorSpaceRemappingCallback = TfToken (*) (const TfToken& csName);
    static void
    RegisterColorSpaceRemappingCallback(
        const ColorSpaceRemappingCallback& callback);
    static TfToken RemapColorSpaceName(const TfToken& csName);

    // Callback should return true if the color space transformation applied by
    // this scene index should be disabled. Otherwise it should return false.
    using DisableTransformCallback = bool (*) (const HdSceneIndexBaseRefPtr &si);
    static void
    RegisterDisableTransformCallback(
        const DisableTransformCallback& callback);
    static bool DisableTransform(const HdSceneIndexBaseRefPtr& si);


protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2308

#endif //EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DISPLAY_COLOR_SPACE_SCENE_INDEX_PLUGIN_H
