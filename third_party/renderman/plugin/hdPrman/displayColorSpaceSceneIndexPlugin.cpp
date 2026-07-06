//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/displayColorSpaceSceneIndexPlugin.h"

// #if PXR_VERSION >= 2308

#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/displayFilterSchema.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialNodeParameterSchema.h"
#include "pxr/imaging/hd/renderProductSchema.h"
#include "pxr/imaging/hd/renderSettingsSchema.h"
#include "pxr/imaging/hd/renderVarSchema.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/utils.h"

#include "pxr/base/gf/colorSpace.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/vt/array.h"

#include <set>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_DisplayColorSpaceSceneIndexPlugin"))

    // Render Settings Tokens 
    // Render Settings relationship property name.
    ((riDisplayFilters, "ri:displayFilters"))
    // Render Settings Dependency Name
    (namespacedSettings_depOn_renderingColorSpace)

    // Color Transform Display Filter Tokens
    (PxrColorTransformDisplayFilter)
    ((colorTransformFilterName, "__ColorTransform_DisplayFilter_"))
    // List of AOVs to be excluded by the display filter. 
    // Non-color AOVS (ex: normals, depth, etc.) should not be transformed.
    ((excludeAOVs, "ri:excludeAOVs"))
    ((colorTransform, "ri:transform"))
    // Display Filter Dependency
    (displayFilter_depOn_renderingColorSpace)
);

TF_MAKE_STATIC_DATA(
    std::vector<
        HdPrman_DisplayColorSpaceSceneIndexPlugin::ColorSpaceRemappingCallback>,
    _colorSpaceRemappingCallbacks)
{
    _colorSpaceRemappingCallbacks->clear();
}

TF_MAKE_STATIC_DATA(
    std::vector<
        HdPrman_DisplayColorSpaceSceneIndexPlugin::DisableTransformCallback>,
    _disableTransformCallbacks)
{
    _disableTransformCallbacks->clear();
}

// Helper function to determine if a dataType represents a color (as opposed to 
// vector/point/normal/float/int data). Used in _BuildExcludeAOVsList()
static bool
_IsColorType(const TfToken& dataType)
{
    // Check if dataType contains "color".
    const std::string& typeStr = dataType.GetString();
    return (typeStr.find("color") != std::string::npos);
}

// Build a list of AOV names to exclude from color transformation based on the
// render var data type. Returns AOVs that are not a color.
static VtStringArray
_BuildExcludeAOVsList(const HdContainerDataSourceHandle& rsPrimDS)
{
    HdContainerDataSourceHandle renderSettingsDS =
        HdContainerDataSource::Cast(
            rsPrimDS->Get(HdRenderSettingsSchemaTokens->renderSettings));
    if (!renderSettingsDS) {
        return {};
    }

    VtStringArray excludeAOVs;
    
    const HdRenderSettingsSchema rsSchema(renderSettingsDS);
    const HdRenderProductVectorSchema productsSchema = rsSchema.GetRenderProducts();

    // Use a set to collect unique source names.
    std::set<std::string> uniqueAOVs;

    const size_t numProducts = productsSchema.GetNumElements();
    for (size_t i = 0; i < numProducts; ++i) {
        const HdRenderProductSchema productSchema = productsSchema.GetElement(i);
        const HdRenderVarVectorSchema varsSchema = productSchema.GetRenderVars();

        const size_t numVars = varsSchema.GetNumElements();
        for (size_t j = 0; j < numVars; ++j) {
            const HdRenderVarSchema varSchema = varsSchema.GetElement(j);

            // Check dataType and exclude if NOT a color.
            const HdPathDataSourceHandle pathDs = varSchema.GetPath();
            const HdTokenDataSourceHandle dataTypeDs = varSchema.GetDataType();

            if (pathDs && dataTypeDs) {
                const TfToken dataType = dataTypeDs->GetTypedValue(0.0f);
                if (!_IsColorType(dataType)) {
                    // Use the leaf name of the render var path as the AOV name,
                    // matching how HdPrman creates AOVs.
                    const SdfPath varPath = pathDs->GetTypedValue(0.0f);
                    uniqueAOVs.insert(varPath.GetName());
                }
            }
        }
    }

    excludeAOVs.assign(uniqueAOVs.begin(), uniqueAOVs.end());

    return excludeAOVs;
}


