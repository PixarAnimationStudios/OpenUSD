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

#include "pxr/usdImaging/usdImaging/renderSettingsFlatteningSceneIndex.h"

#include "pxr/usdImaging/usdImaging/usdRenderProductSchema.h"
#include "pxr/usdImaging/usdImaging/usdRenderSettingsSchema.h"
#include "pxr/usdImaging/usdImaging/usdRenderVarSchema.h"

#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/renderProductSchema.h"
#include "pxr/imaging/hd/renderSettingsSchema.h"
#include "pxr/imaging/hd/renderVarSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (renderSettings_depOn_usdRenderSettings_includedPurposes)
    (renderSettings_depOn_usdRenderSettings_materialBindingPurposes)
    (renderSettings_depOn_usdRenderSettings_namespacedSettings)
    (renderSettings_depOn_usdRenderSettings_renderingColorSpace)
    (renderSettings_depOn_usdRenderSettings_resolution)
    (renderSettings_depOn_usdRenderSettings_pixelAspectRatio)
    (renderSettings_depOn_usdRenderSettings_aspectRatioConformPolicy)
    (renderSettings_depOn_usdRenderSettings_dataWindowNDC)
    (renderSettings_depOn_usdRenderSettings_disableMotionBlur)
    (renderSettings_depOn_usdRenderSettings_camera)
    (__dependencies_depOn_usdRenderSettings_products)
);

namespace {

// A fallback container data source for use when an invalid one is provided to
// avoid conditional checks in the data source overrides below.
//
class _EmptyContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_EmptyContainerDataSource);

    TfTokenVector
    GetNames() override
    {
        return {};
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        return nullptr;
    }
};

void _GetProductAndVarPaths(
    const HdContainerDataSourceHandle &settingsDs,
    const HdSceneIndexBaseRefPtr &si,
    SdfPathVector *products,
    SdfPathVector *vars)
{
    UsdImagingUsdRenderSettingsSchema usdRss =
        UsdImagingUsdRenderSettingsSchema::GetFromParent(settingsDs);

    HdPathArrayDataSourceHandle usdProductsDs = usdRss.GetProducts();
    if (!usdProductsDs) {
        return;
    }
    const VtArray<SdfPath> vProductPaths = usdProductsDs->GetTypedValue(0.0);

    SdfPathSet varPaths;

    for (const SdfPath &productPath : vProductPaths) {
        // Validate that the product prim exists ...
        const auto prodPrim = si->GetPrim(productPath);
        if (!prodPrim.dataSource) {
            continue;
        }

        // ... and has a valid data source ...
        UsdImagingUsdRenderProductSchema usdRps =
            UsdImagingUsdRenderProductSchema::GetFromParent(
                prodPrim.dataSource);
        if (!usdRps) {
            continue;
        }

        // Legit product!
        products->push_back(productPath);

        // For vars, aggregate the paths into a set and validate after looping
        // over the products.
        HdPathArrayDataSourceHandle usdVarsDs = usdRps.GetOrderedVars();
        if (usdVarsDs) {
            const VtArray<SdfPath> vVarPaths = usdVarsDs->GetTypedValue(0.0);
            varPaths.insert(vVarPaths.begin(), vVarPaths.end());
        }
    }

    for (const SdfPath &varPath : varPaths) {
        const auto varPrim = si->GetPrim(varPath);
        if (!varPrim.dataSource) {
            continue;
        }
        UsdImagingUsdRenderVarSchema usdRvs =
            UsdImagingUsdRenderVarSchema::GetFromParent(
                varPrim.dataSource);

        if (usdRvs) {
            vars->push_back(varPath);
        }
    }
}

