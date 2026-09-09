//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdsi/applicationRenderSettingsSceneIndex.h"

#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/renderSettingDescriptorSchema.h"
#include "pxr/imaging/hd/renderSettingsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"
#include "pxr/imaging/hd/sceneIndexCreateArgsSchema.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/trace/trace.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace HdsiApplicationRenderSettingsSceneIndex_Impl
{

// Scene index state.
struct _RenderSettingsInfo
{
    // Renderer-advertised render setting descriptors.
    const HdRenderSettingDescriptorContainerSchema descriptors;
    // Prim-level underlay data source built once from the default
    // values of the descriptors.
    HdContainerDataSourceHandle const primUnderlayDataSource;
    // Application-set values, split into plain and namespaced keys.
    std::map<TfToken, VtValue> applicationRenderSettings;
    std::map<TfToken, VtValue> applicationNamespacedRenderSettings;
    // Cached input active render settings path, for invalidation.
    SdfPath inputActiveRsPath;
};

}

using namespace HdsiApplicationRenderSettingsSceneIndex_Impl;

namespace {

static const SdfPath s_primPath("/__ApplicationRenderSettings");

// Returns true if the render setting token is namespaced, i.e. its string
// contains a ':' (e.g. "ri:samples").
bool
_IsNamespaced(const TfToken &name)
{
    return name.GetString().find(':') != std::string::npos;
}

// Returns the path of the scene index's active render settings prim, or an
// empty path if there is none.
SdfPath
_ComputeInputActiveRenderSettingsPath(
    const HdSceneIndexBaseRefPtr &sceneIndex)
{
    const HdPathDataSourceHandle ds =
        HdSceneGlobalsSchema::GetFromSceneIndex(sceneIndex)
            .GetActiveRenderSettingsPrim();
    if (!ds) {
        return {};
    }
    return ds->GetTypedValue(0.0);
}

// The 'namespacedSettings' sub-container of the application overrides, backed
// directly by _RenderSettingsInfo::applicationNamespacedRenderSettings.
class _NamespacedRenderSettingsDataSource final
    : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_NamespacedRenderSettingsDataSource);

    _NamespacedRenderSettingsDataSource(_RenderSettingsInfoSharedPtr info)
      : _info(std::move(info))
    {
    }

    TfTokenVector
    GetNames() override
    {
        TfTokenVector names;
        names.reserve(_info->applicationNamespacedRenderSettings.size());
        for (const auto &entry : _info->applicationNamespacedRenderSettings) {
            names.push_back(entry.first);
        }
        return names;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        const auto it =
            _info->applicationNamespacedRenderSettings.find(name);
        if (it == _info->applicationNamespacedRenderSettings.end()) {
            return nullptr;
        }
        return HdCreateTypedRetainedDataSource(it->second);
    }

private:
    _RenderSettingsInfoSharedPtr const _info;
};

// The 'renderSettings'-level container data source for the application
// overrides, backed directly by _RenderSettingsInfo so the composed prim
// reflects live overrides: the plain keys are direct children and the
// namespaced keys live under 'namespacedSettings'.
class _RenderSettingsDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_RenderSettingsDataSource);

    _RenderSettingsDataSource(_RenderSettingsInfoSharedPtr info)
      : _info(std::move(info))
    {
    }

    TfTokenVector
    GetNames() override
    {
        TfTokenVector names;
        names.reserve(_info->applicationRenderSettings.size() + 1);
        for (const auto &entry : _info->applicationRenderSettings) {
            names.push_back(entry.first);
        }
        if (!_info->applicationNamespacedRenderSettings.empty()) {
            names.push_back(HdRenderSettingsSchemaTokens->namespacedSettings);
        }
        return names;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        if (name == HdRenderSettingsSchemaTokens->namespacedSettings) {
            if (_info->applicationNamespacedRenderSettings.empty()) {
                return nullptr;
            }
            return _NamespacedRenderSettingsDataSource::New(_info);
        }

        const auto it = _info->applicationRenderSettings.find(name);
        if (it == _info->applicationRenderSettings.end()) {
            return nullptr;
        }
        // Returns nullptr for empty VtValue.
        return HdCreateTypedRetainedDataSource(it->second);
    }

