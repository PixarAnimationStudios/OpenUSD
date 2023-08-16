//
// Copyright 2023 Pixar
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
//
#include "pxr/imaging/hdsi/renderSettingsFilteringSceneIndex.h"

#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/renderSettingsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/utils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdsiRenderSettingsFilteringSceneIndexTokens,
    HDSI_RENDER_SETTINGS_FILTERING_SCENE_INDEX_TOKENS);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (active_depOn_sceneGlobals_arsp)
);

namespace
{

static const SdfPath s_renderScope("/Render");
static const SdfPath s_fallbackPath("/Render/_FallbackSettings");

// Builds and returns a data source to invalidate the renderSettings.active
// locator when the sceneGlobals.activeRenderSettingsPrim locator is dirtied.
//
HdContainerDataSourceHandle
_GetDependenciesDataSource(
    const HdDataSourceBaseHandle &existingDependenciesDs)
{
    static const HdRetainedContainerDataSourceHandle renderSettingsDepDS =
        HdRetainedContainerDataSource::New(
            _tokens->active_depOn_sceneGlobals_arsp,
            HdDependencySchema::Builder()
            .SetDependedOnPrimPath(
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                    HdSceneGlobalsSchema::GetDefaultPrimPath()))
            .SetDependedOnDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdSceneGlobalsSchema::GetActiveRenderSettingsPrimLocator()))
            .SetAffectedDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdRenderSettingsSchema::GetActiveLocator()))
            .Build()
        );
    
    return HdOverlayContainerDataSource::OverlayedContainerDataSources(
                HdContainerDataSource::Cast(existingDependenciesDs),
                renderSettingsDepDS);
}

// Build and return a container with only the names that begin with the
// requested prefixes.
//
HdContainerDataSourceHandle
_GetFilteredNamespacedSettings(
    const HdContainerDataSourceHandle &c,
    const VtArray<TfToken> &prefixes)
{
    if (!c) {
        return HdContainerDataSourceHandle();
    }

    TfTokenVector names = c->GetNames();
    names.erase(
        std::remove_if(names.begin(), names.end(),
            [&](const TfToken &name) {
                const std::string &nameStr = name.GetString();
                for (const TfToken &prefix : prefixes) {
                    if (TfStringStartsWith(nameStr, prefix.GetString())) {
                        return false;
                    }
                }
                return true;
            }),
        names.end());

    std::vector<HdDataSourceBaseHandle> valuesDs;
    valuesDs.reserve(names.size());
    for (const TfToken &name : names) {
        valuesDs.push_back(c->Get(name));
    }

    return HdRetainedContainerDataSource::New(
        names.size(), names.data(), valuesDs.data());
}

// Data source override for the 'renderSettings' locator.
// Filters the 'namespacedSettings' container and adds support for the 'active'
// field.
//
class _RenderSettingsDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_RenderSettingsDataSource);

    _RenderSettingsDataSource(
        const HdContainerDataSourceHandle &renderSettingsContainer,
        const HdSceneIndexBaseRefPtr &si,
        const SdfPath &settingsPrimPath,
        const VtArray<TfToken> &namespacePrefixes)
    : _input(renderSettingsContainer)
    , _si(si)
    , _primPath(settingsPrimPath)
    , _namespacePrefixes(namespacePrefixes)
    {
    }

    TfTokenVector
    GetNames() override
    {
        TfTokenVector names = _input->GetNames();
        names.push_back(HdRenderSettingsSchemaTokens->active);
        return names;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        if (name == HdRenderSettingsSchemaTokens->active) {
            bool isActive = false;
            SdfPath activePath;
            if (HdUtils::HasActiveRenderSettingsPrim(_si, &activePath)) {
                isActive = (activePath == _primPath);
            }
            return HdRetainedTypedSampledDataSource<bool>::New(isActive);
        }

        HdDataSourceBaseHandle result = _input->Get(name);

        if (name == HdRenderSettingsSchemaTokens->namespacedSettings &&
            !_namespacePrefixes.empty()) {

            return _GetFilteredNamespacedSettings(
                HdContainerDataSource::Cast(result), _namespacePrefixes);
        }

        return result;
    }

private:
    const HdContainerDataSourceHandle _input;
    const HdSceneIndexBaseRefPtr _si;
    const SdfPath _primPath;
    const VtArray<TfToken> _namespacePrefixes;
};

