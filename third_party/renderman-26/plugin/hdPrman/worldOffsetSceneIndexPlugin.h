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

class HdPrman_WorldOffsetSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdPrman_WorldOffsetSceneIndexPlugin();

    static void SetWorldOffset(const GfVec3d& worldOffset);
    static const GfVec3d& GetWorldOffset();

    static void SetCameraOffset(const GfVec3d& cameraOffset);
    static const GfVec3d& GetCameraOffset();

    static void SetRenderCamera(const SdfPath& renderCamera);
    static const SdfPath& GetRenderCamera();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr& inputScene,
        const HdContainerDataSourceHandle& inputArgs) override;

    static GfVec3d _worldOffset;
    static GfVec3d _cameraOffset;
    static SdfPath _renderCamera;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_WORLD_OFFSET_SCENE_INDEX_PLUGIN_H