private:
    _RenderSettingsInfoSharedPtr const _info;
};

// Builds the prim-level underlay data source from the default values
// of the render setting descriptors.
// If the render settings descriptors key K contains a ':", the default
// value is at the locator renderSettings/namespacedSettings/K.
// Otherwise, it as at the locator renderSettings/K.
HdContainerDataSourceHandle
_BuildPrimUnderlayDataSource(
    const HdRenderSettingDescriptorContainerSchema &descriptors)
{
    std::vector<TfToken> names;
    std::vector<HdDataSourceBaseHandle> sources;
    std::vector<TfToken> nsNames;
    std::vector<HdDataSourceBaseHandle> nsSources;

    for (const TfToken &name : descriptors.GetNames()) {
        HdRenderSettingDescriptorSchema descriptor = descriptors.Get(name);
        const HdSampledDataSourceHandle defaultValue =
            descriptor.GetDefaultValue();
        if (!defaultValue) {
            continue;
        }
        if (_IsNamespaced(name)) {
            nsNames.push_back(name);
            nsSources.push_back(defaultValue);
        } else {
            names.push_back(name);
            sources.push_back(defaultValue);
        }
    }

    if (!nsNames.empty()) {
        names.push_back(HdRenderSettingsSchemaTokens->namespacedSettings);
        sources.push_back(
            HdSampledDataSourceContainerSchema::BuildRetained(
                nsNames.size(), nsNames.data(), nsSources.data()));
    }

    // When using legacy render delegates through
    // HdRenderDelegateAdapterRenderer, indicate that we communicate the
    // render settings in band and the adapter needs to call them into
    // calls to HdRenderDelegate::SetRenderSetting.

    names.push_back(
        HdRenderSettingsSchemaTokens->useForLegacyRenderDelegateSettings);
    sources.push_back(
        HdRetainedTypedSampledDataSource<bool>::New(true));

    return HdRetainedContainerDataSource::New(
        HdRenderSettingsSchema::GetSchemaToken(),
        HdRetainedContainerDataSource::New(
            names.size(), names.data(), sources.data()));
}

} // anonymous namespace

////////////////////////////////////////////////////////////////////////////////

/* static */
HdsiApplicationRenderSettingsSceneIndexRefPtr
HdsiApplicationRenderSettingsSceneIndex::New(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    const HdContainerDataSourceHandle &inputArgs)
{
    return TfCreateRefPtr(
        new HdsiApplicationRenderSettingsSceneIndex(
            inputSceneIndex, inputArgs));
}

HdsiApplicationRenderSettingsSceneIndex::
HdsiApplicationRenderSettingsSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    const HdContainerDataSourceHandle &inputArgs)
  : HdsiApplicationRenderSettingsSceneIndex(
        inputSceneIndex,
        HdSceneIndexCreateArgsSchema(inputArgs).GetRenderSettingDescriptors())
{
}

HdsiApplicationRenderSettingsSceneIndex::
HdsiApplicationRenderSettingsSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    const HdRenderSettingDescriptorContainerSchema &descriptors)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
  , _info(std::make_shared<_RenderSettingsInfo>(
        _RenderSettingsInfo{
            descriptors,
            _BuildPrimUnderlayDataSource(descriptors),
            /* applicationRenderSettings = */ {},
            /* applicationNamespacedRenderSettings = */ {},
            _ComputeInputActiveRenderSettingsPath(_GetInputSceneIndex()) }))
{
}

/* static */
const SdfPath &
HdsiApplicationRenderSettingsSceneIndex::GetApplicationRenderSettingsPrimPath()
{
    return s_primPath;
}

HdRenderSettingDescriptorContainerSchema
HdsiApplicationRenderSettingsSceneIndex::
GetRenderSettingDescriptorsFromRenderer() const
{
    return _info->descriptors;
}