namespace {

// Gets the authored Rendering Color Space from the given Render Settings Prim 
// data source. Defaults to LinearRec709. 
static GfColorSpace
_GetRenderingColorSpace(const HdContainerDataSourceHandle rsPrimDS)
{
    // Default to Linear Rec 709
    TfToken renderingCsName = GfColorSpaceNames->LinearRec709;
    
    HdRenderSettingsSchema rsSchema =
        HdRenderSettingsSchema::GetFromParent(rsPrimDS);
    if (rsSchema) {
        if (auto rcsHandle = rsSchema.GetRenderingColorSpace()) {
            const TfToken rcsName = rcsHandle->GetTypedValue(0);
            if (!rcsName.IsEmpty()) {
                renderingCsName = rcsName;
            }
        }
    }

    renderingCsName = HdPrman_DisplayColorSpaceSceneIndexPlugin
        ::RemapColorSpaceName(renderingCsName);

    if (!GfColorSpace::IsValid(renderingCsName)) {
        TF_WARN("Rendering Color Space name (%s) is invalid, using LinearRec709.",
                renderingCsName.GetText());
        renderingCsName = GfColorSpaceNames->LinearRec709;
    }

    return GfColorSpace(renderingCsName);
}

// Returns the path to the Color Transform Display Filter prim to be added as a
// child of a Render Settings prim with a given path. This display filter will 
// be responsible to convert the rendered pixels from the Rendering Color Space
// to the Display Color Space.
static SdfPath
_GetDisplayFilterPrimPath(const SdfPath& renderSettingsPath)
{
    return renderSettingsPath.AppendChild(_tokens->colorTransformFilterName);
}

// Uses GfColorSpace to return the 3x3 color transform matrix to transform 
// from the display color space to the rendering color space. 
static VtArray<double>
_GetColorTransformArray(
    GfColorSpace const& displayColorSpace,
    GfColorSpace const& renderingColorSpace)
{
    // The 3x3 matrix to be set in the Color Transform display filter to 
    // convert the colors from the rendering to display color space.
    GfMatrix3f transMatrix;
    if (displayColorSpace == renderingColorSpace) {
        transMatrix.SetIdentity();
    }
    else {
        transMatrix = displayColorSpace.GetRGBToRGB(renderingColorSpace);
    }
    const float* m = transMatrix.data();
    return VtArray<double>( { m[0], m[1], m[2],
                              m[3], m[4], m[5],
                              m[6], m[7], m[8] } );
}


////////////////////////////////////////////////////////////////////////////////
// Display Filter Data Sources
//

// Returns a Display Filter data source that sets the necessary attributes 
// in a Color Transform display filter in order for it to convert the input 
// colors from the Rendering Color Space to the Display Color Space, using 
// the provided list of AOVs to exclude.
static HdContainerDataSourceHandle
_GetDisplayFilterDataSource(
    const VtStringArray& excludeAOVs,
    const GfColorSpace& renderingColorSpace,
    const GfColorSpace& displayColorSpace)
{
    const VtArray<double> transformArray =
        _GetColorTransformArray(displayColorSpace, renderingColorSpace);
    const std::vector<TfToken> paramsNames = {
        _tokens->colorTransform, 
        _tokens->excludeAOVs };
    const std::vector<HdDataSourceBaseHandle> paramsValues = {
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(
                HdRetainedTypedSampledDataSource<VtArray<double>>::New(
                    transformArray))
            .Build(),
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(
                HdRetainedTypedSampledDataSource<VtArray<std::string>>::New(
                    excludeAOVs))
            .Build()
    };

    TF_VERIFY(paramsNames.size() == paramsValues.size(),
              "Parameter names and values must have the same size.");

    return 
        HdDisplayFilterSchema::Builder()
            .SetResource(
            HdMaterialNodeSchema::Builder()
                .SetNodeIdentifier(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        _tokens->PxrColorTransformDisplayFilter))
                .SetParameters(
                    HdRetainedContainerDataSource::New(
                        paramsNames.size(),
                        paramsNames.data(),
                        paramsValues.data()))
                .Build())
            .Build();
}

