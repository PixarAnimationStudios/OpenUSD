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
/// Currently, the lone usage is for volume prims.
///
/// Specfically, the declaration allows the volumeFieldBinding data source
/// locator of a volume prim to be invalidated if any of the targeted volume 
/// fields changes.
/// That is, if, e.g., the filePath of a volume field changes, then the volume
/// using that volume field will be dirtied so that HdStVolume will update
/// which 3d textures it will use.
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
