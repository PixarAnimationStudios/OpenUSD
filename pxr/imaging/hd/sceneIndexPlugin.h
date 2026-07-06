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

/// A scene index plugin bundles one or more (typically filtering) scene indices 
/// together and enables their runtime discovery and participation in the
/// scene index graph construction process.
///
/// Scene index plugins leverage the Plug system and are required to have a
/// corresponding entry in the plugInfo.json file in the library directory they
/// are housed in. They are managed by the HdSceneIndexPluginRegistry.
///
/// The JSON entry for a scene index plugin should have the following structure:
///
/// \code
/// "MySceneIndexPlugin": {
///     "bases": ["HdSceneIndexPlugin"],
///
///     # Mandatory fields for "hydra plugins" due to HfPluginRegistry.
///     "displayName": "My Awesome Plugin", # Unused currently.
///     "priority": 0, # Not relevant for scene index plugins.
///
///     # Filtering fields that specify which renderers and apps the plugin
///     # is relevant for. The empty string "" indicates "all".
///     #
///     "loadWithRenderer": ["rendererA", "rendererB"], # Mandatory.
///     "loadWithApps": ["appA", "appB"], # Optional, defaults to "" (all).
/// 
///     # Fields that specify tags and tag-based ordering constraints for the
///     # plugin. These fields provide an alternative and improvement to the
///     # insertion phase/order based constraints of the C++ registration API.
///     # These constraints are considered when the ordering policy used by the 
///     # HdSceneIndexPluginRegistry is "Hybrid" or "JsonMetadataOnly".
///
///     "tags": ["tagA", "tagB"], # Optional. The plugin typename serves as an
///                               # implicit tag.
///     "ordering": { # Optional.
///
///        # Tags or plugin typenames that this plugin should be ordered after.
///         "after": ["PluginTypeName1", "Tag3"],
///
///         # Tags or plugin typenames that this plugin should be ordered before.
///         "before": ["PluginTypeName2", "Tag4"],
///
///         # The tags above partition the plugins into three groups (indicated
///         # by [] below):
///         # [afterTags] -> [... -> "MySceneIndexPlugin" -> ...] -> [beforeTags]
///         # The insertion position specifies the ordering of this plugin 
///         # within the middle group of plugins, with the options:
///         # - "firstAfter": insert as early as possible, after the afterTags.
///         # - "lastBefore": insert as late as possible, before the beforeTags.
///         # - "doesNotMatter": insert in any position within the middle group.
///         # The default is "doesNotMatter".
///         "position": "firstAfter"
///     }
/// }
///
/// \endcode
///
/// \sa HdSceneIndexPluginRegistry
/// \sa HdSceneIndexPluginRegistry::PluginOrderingPolicy
/// \sa HdSceneIndexPluginRegistry::RegisterSceneIndexForRenderer
///     
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