// Dependency Data Source to dirty the DisplayFilter Resource when the 
// Rendering Color Space on the Parent Render Settings Prim has changed. 
static HdContainerDataSourceHandle
_GetDisplayFilterDependenciesDataSource(const SdfPath& parentRenderSettingsPath)
{
    static const HdRetainedContainerDataSourceHandle dependencyDs =
        HdRetainedContainerDataSource::New(
            _tokens->displayFilter_depOn_renderingColorSpace,
            HdDependencySchema::Builder()
            .SetDependedOnPrimPath(
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                    parentRenderSettingsPath))
            .SetDependedOnDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdRenderSettingsSchema::GetRenderingColorSpaceLocator()))
            .SetAffectedDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdDisplayFilterSchema::GetResourceLocator()))
            .Build()
        );

    return dependencyDs;
}


// Display Filter prim data source that provides overrides for the
// 'resources' and '__dependencies' locators.
class _DisplayFilterPrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_DisplayFilterPrimDataSource);

    _DisplayFilterPrimDataSource(
        const SdfPath& displayFilterPath,
        const HdSceneIndexBasePtr &si,
        const GfColorSpace& displayColorSpace)
    : _si(si)
    , _displayColorSpace(displayColorSpace)
    , _displayFilterPath(displayFilterPath)
    {
        // Get the Rendering Color Space and the excludeAOV list from the
        // parent render settings prim
        const SdfPath parentRsPath = _displayFilterPath.GetParentPath();
        if (!parentRsPath.IsEmpty()) {
            const HdSceneIndexPrim rsPrim = _si->GetPrim(parentRsPath);
            if (rsPrim) {
                _rsPrimDS = rsPrim.dataSource;
            }
        }
    }

    TfTokenVector
    GetNames() override
    {
        TfTokenVector names = {
            HdDependenciesSchemaTokens->__dependencies,
            HdDisplayFilterSchemaTokens->displayFilter
        };
        return names;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result;
        
        if (name == HdDisplayFilterSchemaTokens->displayFilter) {
            return
                _GetDisplayFilterDataSource(
                    _BuildExcludeAOVsList(_rsPrimDS),
                    _GetRenderingColorSpace(_rsPrimDS),
                    _displayColorSpace);
        }
        if (name == HdDependenciesSchemaTokens->__dependencies) {
            return _GetDisplayFilterDependenciesDataSource(
                    _displayFilterPath.GetParentPath());
        }

        return result;
    }

private:
    // HdContainerDataSourceHandle _input;
    HdSceneIndexBasePtr _si;
    const GfColorSpace _displayColorSpace;
    const SdfPath _displayFilterPath;
    HdContainerDataSourceHandle _rsPrimDS;
};


////////////////////////////////////////////////////////////////////////////////
// Render Settings Data Sources
// 
// Adds a Display Filter data source to transform from the Rendering to Display
// Color Space. Adds the path to this display filter in the name in the 
// namespaced Settings data source, and necessary dependencies to make sure
// this information is updated based on the rendering color space.

