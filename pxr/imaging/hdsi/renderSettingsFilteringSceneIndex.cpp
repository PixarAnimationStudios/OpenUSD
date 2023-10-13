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

#include "pxr/imaging/hd/cameraSchema.h"
#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/renderProductSchema.h"
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
static const SdfPath s_fallbackPath(
    "/Render/__HdsiRenderSettingsFilteringSceneIndex__FallbackSettings");

// Builds and returns a data source to invalidate the renderSettings.active
// locator when the sceneGlobals.activeRenderSettingsPrim locator is dirtied.
//
HdContainerDataSourceHandle
_BuildDependencyForActiveLocator()
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
    
    return renderSettingsDepDS;
}

// Builds and returns a data source to:
// (a) invalidate the renderSettings.shutterInterval locator when a targeted 
// camera's shutterOpen or shutterClose locator is dirtied.
// (b) invalidate the renderSettings.shutterInterval locator when the
//     renderProducts locator is dirtied. Due to flattening, we can't limit
//     this to just the cameraPrim
// (c) invalidate the prim's dependencies when the render products locator
//     is dirtied.
//
HdContainerDataSourceHandle
_BuildDependenciesForShutterInterval(
    const SdfPathVector &cameraPaths)
{
    const size_t numCameras = cameraPaths.size();
    const size_t numDependencies = numCameras*2 /* (a) */ + 2 /* (b),(c) */;

    TfTokenVector names;
    names.reserve(numDependencies);
    std::vector<HdDataSourceBaseHandle> values;
    values.reserve(numDependencies);

    static const HdDataSourceLocator shutterOpenLocator =
        HdCameraSchema::GetDefaultLocator().Append(
            HdCameraSchemaTokens->shutterOpen);
    static const HdDataSourceLocator shutterCloseLocator =
        HdCameraSchema::GetDefaultLocator().Append(
            HdCameraSchemaTokens->shutterClose);
    static const HdLocatorDataSourceHandle& shutterOpenLocatorDs =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            shutterOpenLocator);
    static const HdLocatorDataSourceHandle& shutterCloseLocatorDs =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            shutterCloseLocator);
    static const HdLocatorDataSourceHandle& shutterIntervalLocatorDs =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdRenderSettingsSchema::GetShutterIntervalLocator());
    static const HdLocatorDataSourceHandle& productsLocatorDs =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdRenderSettingsSchema::GetRenderProductsLocator());

    static const std::string shutterOpenDepPrefix(
        "renderSettings_depOn_cameraShutterOpen_");
    static const std::string shutterCloseDepPrefix(
        "renderSettings_depOn_cameraShutterClose_");

    // (a)
    for (size_t ii = 0; ii < numCameras; ++ii) {
        // shutterOpen
        names.push_back(TfToken(shutterOpenDepPrefix + std::to_string(ii)));
        values.push_back(
            HdDependencySchema::Builder()
            .SetDependedOnPrimPath(
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                    cameraPaths[ii]))
            .SetDependedOnDataSourceLocator(shutterOpenLocatorDs)
            .SetAffectedDataSourceLocator(shutterIntervalLocatorDs)
            .Build()
        );

        // shutterClose
        names.push_back(TfToken(shutterCloseDepPrefix + std::to_string(ii)));
        values.push_back(
            HdDependencySchema::Builder()
            .SetDependedOnPrimPath(
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                    cameraPaths[ii]))
            .SetDependedOnDataSourceLocator(shutterCloseLocatorDs)
            .SetAffectedDataSourceLocator(shutterIntervalLocatorDs)
            .Build()
        );
    }

    // (b)
    names.push_back(TfToken("shutterInterval_depOn_renderProducts"));
    values.push_back(
        HdDependencySchema::Builder()
        .SetDependedOnPrimPath(
            HdRetainedTypedSampledDataSource<SdfPath>::New(
                SdfPath::EmptyPath()))
        .SetDependedOnDataSourceLocator(productsLocatorDs)
        .SetAffectedDataSourceLocator(shutterIntervalLocatorDs)
        .Build()
    );

    // (c)
    names.push_back(TfToken("__dependencies_depOn_renderProducts"));
    values.push_back(
        HdDependencySchema::Builder()
        .SetDependedOnPrimPath(
            HdRetainedTypedSampledDataSource<SdfPath>::New(
                SdfPath::EmptyPath()))
        .SetDependedOnDataSourceLocator(productsLocatorDs)
        .SetAffectedDataSourceLocator(
            HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                HdDependenciesSchema::GetDefaultLocator())
        )
        .Build()
    );


    return HdRetainedContainerDataSource::New(
        names.size(), names.data(), values.data());
}