// Build and return a data source that captures the following dependencies:
//
// 1. Changes to fields on the `__usdRenderSettings` data source should
//    be forwarded to the (flattened) `renderSettings` data source.
//
// 2. The dependencies captured in (3) and (4) below depend on the render
//    products generated by the settings prim. When this changes, the 
//    dependencies should be updated.
//
// 3. Any changes to a render product prim generated by the render settings
//    prim should dirty the `renderProducts` locator on the (flattened)
//    `renderSettings` data source (i.e., HdRenderSettingsSchema).
//
// 4. Similarly, any changes to a render var that is (transitively) generated
//    by the render settings prim should dirty the `renderProducts` locator on 
//    the (flattened) `renderSettings` data source.
//
static HdContainerDataSourceHandle
_GetRenderSettingsDependenciesDataSource(
    const HdContainerDataSourceHandle &settingsDs,
    const HdSceneIndexBaseRefPtr &si,
    const SdfPath &settingsPrimPath)
{
    if (!settingsDs) {
        return nullptr;
    }

    // Get the render products and vars generated by this settings prim.
    SdfPathVector products;
    SdfPathVector vars;
    _GetProductAndVarPaths(settingsDs, si, &products, &vars);
    
    // ------------------------------------------------------------------------
    // Build dependencies data source.
    // ------------------------------------------------------------------------

    // Populate known "self" dependencies.
    struct _SelfDependencyEntry {
        TfToken name;
        HdDataSourceLocator dependedOnLocator;
        HdDataSourceLocator affectedLocator;
    };

    static const _SelfDependencyEntry entries[] = {
        // Note: a --> b is to be read as "a depends on b".
        // renderSettings --> __usdRenderSettings
        // (1a) Schema entries that map 1:1.
        {
            _tokens->renderSettings_depOn_usdRenderSettings_includedPurposes,
            UsdImagingUsdRenderSettingsSchema::GetIncludedPurposesLocator(),
            HdRenderSettingsSchema::GetIncludedPurposesLocator(),
        },
        {
            _tokens->renderSettings_depOn_usdRenderSettings_materialBindingPurposes,
            UsdImagingUsdRenderSettingsSchema::GetMaterialBindingPurposesLocator(),
            HdRenderSettingsSchema::GetMaterialBindingPurposesLocator(),
        },
        {
            _tokens->renderSettings_depOn_usdRenderSettings_namespacedSettings,
            UsdImagingUsdRenderSettingsSchema::GetNamespacedSettingsLocator(),
            HdRenderSettingsSchema::GetNamespacedSettingsLocator(),
        },
        {
            _tokens->renderSettings_depOn_usdRenderSettings_renderingColorSpace,
            UsdImagingUsdRenderSettingsSchema::GetRenderingColorSpaceLocator(),
            HdRenderSettingsSchema::GetRenderingColorSpaceLocator(),
        },
        // (1b) USD render product-related entries that map to the flattened
        //     render products locator.
        {
            _tokens->renderSettings_depOn_usdRenderSettings_resolution,
            UsdImagingUsdRenderSettingsSchema::GetResolutionLocator(),
            HdRenderSettingsSchema::GetRenderProductsLocator(),
        },
        {
            _tokens->renderSettings_depOn_usdRenderSettings_pixelAspectRatio,
            UsdImagingUsdRenderSettingsSchema::GetPixelAspectRatioLocator(),
            HdRenderSettingsSchema::GetRenderProductsLocator(),
        },
        {
            _tokens->renderSettings_depOn_usdRenderSettings_aspectRatioConformPolicy,
            UsdImagingUsdRenderSettingsSchema::GetAspectRatioConformPolicyLocator(),
            HdRenderSettingsSchema::GetRenderProductsLocator(),
        },
        {
            _tokens->renderSettings_depOn_usdRenderSettings_dataWindowNDC,
            UsdImagingUsdRenderSettingsSchema::GetDataWindowNDCLocator(),
            HdRenderSettingsSchema::GetRenderProductsLocator(),
        },
        {
            _tokens->renderSettings_depOn_usdRenderSettings_disableMotionBlur,
            UsdImagingUsdRenderSettingsSchema::GetDisableMotionBlurLocator(),
            HdRenderSettingsSchema::GetRenderProductsLocator(),
        },
        {
            _tokens->renderSettings_depOn_usdRenderSettings_camera,
            UsdImagingUsdRenderSettingsSchema::GetCameraLocator(),
            HdRenderSettingsSchema::GetRenderProductsLocator(),
        },
        // (2) __dependencies --> __usdRenderSettings
        {
            _tokens->__dependencies_depOn_usdRenderSettings_products,
            UsdImagingUsdRenderSettingsSchema::GetProductsLocator(),
            HdDependenciesSchema::GetDefaultLocator(),
        },
    };

    std::vector<TfToken> names;
    std::vector<HdDataSourceBaseHandle> values;
    const size_t numDependencies =
        TfArraySize(entries) + products.size() + vars.size();
    names.reserve(numDependencies);
    values.reserve(numDependencies);

    // (1 & 2) Add "self" dependencies that we compiled above.
    for (auto const & entry : entries) {
        names.push_back(entry.name);
        values.push_back(
            HdDependencySchema::Builder()
            .SetDependedOnPrimPath(
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                    settingsPrimPath))
            .SetDependedOnDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    entry.dependedOnLocator))
            .SetAffectedDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    entry.affectedLocator))
            .Build()
        );
    }

    // (3) Add renderSettings --> renderProduct dependencies
    const std::string prefix("renderSettings_depOn_");
    for (size_t pid = 0; pid < products.size(); ++pid) {
        const std::string depName = prefix + "product_" + std::to_string(pid);
        names.push_back(TfToken(depName));

        values.push_back(
            HdDependencySchema::Builder()
            .SetDependedOnPrimPath(
                HdRetainedTypedSampledDataSource<SdfPath>::New(products[pid]))
            .SetDependedOnDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    UsdImagingUsdRenderProductSchema::GetDefaultLocator()))
            .SetAffectedDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdRenderSettingsSchema::GetRenderProductsLocator()))
            .Build()
        );
    }

    // (4) Add renderSettings --> renderVar dependencies.
    for (size_t vid = 0; vid < vars.size(); ++vid) {
        const std::string depName = prefix + "var_" + std::to_string(vid);
        names.push_back(TfToken(depName));

        values.push_back(
            HdDependencySchema::Builder()
            .SetDependedOnPrimPath(
                HdRetainedTypedSampledDataSource<SdfPath>::New(vars[vid]))
            .SetDependedOnDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    UsdImagingUsdRenderVarSchema::GetDefaultLocator()))
            .SetAffectedDataSourceLocator(
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdRenderSettingsSchema::GetRenderProductsLocator()))
            .Build()
        );
    }

    return HdRetainedContainerDataSource::New(
        names.size(), names.data(), values.data());
}