// Data source override for the 'namespacedSettings' that prepends the color
// transform display filter.
class _ColorTransformNamespacedSettingsDataSource final 
    : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_ColorTransformNamespacedSettingsDataSource);

    _ColorTransformNamespacedSettingsDataSource(
        const HdContainerDataSourceHandle &input,
        const SdfPath &colorTransformFilterPath,
        const HdSceneIndexBaseRefPtr &si)
    : _input(input)
    , _colorTransformFilterPath(colorTransformFilterPath)
    , _si(si)
    {}

    TfTokenVector
    GetNames() override
    {
        TfTokenVector names = _input->GetNames();
        names.push_back(_tokens->riDisplayFilters);
        return names;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result = _input->Get(name);

        if (name == _tokens->riDisplayFilters) {

            SdfPathVector newFilterPaths = { _colorTransformFilterPath };
            if (result) {
                // Add existing filters after the color transform filter.
                const HdSampledDataSourceHandle sampledDs =
                    HdSampledDataSource::Cast(result);
                const SdfPathVector filterPaths =
                    sampledDs->GetValue(0).GetWithDefault<SdfPathVector>();
                newFilterPaths.insert(newFilterPaths.end(), filterPaths.begin(),
                                      filterPaths.end());
            }

            return HdRetainedTypedSampledDataSource<SdfPathVector>::New(
                    newFilterPaths);
        }

        return result;
    }

private:
    HdContainerDataSourceHandle _input;
    const SdfPath _colorTransformFilterPath;
    HdSceneIndexBasePtr _si;
};

// Data source override for 'renderSettings' that overrides the data source for
// the 'namespacedSettings' locator when the rendering color space is different 
// from the display color space (currently assumed to be LinearRec709).
class _RenderSettingsDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_RenderSettingsDataSource);

    _RenderSettingsDataSource(
        const HdContainerDataSourceHandle& input,
        const SdfPath& colorTransformFilterPath,
        const HdSceneIndexBasePtr &si)
    : _input(input)
    , _colorTransformFilterPath(colorTransformFilterPath)
    , _si(si)
    {}

    TfTokenVector
    GetNames() override
    {
        return _input->GetNames();
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        // Override or add the displayFilters sampled data source inside the
        // namespaced settings data source if the rendering and display color
        // spaces differ. 
        if (name == HdRenderSettingsSchemaTokens->namespacedSettings) {
            HdRenderSettingsSchema rsSchema(_input);
            if (!rsSchema.IsDefined()) {
                return nullptr;
            }

            HdContainerDataSourceHandle namespacedSettingsDS = 
                rsSchema.GetNamespacedSettings().GetContainer();

            if (namespacedSettingsDS && _NeedColorSpaceTransform(rsSchema)) {

                // NOTE: This DS just prepends the color transform path
                // inside the render settings namespaced settings 
                return _ColorTransformNamespacedSettingsDataSource::New(
                    namespacedSettingsDS, _colorTransformFilterPath, _si);
            }
        }

        return _input->Get(name);
    }

private:
    HdContainerDataSourceHandle _input;
    const SdfPath _colorTransformFilterPath;
    HdSceneIndexBasePtr _si;

    // Returns true if rendering and display color spaces are different
    bool _NeedColorSpaceTransform(const HdRenderSettingsSchema &rsSchema)
    {
        if (HdPrman_DisplayColorSpaceSceneIndexPlugin::DisableTransform(_si)) {
            return false;
        }

        const HdTokenDataSourceHandle colorSpaceDS =
            rsSchema.GetRenderingColorSpace();
        TfToken renderingCsName = (colorSpaceDS)
            ? colorSpaceDS->GetTypedValue(0)
            : GfColorSpaceNames->LinearRec709;

        renderingCsName =
            HdPrman_DisplayColorSpaceSceneIndexPlugin::RemapColorSpaceName(
                renderingCsName);
        
        // We assume the Display Color Space is LinearRec709 since we 
        // currently do not have a way to specify it. 
        return (renderingCsName != GfColorSpaceNames->LinearRec709);
    }
};

