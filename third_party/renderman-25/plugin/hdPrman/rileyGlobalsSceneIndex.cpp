// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "hdPrman/rileyGlobalsSceneIndex.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "hdPrman/rileyGlobalsSchema.h"
#include "hdPrman/rileyParamSchema.h"
#include "hdPrman/rileyParamListSchema.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/renderSettingsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _dependencyTokens,
    ((frame, "__frame"))
    ((renderSettings, "__renderSettings"))
    ((renderSettingsPath, "__renderSettingsPath"))
);

TF_DEFINE_PRIVATE_TOKENS(
    _renderTerminalTokens, // properties in PxrRenderTerminalsAPI
    ((outputsRiIntegrator, "outputs:ri:integrator"))
    ((outputsRiSampleFilters, "outputs:ri:sampleFilters"))
    ((outputsRiDisplayFilters, "outputs:ri:displayFilters"))
);

static
const SdfPath &
_GetGlobalsPrimPath()
{
    static const SdfPath path("/__rileyGlobals__");
    return path;
}

#if HD_API_VERSION >= 71

static
const TfToken &
_GetRileyFrameToken()
{
    static const TfToken token(RixStr.k_Ri_Frame.CStr());
    return token;
}

/// Invalidate frame riley option if current frame on scene globals
/// changes.
static
const HdDataSourceBaseHandle &
_GetFrameDependency()
{
    static HdDataSourceBaseHandle const ds =
        HdDependencySchema::Builder()
            .SetDependedOnPrimPath(
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                    HdSceneGlobalsSchema::GetDefaultPrimPath()))
            .SetDependedOnDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdSceneGlobalsSchema::GetCurrentFrameLocator()))
            .SetAffectedDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdPrmanRileyGlobalsSchema::GetOptionsLocator()
                        .Append(HdPrmanRileyParamListSchemaTokens->params)
                        .Append(_GetRileyFrameToken())))
            .Build();
    return ds;
}

#endif // #if HD_API_VERSION >= 71

/// Invalidate riley options if the namespaced settings on the
/// active render settings prim changes.
static
HdDataSourceBaseHandle
_GetRenderSettingsDependency(const SdfPath renderSettingsPath)
{
    static HdLocatorDataSourceHandle const dependedLocatorDs =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdRenderSettingsSchema::GetNamespacedSettingsLocator());
    static HdLocatorDataSourceHandle const affectedLocatorDs =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdPrmanRileyGlobalsSchema::GetOptionsLocator()
                .Append(HdPrmanRileyParamListSchemaTokens->params));

    return
        HdDependencySchema::Builder()
            .SetDependedOnPrimPath(
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                    renderSettingsPath))
            .SetDependedOnDataSourceLocator(
                dependedLocatorDs)
            .SetAffectedDataSourceLocator(
                affectedLocatorDs)
            .Build();
}

/// Invalidate the prim path of the dependency on the render settings prim
/// when the render settings prim path changes.
static
const HdDataSourceBaseHandle &
_GetRenderSettingsPathDependency()
{
    static HdDataSourceBaseHandle const ds =
        HdDependencySchema::Builder()
            .SetDependedOnPrimPath(
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                    HdSceneGlobalsSchema::GetDefaultPrimPath()))
            .SetDependedOnDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdSceneGlobalsSchema::GetActiveRenderSettingsPrimLocator()))
            .SetAffectedDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdDependenciesSchema::GetDefaultLocator()
                        .Append(_dependencyTokens->renderSettings)
                        .Append(HdDependencySchemaTokens->dependedOnPrimPath)))
            .Build();
    return ds;
}

/// Get all dependencies for riley:globals prim.
static
HdContainerDataSourceHandle
_GetDependencies(const SdfPath &renderSettingsPath)
{
    static TfToken names[3]  = {
#if HD_API_VERSION >= 71
        _dependencyTokens->frame,
#endif
        _dependencyTokens->renderSettingsPath,
        _dependencyTokens->renderSettings
    };
    static HdDataSourceBaseHandle values[3];
    size_t count = 0;

#if HD_API_VERSION >= 71
    values[count] = _GetFrameDependency();
    count++;
#endif

    values[count] = _GetRenderSettingsPathDependency();
    count++;
    
    if (!renderSettingsPath.IsEmpty()) {
        values[count] =
            _GetRenderSettingsDependency(renderSettingsPath);
        count++;
    }

    return
        HdDependenciesSchema::BuildRetained(
            count,
            names,
            values);
}

// Add current frame from globals schema to riley params
// (as riley param schema).
static
void
_FillRileyParamsFromSceneGlobals(
    HdSceneGlobalsSchema globalsSchema,
    std::vector<TfToken> * const names,
    std::vector<HdDataSourceBaseHandle> * const dataSources)
{

#if HD_API_VERSION >= 71
    if (HdDoubleDataSourceHandle const currentFrameDs =
            globalsSchema.GetCurrentFrame())
    {
        const double frame = currentFrameDs->GetTypedValue(0.0f);
        if (!std::isnan(frame)) {
            names->push_back(_GetRileyFrameToken());
            dataSources->push_back(
                HdPrmanRileyParamSchema::Builder()
                    .SetValue(
                        HdRetainedTypedSampledDataSource<int>::New(
                            int(frame)))
                    .Build());
        }
    }
#endif
}


