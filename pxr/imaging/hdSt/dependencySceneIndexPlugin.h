//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef PXR_IMAGING_HD_ST_DEPENDENCY_SCENE_INDEX_PLUGIN_H
#define PXR_IMAGING_HD_ST_DEPENDENCY_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdSt_DependencySceneIndexPlugin
///
/// Plugin adds a scene index that declares Storm-specific dependencies to
/// trigger the necessary invalidation.
///
/// Currently, the scene index has two uses.
///
/// 1) For volumes.
/// Specfically, the declaration allows the volumeFieldBinding data source
/// locator of a volume prim to be invalidated if any of the targeted volume 
/// fields changes.
/// That is, if, e.g., the filePath of a volume field changes, then the volume
/// using that volume field will be dirtied so that HdStVolume will update
/// which 3d textures it will use.
///
/// 2) For meshes.
/// Adding dependency of the material binding on the material datasource
/// of the bound material.
/// Recall that a mesh has to be quadrangulated if the bound material is using
/// any ptex texture. If there is any change to the material, this scene index
/// will dirty the mesh's materialBindings locator. This causes a
/// HdStMesh::Sync with the HdChangeTracker::DirtyMaterialId dirty bit set so
/// the mesh will re-evaluate whether the bound material is using any ptex
/// texture.
///
class HdSt_DependencySceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdSt_DependencySceneIndexPlugin();    

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_DEPENDENCY_SCENE_INDEX_PLUGIN_H