static
VtValue
_GetValue(HdSampledDataSourceHandle const &ds)
{
    if (ds) {
        return ds->GetValue(0.0f);
    } else {
        return {};
    }
}


static
VtValue
_GetRenderSetting(const HdRenderSettingsSchema &schema, const TfToken &name)
{
    TRACE_FUNCTION();

    if (_IsNamespaced(name)) {
        return _GetValue(
            schema.GetNamespacedSettings().Get(name));
    } else {
        HdContainerDataSourceHandle const container = schema.GetContainer();
        if (!container) {
            return {};
        }
        return _GetValue(HdSampledDataSource::Cast(container->Get(name)));
    }
}

VtValue
HdsiApplicationRenderSettingsSceneIndex::GetRenderSetting(
    const RenderSettingsSource src,
    const TfToken &name) const
{
    TRACE_FUNCTION();

    switch(src)
    {
    case RenderSettingsSource::Resolved:
        return _GetRenderSetting(
            HdRenderSettingsSchema::GetFromParent(
                _GetApplicationRenderSettingsDataSource()),
            name);
    case RenderSettingsSource::ApplicationOverride:
    {
        const std::map<TfToken, VtValue> &settings =
            _IsNamespaced(name)
            ? _info->applicationNamespacedRenderSettings
            : _info->applicationRenderSettings;
        const auto it = settings.find(name);
        if (it == settings.end()) {
            return {};
        }
        return it->second;
    }
    case RenderSettingsSource::InputRenderSettings:
        return _GetRenderSetting(
            HdRenderSettingsSchema::GetFromParent(
                _GetInputRenderSettingsDataSource()),
            name);
    case RenderSettingsSource::RendererDefault:
        return _GetValue(
            GetRenderSettingDescriptorsFromRenderer()
                .Get(name)
                .GetDefaultValue());
    }

    return {};
}

void
HdsiApplicationRenderSettingsSceneIndex::SetApplicationOverrideRenderSetting(
    const TfToken &name, const VtValue &newValue)
{
    TRACE_FUNCTION();

    const bool isNamespaced = _IsNamespaced(name);

    std::map<TfToken, VtValue> &settings =
        isNamespaced
        ? _info->applicationNamespacedRenderSettings
        : _info->applicationRenderSettings;

    VtValue &value = settings[name];
    if (value == newValue) {
        // No-op: the override value is unchanged.
        return;
    }

    value = newValue;

    if (!_IsObserved()) {
        return;
    }

    const HdDataSourceLocator baseLocator =
        isNamespaced
        ? HdRenderSettingsSchema::GetNamespacedSettingsLocator()
        : HdRenderSettingsSchema::GetDefaultLocator();

    _SendPrimsDirtied({{
        GetApplicationRenderSettingsPrimPath(), baseLocator.Append(name)}});
}

HdContainerDataSourceHandle
HdsiApplicationRenderSettingsSceneIndex::
_GetInputRenderSettingsDataSource() const
{
    if (_info->inputActiveRsPath.IsEmpty()) {
        return nullptr;
    }
    return
        _GetInputSceneIndex()->GetPrim(_info->inputActiveRsPath).dataSource;
}

HdContainerDataSourceHandle
HdsiApplicationRenderSettingsSceneIndex::
_GetApplicationRenderSettingsDataSource() const
{
    TRACE_FUNCTION();

    HdContainerDataSourceHandle const overrideDs =
        HdRetainedContainerDataSource::New(
            HdRenderSettingsSchema::GetSchemaToken(),
            _RenderSettingsDataSource::New(_info));

    return HdOverlayContainerDataSource::New(
        { overrideDs,
          _GetInputRenderSettingsDataSource(),
          _info->primUnderlayDataSource });
}