class _RenderSettingsPrimDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_RenderSettingsPrimDataSource);

    _RenderSettingsPrimDataSource(
        const HdContainerDataSourceHandle &input,
        const HdSceneIndexBaseRefPtr &si,
        const SdfPath &primPath,
        const VtArray<TfToken> namespacePrefixes)
    : _input(input)
    , _si(si)
    , _primPath(primPath)
    , _namespacePrefixes(namespacePrefixes)
    {
    }

    TfTokenVector
    GetNames() override
    {
        TfTokenVector names = _input->GetNames();
        names.push_back(HdDependenciesSchemaTokens->__dependencies);
        return names;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result = _input->Get(name);

        if (name == HdRenderSettingsSchemaTokens->renderSettings) {
            if (HdContainerDataSourceHandle renderSettingsContainer =
                    HdContainerDataSource::Cast(result)) {
                return _RenderSettingsDataSource::New(
                    renderSettingsContainer, _si, _primPath,
                    _namespacePrefixes);
            }
        }
        if (name == HdDependenciesSchemaTokens->__dependencies) {
            return _GetDependenciesDataSource(result);
        }

        return result;
    }

private:
    const HdContainerDataSourceHandle _input;
    const HdSceneIndexBaseRefPtr _si;
    const SdfPath _primPath;
    const VtArray<TfToken> _namespacePrefixes;
};

VtArray<TfToken>
_GetNamespacePrefixes(const HdContainerDataSourceHandle &inputArgs)
{
    if (!inputArgs) {
        return {};
    }

    HdTokenArrayDataSourceHandle tokenArrayHandle =
        HdTokenArrayDataSource::Cast(inputArgs->Get(
            HdsiRenderSettingsFilteringSceneIndexTokens->namespacePrefixes));

    if (tokenArrayHandle) {
        return tokenArrayHandle->GetTypedValue(0);
    }

    return {};
}

HdContainerDataSourceHandle
_GetFallbackPrimDataSource(const HdContainerDataSourceHandle &inputArgs)
{
    if (!inputArgs) {
        return nullptr;
    }

    return HdContainerDataSource::Cast(inputArgs->Get(
            HdsiRenderSettingsFilteringSceneIndexTokens->fallbackPrimDs));
}

} // namespace anonymous

////////////////////////////////////////////////////////////////////////////////

/* static */
HdsiRenderSettingsFilteringSceneIndexRefPtr
HdsiRenderSettingsFilteringSceneIndex::New(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    const HdContainerDataSourceHandle &inputArgs)
{
    return TfCreateRefPtr(
        new HdsiRenderSettingsFilteringSceneIndex(inputSceneIndex, inputArgs));
}


HdsiRenderSettingsFilteringSceneIndex::HdsiRenderSettingsFilteringSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    const HdContainerDataSourceHandle &inputArgs)
: HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
, _namespacePrefixes(_GetNamespacePrefixes(inputArgs))
, _fallbackPrimDs(_GetFallbackPrimDataSource(inputArgs))
, _addedFallbackPrim(false)
{
}

HdSceneIndexPrim
HdsiRenderSettingsFilteringSceneIndex::GetPrim(const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (prim.primType == HdPrimTypeTokens->renderSettings && prim.dataSource) {
        // existing render settings prim
        prim.dataSource = _RenderSettingsPrimDataSource::New(
            prim.dataSource, _GetInputSceneIndex(), primPath,
            _namespacePrefixes);

    } else if (_addedFallbackPrim  && primPath == GetFallbackPrimPath()) {

        prim.primType = HdPrimTypeTokens->renderSettings;
        prim.dataSource = _fallbackPrimDs;
    }

    return prim;
}

SdfPathVector
HdsiRenderSettingsFilteringSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{ 
    // Avoid a copy if possible.
    if (ARCH_UNLIKELY(
            (primPath == GetRenderScope() || primPath.IsAbsoluteRootPath()) &&
             _addedFallbackPrim)) {
        
        SdfPathVector paths =
            _GetInputSceneIndex()->GetChildPrimPaths(primPath);
        paths.push_back(primPath.IsAbsoluteRootPath()
                        ? GetRenderScope()
                        : GetFallbackPrimPath());

        return paths;
    }

    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

/* static */
const SdfPath&
HdsiRenderSettingsFilteringSceneIndex::GetFallbackPrimPath()
{
    return s_fallbackPath;
}

/* static */
const SdfPath&
HdsiRenderSettingsFilteringSceneIndex::GetRenderScope()
{
    return s_renderScope;
}

void
HdsiRenderSettingsFilteringSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    if (ARCH_UNLIKELY(_fallbackPrimDs && !_addedFallbackPrim)) {
        
        HdSceneIndexObserver::AddedPrimEntries addedEntries = entries;

        addedEntries.emplace_back(
            GetFallbackPrimPath(), HdPrimTypeTokens->renderSettings);

        _addedFallbackPrim = true;
        _SendPrimsAdded(addedEntries);

    } else {
        _SendPrimsAdded(entries);
    }
}

void
HdsiRenderSettingsFilteringSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

void
HdsiRenderSettingsFilteringSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _SendPrimsDirtied(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