// Builds and returns a dependencies data source that declares a "self" 
// dependency on the Render Settings Prim to invalidate the 'namespacedSettings' 
// locator when the 'renderingColorSpace' locator is dirtied. This is done 
// because the connected display filters are aggregated under 
// 'namespacedSettings'.
static HdContainerDataSourceHandle
_GetRenderSettingsDependenciesDataSource(
    const HdDataSourceBaseHandle &existingDependenciesDs)
{
    static const HdRetainedContainerDataSourceHandle dependencyDs =
        HdRetainedContainerDataSource::New(
            _tokens->namespacedSettings_depOn_renderingColorSpace,
            HdDependencySchema::Builder()
            // empty path implies self-dependency.
            .SetDependedOnPrimPath(
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                    SdfPath::EmptyPath()))
            .SetDependedOnDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdRenderSettingsSchema::GetRenderingColorSpaceLocator()))
            .SetAffectedDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdRenderSettingsSchema::GetNamespacedSettingsLocator()))
            .Build()
        );

    return
        HdOverlayContainerDataSource::OverlayedContainerDataSources(
            HdContainerDataSource::Cast(existingDependenciesDs),
            dependencyDs);
}

// Render Settings Prim Data Source provides overrides for the
// 'renderSettings' (to add the child Display Filter) and '__dependencies' 
// locators (to make sure the namespaced settings are dirtied when the 
// rendering color space changes).
class _RenderSettingsPrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_RenderSettingsPrimDataSource);

    _RenderSettingsPrimDataSource(
        const HdContainerDataSourceHandle& input,
        const SdfPath& colorTransformFilterPath,
        const HdSceneIndexBasePtr &si)
    : _input(input)
    , _colorTransformFilterPath(colorTransformFilterPath)
    , _si(si)
    {}

    TfTokenVector
    GetNames() override
    {
        TfTokenVector names = _input->GetNames();
        // Duplicates are ok.
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
                        _colorTransformFilterPath, _si);
            }
        } else if (name == HdDependenciesSchemaTokens->__dependencies) {
            return _GetRenderSettingsDependenciesDataSource(result);
        }

        return result;
    }

private:
    HdContainerDataSourceHandle _input;
    const SdfPath _colorTransformFilterPath;
    HdSceneIndexBasePtr _si;
};


////////////////////////////////////////////////////////////////////////////////
// Scene Index that adds a Display Filter prim under every Render Settings prim. 
// The filter is added to the list of Display Filters connected to the 
// Render Settings prim when its Rendering Color Space is different from the 
// display color space (which is currently assumed to be LinearRec709)
TF_DECLARE_REF_PTRS(_HdPrmanDisplayColorSpaceSceneIndex);