// XXX: Schema arguments below should be const &, but getter member
// functions on the schema aren't const.
HdDataSourceBaseHandle
_ToHdRenderVarDS(
    UsdImagingUsdRenderVarSchema &var,
    const SdfPath &varPath)
{
    return
        HdRenderVarSchema::Builder()
        .SetPath(HdRetainedTypedSampledDataSource<SdfPath>::New(varPath))
        .SetDataType(var.GetDataType())
        // HdRenderVarSchema uses token for sourceName while
        // UsdImagingUsdRenderVarSchema mimics the UsdRenderVar schema and
        // uses string.
        .SetSourceName(
            HdRetainedTypedSampledDataSource<TfToken>::New(
                TfToken(var.GetSourceName()->GetTypedValue(0))))
        .SetSourceType(var.GetSourceType())
        .SetNamespacedSettings(var.GetNamespacedSettings())
        .Build();
}

// The UsdRender OM has UsdRenderSettings and UsdRenderProduct share a common
// "base" set of properties. For value resolution, if the product has an
// authored opinion, that wins; else the settings opinion is used. Note that
// this isn't driven by prim inheritance in scene description (i.e., the 
// RenderSettings and RenderProduct prims don't need to inherit from a common
// RenderSettingsBase prim).
//
template <typename T>
T _Resolve(const T& productDsHandle, const T &settingsDsHandle)
{
    return productDsHandle? productDsHandle : settingsDsHandle;
}

// XXX: Schema arguments below should be const &, but getter member
// functions on the schema aren't const.
HdDataSourceBaseHandle
_ToHdRenderProductDS(
    UsdImagingUsdRenderSettingsSchema &settings,
    UsdImagingUsdRenderProductSchema &product,
    const SdfPath &productPath,
    std::vector<HdDataSourceBaseHandle> &vars)
{
    UsdImagingUsdRenderSettingsSchema &s = settings;
    UsdImagingUsdRenderProductSchema &p = product;
    return
        HdRenderProductSchema::Builder()
        .SetPath(HdRetainedTypedSampledDataSource<SdfPath>::New(productPath))
        .SetType(p.GetProductType())
        .SetName(p.GetProductName())
        .SetResolution(_Resolve(p.GetResolution(), s.GetResolution()))
        .SetRenderVars(
            HdRetainedSmallVectorDataSource::New(vars.size(), vars.data()))
        .SetCameraPrim(_Resolve(p.GetCamera(), s.GetCamera()))
        .SetPixelAspectRatio(
            _Resolve(p.GetPixelAspectRatio(), s.GetPixelAspectRatio()))
        .SetAspectRatioConformPolicy(
            _Resolve(p.GetAspectRatioConformPolicy(),
                     s.GetAspectRatioConformPolicy()))
        .SetApertureSize(/*XXX*/nullptr)
        .SetDataWindowNDC(
            _Resolve(p.GetDataWindowNDC(), s.GetDataWindowNDC()))
        .SetDisableMotionBlur(
            _Resolve(p.GetDisableMotionBlur(), s.GetDisableMotionBlur()))
        .SetNamespacedSettings(p.GetNamespacedSettings())
        .Build();
}