// Given the namespaced settings container of the current render settings
// prim, add the suitable settings to the riley params (as riley
// param schema).
//
// We explicitly drop the the outputs from PxrRenderTerminalsAPI.
//
// We also drop the "ri:" namespace when creating the riley params (and
// drop those settings not in the namespace).
//
// Examples:
//
//   Render setting name     Riley param name    Pre-defined UString
//
//   ri:hider:maxsamples     hider:maxsamples    Rix:k_hider_maxsamples
//   ri:Ri:Cropwindow        Ri:Cropwindow       Rix:k_riCropWindow.
//
static
void
_FillRileyParamsFromNamespacedSettings(
    HdContainerDataSourceHandle const &settingsDs,
    std::vector<TfToken> * const names,
    std::vector<HdDataSourceBaseHandle> * const dataSources)
{
    for (const TfToken &name : settingsDs->GetNames()) {
        if (name == _renderTerminalTokens->outputsRiIntegrator    ||
            name == _renderTerminalTokens->outputsRiSampleFilters ||
            name == _renderTerminalTokens->outputsRiDisplayFilters) {
            continue;
        }

        static const std::string rileyPrefix("ri");

        const std::pair<std::string, bool> strippedName =
            SdfPath::StripPrefixNamespace(
                name.GetString(), rileyPrefix);

        if (!strippedName.second) {
            continue;
        }

        HdSampledDataSourceHandle const ds =
            HdSampledDataSource::Cast(settingsDs->Get(name));
        if (!ds) {
            continue;
        }

        names->push_back(TfToken(strippedName.first));
        dataSources->push_back(
            HdPrmanRileyParamSchema::Builder()
                .SetValue(ds)
                .Build());
    }
}

static
void
_FillRileyParamsFromRenderSettings(
    HdRenderSettingsSchema renderSettingsSchema,
    std::vector<TfToken> * const names,
    std::vector<HdDataSourceBaseHandle> * const dataSources)
{
    if (HdContainerDataSourceHandle const settingsDs =
            renderSettingsSchema.GetNamespacedSettings()) {
        _FillRileyParamsFromNamespacedSettings(
            settingsDs, names, dataSources);
    }
}

/* static */
HdPrman_RileyGlobalsSceneIndexRefPtr
HdPrman_RileyGlobalsSceneIndex::New(
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    const HdContainerDataSourceHandle &inputArgs)
{
    return TfCreateRefPtr(
        new HdPrman_RileyGlobalsSceneIndex(inputSceneIndex, inputArgs));
}

HdPrman_RileyGlobalsSceneIndex::HdPrman_RileyGlobalsSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    const HdContainerDataSourceHandle &inputArgs)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

HdPrman_RileyGlobalsSceneIndex::~HdPrman_RileyGlobalsSceneIndex() = default;

// Container of HdPrmanRileyParamSchemas.
HdContainerDataSourceHandle
HdPrman_RileyGlobalsSceneIndex::_GetRileyParams(
    HdSceneGlobalsSchema globalsSchema,
    const SdfPath &renderSettingsPath) const
{
    std::vector<TfToken> names;
    std::vector<HdDataSourceBaseHandle> dataSources;

    _FillRileyParamsFromSceneGlobals(
        globalsSchema, &names, &dataSources);

    if (!renderSettingsPath.IsEmpty()) {

        const HdSceneIndexPrim renderSettingsPrim =
            _GetInputSceneIndex()->GetPrim(renderSettingsPath);
        HdRenderSettingsSchema renderSettingsSchema =
            HdRenderSettingsSchema::GetFromParent(
                renderSettingsPrim.dataSource);

        _FillRileyParamsFromRenderSettings(
            renderSettingsSchema, &names, &dataSources);
    }

    if (names.empty()) {
        return nullptr;
    }

    return HdRetainedContainerDataSource::New(
        names.size(), names.data(), dataSources.data());
}

HdContainerDataSourceHandle
HdPrman_RileyGlobalsSceneIndex::_GetGlobalsPrimSource() const
{
    HdSceneGlobalsSchema globalsSchema = HdSceneGlobalsSchema::GetFromParent(
        _GetInputSceneIndex()->GetPrim(
            HdSceneGlobalsSchema::GetDefaultPrimPath()).dataSource);

    HdPathDataSourceHandle const renderSettingsPathDs =
        globalsSchema.GetActiveRenderSettingsPrim();

    const SdfPath renderSettingsPath =
        renderSettingsPathDs
            ? renderSettingsPathDs->GetTypedValue(0.0f)
            : SdfPath();

    return
        HdRetainedContainerDataSource::New(
            HdPrmanRileyGlobalsSchema::GetSchemaToken(),
            HdPrmanRileyGlobalsSchema::Builder()
                .SetOptions(
                    HdPrmanRileyParamListSchema::Builder()
                        .SetParams(
                            _GetRileyParams(
                                globalsSchema, renderSettingsPath))
                        .Build())
                .Build(),
            HdDependenciesSchema::GetSchemaToken(),
            _GetDependencies(renderSettingsPath));
}

HdSceneIndexPrim
HdPrman_RileyGlobalsSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    if (primPath == _GetGlobalsPrimPath()) {
        return {
            HdPrmanRileyPrimTypeTokens->globals,
            _GetGlobalsPrimSource() };
    }

    return _GetInputSceneIndex()->GetPrim(primPath);
}

SdfPathVector
HdPrman_RileyGlobalsSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    SdfPathVector result = _GetInputSceneIndex()->GetChildPrimPaths(primPath);

    if (primPath == SdfPath::AbsoluteRootPath()) {
        result.push_back(_GetGlobalsPrimPath());
    }

    return result;
}

void
HdPrman_RileyGlobalsSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    _SendPrimsAdded(entries);
}

void
HdPrman_RileyGlobalsSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

void
HdPrman_RileyGlobalsSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _SendPrimsDirtied(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