HdSceneIndexPrim
HdsiApplicationRenderSettingsSceneIndex::GetPrim(const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    if (primPath == GetApplicationRenderSettingsPrimPath()) {
        return {
            HdPrimTypeTokens->renderSettings,
            _GetApplicationRenderSettingsDataSource() };
    }

    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (primPath == HdSceneGlobalsSchema::GetDefaultPrimPath()) {
        // Make our prim the active render settings prim (strongest opinion).
        static HdContainerDataSourceHandle const sceneGlobals =
            HdRetainedContainerDataSource::New(
                HdSceneGlobalsSchemaTokens->sceneGlobals,
                HdSceneGlobalsSchema::Builder()
                    .SetActiveRenderSettingsPrim(
                        HdRetainedTypedSampledDataSource<SdfPath>::New(
                            GetApplicationRenderSettingsPrimPath()))
                    .Build());
        prim.dataSource =
            HdCreateOverlayContainerDataSource(
                sceneGlobals, prim.dataSource);
    }

    return prim;
}

SdfPathVector
HdsiApplicationRenderSettingsSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    SdfPathVector paths = _GetInputSceneIndex()->GetChildPrimPaths(primPath);

    if (primPath == HdSceneGlobalsSchema::GetDefaultPrimPath()) {
        const SdfPath &newPath = GetApplicationRenderSettingsPrimPath();
        if (std::find(paths.begin(), paths.end(), newPath) == paths.end()) {
            paths.push_back(newPath);
        }
    }

    return paths;
}

void
HdsiApplicationRenderSettingsSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    const bool isObserved = _IsObserved();

    bool resyncApplicationRenderSettings = false;

    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        if (entry.primPath == HdSceneGlobalsSchema::GetDefaultPrimPath()) {
            _info->inputActiveRsPath =
                _ComputeInputActiveRenderSettingsPath(_GetInputSceneIndex());
            resyncApplicationRenderSettings = true;
            break;
        }
        if (isObserved && entry.primPath == _info->inputActiveRsPath) {
            resyncApplicationRenderSettings = true;
        }
    }

    if (resyncApplicationRenderSettings) {
        HdSceneIndexObserver::AddedPrimEntries newEntries(entries);
        newEntries.push_back({
                GetApplicationRenderSettingsPrimPath(),
                HdPrimTypeTokens->renderSettings });
        _SendPrimsAdded(newEntries);
    } else {
        _SendPrimsAdded(entries);
    }
}

void
HdsiApplicationRenderSettingsSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    // Does not handle the case where scene globals prim gets removed.
    // But that should never happen.

    if (!_IsObserved()) {
        return;
    }

    _SendPrimsRemoved(entries);

    if (_info->inputActiveRsPath.IsEmpty()) {
        return;
    }

    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        if (_info->inputActiveRsPath.HasPrefix(entry.primPath)) {
            _SendPrimsAdded({{
                GetApplicationRenderSettingsPrimPath(),
                HdPrimTypeTokens->renderSettings }});
            break;
        }
    }
}

void
HdsiApplicationRenderSettingsSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    const bool isObserved = _IsObserved();

    HdDataSourceLocatorSet dirtyLocators;

    for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
        if (entry.primPath == HdSceneGlobalsSchema::GetDefaultPrimPath() &&
            entry.dirtyLocators.Intersects(
                HdSceneGlobalsSchema::GetActiveRenderSettingsPrimLocator())) {
            _info->inputActiveRsPath =
                _ComputeInputActiveRenderSettingsPath(_GetInputSceneIndex());
            dirtyLocators = HdDataSourceLocatorSet::UniversalSet();
            break;
        }
        if (isObserved && entry.primPath == _info->inputActiveRsPath) {
            dirtyLocators.insert(entry.dirtyLocators);
        }
    }

    if (!isObserved) {
        return;
    }

    if (dirtyLocators.IsEmpty()) {
        _SendPrimsDirtied(entries);
    } else {
        HdSceneIndexObserver::DirtiedPrimEntries newEntries(entries);
        newEntries.push_back({
                GetApplicationRenderSettingsPrimPath(),
                dirtyLocators});
        _SendPrimsDirtied(newEntries);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