HdDataSourceBaseHandle
_FlattenRenderProducts(
    HdContainerDataSourceHandle const &settingsPrimDs,
    const HdSceneIndexBaseRefPtr &si)
{
    if (!settingsPrimDs) {
        return nullptr;
    }

    UsdImagingUsdRenderSettingsSchema usdRss =
        UsdImagingUsdRenderSettingsSchema::GetFromParent(settingsPrimDs);
    
    HdPathArrayDataSourceHandle usdProductsDs = usdRss.GetProducts();
    if (!usdProductsDs) {
        return nullptr;
    }
    const VtArray<SdfPath> productPaths = usdProductsDs->GetTypedValue(0.0);

    std::vector<HdDataSourceBaseHandle> hdProductsDs;
    hdProductsDs.reserve(productPaths.size());

    for (const SdfPath &productPath : productPaths) {
        const auto prodPrim = si->GetPrim(productPath);
        if (!prodPrim.dataSource) {
            continue;
        }

        UsdImagingUsdRenderProductSchema usdRps =
            UsdImagingUsdRenderProductSchema::GetFromParent(
                prodPrim.dataSource);
        if (!usdRps) {
            continue;
        }

        HdPathArrayDataSourceHandle usdVarsDs = usdRps.GetOrderedVars();
        const VtArray<SdfPath> varsPaths = usdVarsDs->GetTypedValue(0.0);
        std::vector<HdDataSourceBaseHandle> hdVarsDs;
        hdVarsDs.reserve(varsPaths.size());
        for (const SdfPath &varPath : varsPaths) {
            const auto varPrim = si->GetPrim(varPath);
            if (!varPrim.dataSource) {
                continue;
            }
            UsdImagingUsdRenderVarSchema usdRvs =
                UsdImagingUsdRenderVarSchema::GetFromParent(
                    varPrim.dataSource);

            if (usdRvs) {
                hdVarsDs.push_back(_ToHdRenderVarDS(usdRvs, varPath));
            }
        }

        hdProductsDs.push_back(
            _ToHdRenderProductDS(usdRss, usdRps, productPath, hdVarsDs));
    }

    return HdRetainedSmallVectorDataSource::New(
        hdProductsDs.size(), hdProductsDs.data());
}

// Flattened render settings representation.
//
class _RenderSettingsDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_RenderSettingsDataSource);

    _RenderSettingsDataSource(
        const HdContainerDataSourceHandle &settingsPrimDs,
        const HdSceneIndexBaseRefPtr &si)
    : _input(settingsPrimDs)
    , _si(si)
    {
        if (ARCH_UNLIKELY(!settingsPrimDs)) {
            TF_CODING_ERROR("Invalid container data source input provided.");
            _input = _EmptyContainerDataSource::New();
        }
    }

    TfTokenVector
    GetNames() override
    {
        // Note: 'active' is skipped here; it will be handled in a standalone
        //       scene index to accommodate emulation.
        static TfTokenVector names = {
                HdRenderSettingsSchemaTokens->namespacedSettings,
                HdRenderSettingsSchemaTokens->renderProducts,
                HdRenderSettingsSchemaTokens->includedPurposes,
                HdRenderSettingsSchemaTokens->materialBindingPurposes,
                HdRenderSettingsSchemaTokens->renderingColorSpace};
        
        return names;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        if (name == HdRenderSettingsSchemaTokens->namespacedSettings) {
            UsdImagingUsdRenderSettingsSchema usdRss =
                UsdImagingUsdRenderSettingsSchema::GetFromParent(_input);
            return usdRss.GetNamespacedSettings();
        }

        if (name == HdRenderSettingsSchemaTokens->renderProducts) {
            return _FlattenRenderProducts(_input, _si);
        }

        if (name == HdRenderSettingsSchemaTokens->includedPurposes) {
            UsdImagingUsdRenderSettingsSchema usdRss =
                UsdImagingUsdRenderSettingsSchema::GetFromParent(_input);
            return usdRss.GetIncludedPurposes();
        }

        if (name == HdRenderSettingsSchemaTokens->materialBindingPurposes) {
            UsdImagingUsdRenderSettingsSchema usdRss =
                UsdImagingUsdRenderSettingsSchema::GetFromParent(_input);
            return usdRss.GetMaterialBindingPurposes();
        }

        if (name == HdRenderSettingsSchemaTokens->renderingColorSpace) {
            UsdImagingUsdRenderSettingsSchema usdRss =
                UsdImagingUsdRenderSettingsSchema::GetFromParent(_input);
            return usdRss.GetRenderingColorSpace();
        }

        return _input->Get(name);
    }

