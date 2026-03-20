//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_SCENE_INDEX_PLUGIN_H
#define PXR_IMAGING_HD_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hf/pluginBase.h"
#include "pxr/imaging/hd/sceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneIndexPlugin : public HfPluginBase
{
public:

    HD_API
    HdSceneIndexBaseRefPtr AppendSceneIndex(
        const std::string &renderInstanceId,
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs);
    
    /// Returns true if the plugin is enabled, in the sense that it should be
    /// consulted for its contribution to the scene index chain via
    /// AppendSceneIndex.
    ///
    /// Current implementation simply delegates to _IsEnabled; this may change 
    /// in the future to first consult inputArgs (e.g. to check a data source
    /// that provides the plugin IDs to disable) before calling _IsEnabled.
    ///
    /// \sa HdSceneIndexPluginRegistry::AppendSceneIndex.
    ///
    HD_API
    bool IsEnabled(
        const HdContainerDataSourceHandle &inputArgs) const;

protected:

    /// Subclasses implement this to instantiate one or more scene indicies
    /// which take the provided scene as input. The return value should be
    /// the final scene created -- or the inputScene itself if nothing is
    /// created.
    HD_API
    virtual HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs);

    /// Subclasses implement this to instantiate one or more scene indicies
    /// which take the provided scene as input. The return value should be
    /// the final scene created -- or the inputScene itself if nothing is
    /// created.
    ///
    /// Callers can override this method if they want to get the render
    /// instance ID in addition to the input scene and arguments. Callers
    /// should only override one of the two _AppendSceneIndex overrides: if
    /// both are overridden, only this override will be called.
    ///
    HD_API
    virtual HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const std::string &renderInstanceId,
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs);

    /// Subclasses implement this to indicate whether the plugin is enabled.
    /// This is preferable to using env guards to gate registration and
    /// scene index instatiation in _AppendSceneIndex.
    ///
    /// Base implementation returns true.
    HD_API
    virtual bool _IsEnabled(
        const HdContainerDataSourceHandle &inputArgs) const;
    
    HdSceneIndexPlugin() = default;
    HD_API
    ~HdSceneIndexPlugin() override;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_SCENE_INDEX_PLUGIN_H
