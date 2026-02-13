//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_WORLD_OFFSET_SCENE_INDEX_PLUGIN_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_WORLD_OFFSET_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

/// HdPrman_WorldOffsetSceneIndexPlugin implements the RenderMan options
/// trace:worldorigin and trace:worldoffset.  If worldorigin is set to
/// "camera", the trace:worldoffset is overridden with the position of
/// the primary scene camera.  The trace:worldoffset value is applied
/// as an additional translation to all geometry passed into Riley.
///
/// RenderMan will internally remove this extra translation when evaluating
/// shading in "world" space and when writing out AOV results in "world"
/// space.  However, it still requires the scene generator to apply
/// this offset itself when submitting xforms via Riley API, so we
/// handle it that in this scene index.
///
class HdPrman_WorldOffsetSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdPrman_WorldOffsetSceneIndexPlugin();

#if PXR_VERSION < 2505
    static void SetWorldOffset(const GfVec3d& worldOffset);
    static const GfVec3d& GetWorldOffset();

    static void SetCameraOffset(const GfVec3d& cameraOffset);
    static const GfVec3d& GetCameraOffset();

    static void SetRenderCamera(const SdfPath& renderCamera);
    static const SdfPath& GetRenderCamera();
#endif

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr& inputScene,
        const HdContainerDataSourceHandle& inputArgs) override;

#if PXR_VERSION < 2505
    static GfVec3d _worldOffset;
    static GfVec3d _cameraOffset;
    static SdfPath _renderCamera;
#endif
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_WORLD_OFFSET_SCENE_INDEX_PLUGIN_H