private:
    HdContainerDataSourceHandle _input;
    const HdSceneIndexBaseRefPtr _si;
};

// Prim data source override that adds the flattened representation for
// backend/emulation consumption and dependencies for notice forwarding. 
// 
class _RenderSettingsPrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_RenderSettingsPrimDataSource);

    _RenderSettingsPrimDataSource(
        const HdContainerDataSourceHandle &input,
        const HdSceneIndexBaseRefPtr &si,
        const SdfPath &primPath)
    : _input(input)
    , _si(si)
    , _primPath(primPath)
    {
        if (ARCH_UNLIKELY(!input)) {
            TF_CODING_ERROR("Invalid container data source input provided.");
            _input = _EmptyContainerDataSource::New();
        }
    }

    TfTokenVector
    GetNames() override
    {
        TfTokenVector names = _input->GetNames();
        names.push_back(HdRenderSettingsSchema::GetSchemaToken());
        names.push_back(HdDependenciesSchema::GetSchemaToken());
        return names;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        if (name == HdRenderSettingsSchema::GetSchemaToken()) {
            return _RenderSettingsDataSource::New(_input, _si);

        }
        if (name == HdDependenciesSchema::GetSchemaToken()) {
            return _GetRenderSettingsDependenciesDataSource(
                        _input, _si, _primPath);
        }

        return _input->Get(name);
    }


private:
    HdContainerDataSourceHandle _input;
    const HdSceneIndexBaseRefPtr _si;
    SdfPath _primPath;
};

} // anonymous namespace

// -------------------------------------------------------------------------- //

/* static */
UsdImagingRenderSettingsFlatteningSceneIndexRefPtr
UsdImagingRenderSettingsFlatteningSceneIndex::New(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
{
    return TfCreateRefPtr(
        new UsdImagingRenderSettingsFlatteningSceneIndex(inputSceneIndex));
}

UsdImagingRenderSettingsFlatteningSceneIndex::
UsdImagingRenderSettingsFlatteningSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

HdSceneIndexPrim
UsdImagingRenderSettingsFlatteningSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (prim.primType == HdPrimTypeTokens->renderSettings) {
        // Override to add the flattened hydra render settings data source and
        // dependencies for notice forwarding.
        prim.dataSource = _RenderSettingsPrimDataSource::New(
            prim.dataSource, _GetInputSceneIndex(), primPath);
    }

    return prim;
}

SdfPathVector
UsdImagingRenderSettingsFlatteningSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
UsdImagingRenderSettingsFlatteningSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    // Note: We could suppress notices that add renderProduct and renderVar
    //       prims here.
    _SendPrimsAdded(entries);
}

void
UsdImagingRenderSettingsFlatteningSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    // Note: In USD, if a targeted prim (e.g., product or var) is removed,
    //       the relationship connections on the targeting prim (e.g. settings
    //       or product) aren't updated. So, when a product is removed, we won't
    //       receive a change notice that `products` under `__usdRenderSettings`
    //       has changed.
    //       XXX The dependency forwarding scene index doesn't handle this
    //       scenario yet. Update this comment once addressed.

    _SendPrimsRemoved(entries);
}

void
UsdImagingRenderSettingsFlatteningSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    // Nothing to do here since we rely on the dependency forwarding scene index
    // to flag the affected data source locator(s) on the flattened data
    // source.
    _SendPrimsDirtied(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