// Build and return a container with only the names that begin with the
// requested prefixes.
//
HdContainerDataSourceHandle
_GetFilteredNamespacedSettings(
    const HdContainerDataSourceHandle &c,
    const VtArray<TfToken> &prefixes)
{
    if (prefixes.empty() || !c) {
        return c;
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

bool
_Contains(
    const SdfPathVector &paths,
    const SdfPath &path)
{
    return std::find(paths.begin(), paths.end(), path) != paths.end();
}

// Return unique camera paths used by products generated by the render settings
// prim.
SdfPathVector
_GetTargetedCameras(
    HdRenderProductVectorSchema products)
{
    const HdVectorDataSourceHandle vds = products.GetVector();
    if (!vds) {
        return SdfPathVector();
    }

    SdfPathVector cameraPaths;
    for (size_t ii = 0; ii < vds->GetNumElements(); ++ii) {
        HdRenderProductSchema productSchema(
            HdContainerDataSource::Cast(vds->GetElement(ii)));
        
        const HdPathDataSourceHandle camPathDs =  productSchema.GetCameraPrim();
        if (camPathDs) {
            const SdfPath camPath = camPathDs->GetTypedValue(0.0);
            if (!camPath.IsEmpty() && !_Contains(cameraPaths, camPath)) {
                cameraPaths.push_back(camPath);
            }
        }
    }

    return cameraPaths;
}

bool
_GetCameraShutterOpenAndClose(
    const HdSceneIndexBaseRefPtr &si,
    const SdfPath cameraPath,
    GfVec2d *shutter)
{
    HdCameraSchema camSchema = 
        HdCameraSchema::GetFromParent(si->GetPrim(cameraPath).dataSource);
    
    if (!camSchema) {
        return false;
    }

    // Note: The times below are frame relative and refer to the times the
    //       shutter begins to open and is fully closed respectively.
    const HdDoubleDataSourceHandle shutterOpenDs = camSchema.GetShutterOpen();
    const HdDoubleDataSourceHandle shutterCloseDs = camSchema.GetShutterClose();

    if (shutterOpenDs && shutterCloseDs) {
        if (shutter) {
            (*shutter)[0] = shutterOpenDs->GetTypedValue(0.0);
            (*shutter)[1] = shutterCloseDs->GetTypedValue(0.0);
        }

        return true;
    }

    return false;
}

struct _ProductShutterInfo
{
    SdfPath cameraPath;
    bool disableMotionBlur;
};
using _ProductShutterInfoVec = std::vector<_ProductShutterInfo>;

_ProductShutterInfoVec
_GetShutterInfoFromProducts(
    HdRenderProductVectorSchema products)
{
    const HdVectorDataSourceHandle vds = products.GetVector();
    if (!vds) {
        return _ProductShutterInfoVec();
    }

    _ProductShutterInfoVec result;
    for (size_t ii = 0; ii < vds->GetNumElements(); ++ii) {
        HdRenderProductSchema productSchema(
            HdContainerDataSource::Cast(vds->GetElement(ii)));
        
        const HdPathDataSourceHandle camPathDs =  productSchema.GetCameraPrim();
        if (camPathDs) {
            const SdfPath camPath = camPathDs->GetTypedValue(0.0);
            if (!camPath.IsEmpty()) {
                const HdBoolDataSourceHandle disableMotionBlurDs =
                    productSchema.GetDisableMotionBlur();
                const bool disableMotionBlur
                    = disableMotionBlurDs
                    ? disableMotionBlurDs->GetTypedValue(0.0)
                    : false;

                result.push_back({camPath, disableMotionBlur});
            }
        }
    }

    return result;
}

HdVec2dDataSourceHandle
_ComputeUnionedCameraShutterInterval(
    const HdSceneIndexBaseRefPtr &si,
    const _ProductShutterInfoVec &shutterInfoVec)
{
    GfVec2d result;
    bool initialized = false;

    for (const auto &shutterInfo : shutterInfoVec) {
        const SdfPath &camPath = shutterInfo.cameraPath;
        const bool &disableMotionBlur = shutterInfo.disableMotionBlur;

        GfVec2d camShutter;
        if (_GetCameraShutterOpenAndClose(si, camPath, &camShutter)) {
            if (disableMotionBlur) {
                camShutter[0] = camShutter[1] = 0.0;
            }

            if (!initialized) {
                result = camShutter;
                initialized = true;
            } else {
                result[0] = std::min(result[0], camShutter[0]);
                result[1] = std::max(result[1], camShutter[1]);
            }
        }
    }

    if (initialized) {
        return HdRetainedTypedSampledDataSource<GfVec2d>::New(result);
    }

    return nullptr;
}

HdContainerDataSourceHandle
_BuildOverlayContainerDataSource(
    const HdContainerDataSourceHandle &src1,
    const HdContainerDataSourceHandle &src2,
    const HdContainerDataSourceHandle &src3)
{
    return
        HdOverlayContainerDataSource::OverlayedContainerDataSources(
            src1,
            HdOverlayContainerDataSource::OverlayedContainerDataSources(
                src2, src3));
}

// Data source override for the 'renderSettings' locator.
// Adds support for the 'active' and 'shutterInterval' fields and filters
// entries in the 'namespacedSettings' container.
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
        names.push_back(HdRenderSettingsSchemaTokens->shutterInterval);
        return names;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        if (name == HdRenderSettingsSchemaTokens->active) {
            bool isActive = false;
            SdfPath activePath;
            if (HdUtils::HasActiveRenderSettingsPrim(
                    _si, &activePath)) {
                isActive = (activePath == _primPath);
            }
            return HdRetainedTypedSampledDataSource<bool>::New(isActive);
        }

        if (name == HdRenderSettingsSchemaTokens->shutterInterval) {
            const _ProductShutterInfoVec shutterInfoVec =
                _GetShutterInfoFromProducts(
                    HdRenderSettingsSchema(_input).GetRenderProducts());

            return _ComputeUnionedCameraShutterInterval(_si, shutterInfoVec);
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
        const HdContainerDataSourceHandle &primDataSource,
        const HdSceneIndexBaseRefPtr &si,
        const SdfPath &primPath,
        const VtArray<TfToken> namespacePrefixes)
    : _input(primDataSource)
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
                    renderSettingsContainer,
                    _si,
                    _primPath,
                    _namespacePrefixes);
            }
        }

        if (name == HdDependenciesSchemaTokens->__dependencies) {
            const SdfPathVector cameraPaths =
                _GetTargetedCameras(
                    HdRenderSettingsSchema::GetFromParent(_input)
                    .GetRenderProducts());
            return
                _BuildOverlayContainerDataSource(
                    _BuildDependencyForActiveLocator(),
                    _BuildDependenciesForShutterInterval(cameraPaths),
                    HdContainerDataSource::Cast(result));
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
    // Avoid a copy if possible in the generic case.
    if (ARCH_UNLIKELY(
            primPath.IsAbsoluteRootPath() && _addedFallbackPrim)) {
        
        SdfPathVector paths =
            _GetInputSceneIndex()->GetChildPrimPaths(primPath);
        if (!_Contains(paths, GetRenderScope())) {
            paths.push_back(GetRenderScope());
        }
        return paths;
    }

    if (ARCH_UNLIKELY(
            primPath == GetRenderScope() && _addedFallbackPrim)) {
        
        SdfPathVector paths =
            _GetInputSceneIndex()->GetChildPrimPaths(primPath);
        paths.push_back(GetFallbackPrimPath());
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
