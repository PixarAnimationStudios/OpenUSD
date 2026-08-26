//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_APPLICATION_RENDER_SETTINGS_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_APPLICATION_RENDER_SETTINGS_SCENE_INDEX_H

#include "pxr/imaging/hdsi/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/schemaTypeDefs.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace HdsiApplicationRenderSettingsSceneIndex_Impl
{
struct _RenderSettingsInfo;
using _RenderSettingsInfoSharedPtr = std::shared_ptr<_RenderSettingsInfo>;
}

TF_DECLARE_WEAK_AND_REF_PTRS(HdsiApplicationRenderSettingsSceneIndex);

/// \class HdsiApplicationRenderSettingsSceneIndex
///
/// A filtering scene index that lets an application inject a
/// single, authoritative render settings prim into the scene and make it the
/// active render settings prim (via HdSceneGlobalsSchema).
///
/// The injected prim gathers render setting opinions from three sources, in
/// decreasing priority order:
/// 1. Values the application explicitly overrode (via
///    SetApplicationRenderSetting).
/// 2. The render settings of the input scene's currently-active render
///    settings prim.
/// 3. The renderer-advertised default values from the descriptors carried by
///    \p inputArgs (which conforms to HdSceneIndexCreateArgsSchema).
///
class HdsiApplicationRenderSettingsSceneIndex final
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    /// The scene index assumes that \p inputArgs conforms to
    /// HdSceneIndexCreateArgsSchema to extract the render settings
    /// descriptors advertised by HdRendererPlugin::GetSceneIndexCreateArgs().
    HDSI_API
    static HdsiApplicationRenderSettingsSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdContainerDataSourceHandle &inputArgs);

    /// The render setting descriptors advertised by the renderer.
    HDSI_API
    HdRenderSettingDescriptorContainerSchema
    GetRenderSettingDescriptorsFromRenderer() const;

    enum class RenderSettingsSource
    {
        /// The resolved value as seen by observers of the render settings prim
        /// in this scene index.
        Resolved,
        /// The override opinion set on the scene index. Empty value indicates
        /// no override.
        ApplicationOverride,
        /// The opinion from the render settings prim at the active render
        /// settings prim path in the input scene index.
        InputRenderSettings,
        /// The default value that the renderer provides for the render setting.
        RendererDefault
    };

    /// Returns the value of a render setting \p name at a particular stage \p name.
    HDSI_API
    VtValue GetRenderSetting(RenderSettingsSource src, const TfToken &name) const;

    /// Overrides the value for the render setting \p name with an application
    /// opinion, taking precedence over the input scene's active render settings
    /// prim. A set that does not change the stored override value is a no-op.
    /// Passing an empty VtValue removed the override.
    HDSI_API
    void SetApplicationOverrideRenderSetting(const TfToken &name, const VtValue &value);

    /// Path of the injected render settings prim
    /// (/__ApplicationRenderSettings).
    HDSI_API
    static const SdfPath &GetApplicationRenderSettingsPrimPath();
    
    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    HDSI_API
    HdsiApplicationRenderSettingsSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdContainerDataSourceHandle &inputArgs);

    HDSI_API
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    HDSI_API
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    HDSI_API
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:
    HdsiApplicationRenderSettingsSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdRenderSettingDescriptorContainerSchema &descriptors);

    HdContainerDataSourceHandle _GetInputRenderSettingsDataSource() const;
    HdContainerDataSourceHandle _GetApplicationRenderSettingsDataSource() const;
    
    HdsiApplicationRenderSettingsSceneIndex_Impl::_RenderSettingsInfoSharedPtr
        const _info;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDSI_APPLICATION_RENDER_SETTINGS_SCENE_INDEX_H
