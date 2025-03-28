//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef PXR_IMAGING_HDPRMAN_DEPENDENCY_FORWARDING_SCENE_INDEX_PLUGIN_H
#define PXR_IMAGING_HDPRMAN_DEPENDENCY_FORWARDING_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#if PXR_VERSION >= 2208

#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "hdPrman/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_DependencyForwardingSceneIndexPlugin
///
/// Plugin adds a dependency forwarding scene index to the Prman render
/// delegate to resolve any dependencies introduced by other scene indices.
///
class HdPrman_DependencyForwardingSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    HdPrman_DependencyForwardingSceneIndexPlugin();    

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2208

#endif // PXR_IMAGING_HDPRMAN_DEPENDENCY_FORWARDING_SCENE_INDEX_PLUGIN_H