class _HdPrmanDisplayColorSpaceSceneIndex
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    static _HdPrmanDisplayColorSpaceSceneIndexRefPtr
        New(const HdSceneIndexBaseRefPtr &inputSceneIndex)
    {
        return TfCreateRefPtr(
            new _HdPrmanDisplayColorSpaceSceneIndex(inputSceneIndex));
    }

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const
    {
        HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

        if (prim.dataSource && prim.primType == HdPrimTypeTokens->renderSettings) {
            prim.dataSource =
                _RenderSettingsPrimDataSource::New(
                    prim.dataSource, 
                    _GetDisplayFilterPrimPath(primPath),
                    _GetInputSceneIndex());
        }

        else if (primPath.GetNameToken() == _tokens->colorTransformFilterName) {
            prim.primType = HdPrimTypeTokens->displayFilter;
            prim.dataSource =
                _DisplayFilterPrimDataSource::New(
                    primPath,
                    _GetInputSceneIndex(),
                    _displayColorSpace);
        }

        return prim;
    }

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const
    {
        HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

        if (prim.primType == HdPrimTypeTokens->renderSettings) {
            // Add a Color Transform display filter prim as its child.
            SdfPathVector vec = _GetInputSceneIndex()->GetChildPrimPaths(primPath);
            vec.emplace_back(_GetDisplayFilterPrimPath(primPath));
            return vec;
        }    

        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

protected:
    _HdPrmanDisplayColorSpaceSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex)
      : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    {
    }

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries)
    {
        if (!_IsObserved()) {
            return;
        }

        HdSceneIndexObserver::AddedPrimEntries added;

        for (auto it = entries.begin(), e = entries.end(); it != e; ++it) {
            const HdSceneIndexObserver::AddedPrimEntry& entry = *it;

            // If this is a RenderSettings node, then add a Color Transform
            // display filter prim as its child.
            if (entry.primType == HdPrimTypeTokens->renderSettings) {
                // The path to the color transform prim expected under
                // this prim.
                const SdfPath colorTransformPath =
                    _GetDisplayFilterPrimPath(entry.primPath);
                added.emplace_back(
                    colorTransformPath, HdPrimTypeTokens->displayFilter);
            }
        }

        // Make sure the new Color Transform display filter prims are added to 
        // the results any was added above. Otherwise, simply return the input
        // entries.
        if (!added.empty()) {
            HdSceneIndexObserver::AddedPrimEntries allEntries(entries);
            allEntries.insert(allEntries.end(), added.begin(), added.end());
            _SendPrimsAdded(allEntries);
        
        } else {
            _SendPrimsAdded(entries);
        }
    }

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries)
    {
        _SendPrimsRemoved(entries);
    }

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries)
    {
        _SendPrimsDirtied(entries);
    }

private:
    const GfColorSpace _displayColorSpace =
        GfColorSpace(GfColorSpaceNames->LinearRec709);
};

} // annonymous namespace

////////////////////////////////////////////////////////////////////////////////
// Plugin registrations
////////////////////////////////////////////////////////////////////////////////

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_DisplayColorSpaceSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 5;

    for (auto const& pluginDisplayName : HdPrman_GetPluginDisplayNames()) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            pluginDisplayName,
            _tokens->sceneIndexPluginName,
            /* inputArgs =*/ nullptr,
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Scene Index Implementations
////////////////////////////////////////////////////////////////////////////////

HdPrman_DisplayColorSpaceSceneIndexPlugin::
HdPrman_DisplayColorSpaceSceneIndexPlugin()
{
    TfRegistryManager::GetInstance()
        .SubscribeTo<HdPrman_DisplayColorSpaceSceneIndexPlugin>();
}

/* static */
void
HdPrman_DisplayColorSpaceSceneIndexPlugin::RegisterColorSpaceRemappingCallback(
    const ColorSpaceRemappingCallback& callback)
{
    _colorSpaceRemappingCallbacks->push_back(callback);
}

TfToken
HdPrman_DisplayColorSpaceSceneIndexPlugin::RemapColorSpaceName(
    const TfToken& csName)
{
    TfToken convertedCsName = csName;
    for (const auto& callback : *_colorSpaceRemappingCallbacks) {
        convertedCsName = callback(csName);
    }
    return convertedCsName;
}

/* static */
void
HdPrman_DisplayColorSpaceSceneIndexPlugin::RegisterDisableTransformCallback(
    const DisableTransformCallback& callback)
{
    _disableTransformCallbacks->push_back(callback);
}

bool
HdPrman_DisplayColorSpaceSceneIndexPlugin::DisableTransform(
    const HdSceneIndexBaseRefPtr& si)
{
    for (const auto& callback : *_disableTransformCallbacks) {
        if (callback(si)) {
            return true;
        }
    }
    return false;
}

HdSceneIndexBaseRefPtr
HdPrman_DisplayColorSpaceSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return _HdPrmanDisplayColorSpaceSceneIndex::New(inputScene);
}


PXR_NAMESPACE_CLOSE_SCOPE

// #endif // PXR_VERSION >= 2308
