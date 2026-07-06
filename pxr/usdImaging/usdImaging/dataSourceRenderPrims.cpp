//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/dataSourceRenderPrims.h"

#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"
#include "pxr/usdImaging/usdImaging/usdRenderProductSchema.h"
#include "pxr/usdImaging/usdImaging/usdRenderSettingsSchema.h"
#include "pxr/usdImaging/usdImaging/usdRenderVarSchema.h"

#include "pxr/usd/usdRender/pass.h"
#include "pxr/usd/usdRender/product.h"
#include "pxr/usd/usdRender/settings.h"
#include "pxr/usd/usdRender/var.h"

#include "pxr/imaging/hd/renderPassSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (RenderSettings)
    ((riIntegrator, "ri:integrator"))
    ((riSampleFilters, "ri:sampleFilters"))
    ((riDisplayFilters, "ri:displayFilters"))
    ((riEnergyFilters, "ri:energyFilters"))
);

inline TfTokenVector
_Concat(const TfTokenVector &a, const TfTokenVector &b)
{
    TfTokenVector result;
    result.reserve(a.size() + b.size());
    result.insert(result.end(), a.begin(), a.end());
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

static bool
_IsNamespacedAttribute(const UsdAttribute &attr)
{
    const std::string &name = attr.GetName().GetString();
    // Strip "outputs:" prefix (UsdShadeOutput convention) before checking
    // for a namespace delimiter.
    const std::string &basename =
        (name.size() > 8 && name.compare(0, 8, "outputs:") == 0)
            ? name.substr(8) : name;
    return basename.find(':') != std::string::npos;
}

// A container data source for "namespacedSettings" that vends each
// namespaced attribute as a live UsdImagingDataSourceAttribute, preserving
// time-varying values and registering them with the stage globals.
//
// XXX: We explicitly populate PxrRenderTerminalsAPI relationships
// to RenderSettings, avoiding populating all relationships; this
// should be moved to renderman-specific code in a future change.
// https://jira.pixar.com/browse/HYD-3280
class _DataSourceNamespacedSettings : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_DataSourceNamespacedSettings);

    TfTokenVector GetNames() override
    {
        TfTokenVector names;
        for (const UsdAttribute &attr : _prim.GetAuthoredAttributes()) {
            if (_IsNamespacedAttribute(attr)) {
                names.push_back(attr.GetName());
            }
        }
        // XXX UsdImaging should not have knowledge of RenderMan
        // specific concepts; these should be moved out to a
        // usdRiPxrImaging adapter.
        for (const TfToken &relName : {
                _tokens->riIntegrator,
                _tokens->riSampleFilters,
                _tokens->riDisplayFilters,
                _tokens->riEnergyFilters}) {
            if (UsdRelationship rel = _prim.GetRelationship(relName)) {
                if (rel.HasAuthoredTargets()) {
                    names.push_back(relName);
                }
            }
        }
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        // XXX UsdImaging should not have knowledge of RenderMan
        // specific concepts; these should be moved out to a
        // usdRiPxrImaging adapter.
        for (const TfToken &relName : {
                _tokens->riIntegrator,
                _tokens->riSampleFilters,
                _tokens->riDisplayFilters,
                _tokens->riEnergyFilters}) {
            if (name == relName) {
                if (UsdRelationship rel = _prim.GetRelationship(name)) {
                    SdfPathVector targets;
                    rel.GetTargets(&targets);
                    // XXX Surprisingly, these settings come through as
                    // std::vector even though VtArray is what is typically
                    // used in the scene index.
                    return HdRetainedTypedSampledDataSource<SdfPathVector>
                        ::New(targets);
                }
                return nullptr;
            }
        }

        if (UsdAttribute attr = _prim.GetAttribute(name)) {
            return UsdImagingDataSourceAttributeNew(
                attr, _stageGlobals, _sceneIndexPath,
                _locatorPrefix.Append(name));
        }
        return nullptr;
    }

private:
    _DataSourceNamespacedSettings(
            const SdfPath &sceneIndexPath,
            const UsdPrim &prim,
            const UsdImagingDataSourceStageGlobals &stageGlobals,
            const HdDataSourceLocator &locatorPrefix)
        : _sceneIndexPath(sceneIndexPath)
        , _prim(prim)
        , _stageGlobals(stageGlobals)
        , _locatorPrefix(locatorPrefix)
    {}

    SdfPath _sceneIndexPath;
    UsdPrim _prim;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
    HdDataSourceLocator _locatorPrefix;
};

HD_DECLARE_DATASOURCE_HANDLES(_DataSourceNamespacedSettings);

}

// ----------------------------------------------------------------------------
//                               RENDER PASS
// ----------------------------------------------------------------------------
namespace {

///
/// A container data source representing render pass
///
class _DataSourceRenderPass : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_DataSourceRenderPass);

    static
    const TfTokenVector& GetPropertyNames() {
        // We do not supply all of the UsdRenderPass attributes,
        // since some are for batch processing purposes.
        static TfTokenVector names = {
            UsdRenderTokens->passType,
            UsdRenderTokens->renderSource };
        return names;
    }

    TfTokenVector GetNames() override
    {
        return GetPropertyNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == UsdRenderTokens->passType) {
            TfToken passType;
            if (_usdRenderPass.GetPassTypeAttr().Get(&passType)) {
                return HdRetainedTypedSampledDataSource<TfToken>::New(passType);
            }
            return nullptr;
        } else if (name == UsdRenderTokens->renderSource) {
            if (UsdRelationship renderSourceRel =
                _usdRenderPass.GetRenderSourceRel()) {
                SdfPathVector targets; 
                renderSourceRel.GetForwardedTargets(&targets);
                if (!targets.empty()) {
                    return HdRetainedTypedSampledDataSource<SdfPath>::New(
                        targets[0]);
                }
            }
            return nullptr;
        }
        return nullptr;
    }

private:

    // Private constructor, use static New() instead.
    _DataSourceRenderPass(
            const SdfPath &sceneIndexPath,
            UsdRenderPass usdRenderPass,
            const UsdImagingDataSourceStageGlobals &stageGlobals)
        : _sceneIndexPath(sceneIndexPath)
        , _usdRenderPass(usdRenderPass)
        , _stageGlobals(stageGlobals)
    {}

private:
    const SdfPath _sceneIndexPath;
    UsdRenderPass _usdRenderPass;
    const UsdImagingDataSourceStageGlobals & _stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(_DataSourceRenderPass);

}

UsdImagingDataSourceRenderPassPrim::UsdImagingDataSourceRenderPassPrim(
    const SdfPath &sceneIndexPath,
    UsdPrim usdPrim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourcePrim(sceneIndexPath, usdPrim, stageGlobals)
{
}

TfTokenVector 
UsdImagingDataSourceRenderPassPrim::GetNames()
{
    // Note: Skip properties on UsdImagingDataSourcePrim.
    return { HdRenderPassSchema::GetSchemaToken() };
}

HdDataSourceBaseHandle 
UsdImagingDataSourceRenderPassPrim::Get(const TfToken & name)
{
    if (name == HdRenderPassSchema::GetSchemaToken()) {
        return _DataSourceRenderPass::New(
                    _GetSceneIndexPath(),
                    UsdRenderPass(_GetUsdPrim()),
                    _GetStageGlobals());
    } 

    // Note: Skip properties on UsdImagingDataSourcePrim.
    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingDataSourceRenderPassPrim::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties,
    const UsdImagingPropertyInvalidationType invalidationType)

{
    TRACE_FUNCTION();

    const TfTokenVector &names = _DataSourceRenderPass::GetPropertyNames();
    static TfToken::HashSet tokensSet(names.begin(), names.end());

    HdDataSourceLocatorSet locators;

    for (const TfToken &propertyName : properties) {
        if (tokensSet.find(propertyName) != tokensSet.end()) {
            locators.insert(
                HdRenderPassSchema::GetDefaultLocator()
                    .Append(propertyName));
        }
        // Note: Skip UsdImagingDataSourcePrim::Invalidate(...)
        // since none of the "base" set of properties are relevant here.
    }

    return locators;
}

// ----------------------------------------------------------------------------
//                               RENDER SETTINGS
// ----------------------------------------------------------------------------
namespace {

///
/// A container data source representing render settings info.
///
class _DataSourceRenderSettings : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_DataSourceRenderSettings);

    static
    const TfTokenVector& GetPropertyNames() {
        static TfTokenVector names = _Concat(
            UsdRenderSettings::GetSchemaAttributeNames(
                /* includeInherited = */ true),
            {   UsdImagingUsdRenderSettingsSchemaTokens->namespacedSettings,
                // Relationships need to be explicitly specified.
                UsdImagingUsdRenderSettingsSchemaTokens->camera,
                UsdImagingUsdRenderSettingsSchemaTokens->products });
        
        return names;
    }

    TfTokenVector GetNames() override
    {
        return GetPropertyNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == UsdImagingUsdRenderSettingsSchemaTokens->namespacedSettings)
        {
            return _DataSourceNamespacedSettings::New(
                _sceneIndexPath,
                _usdRenderSettings.GetPrim(),
                _stageGlobals,
                UsdImagingUsdRenderSettingsSchema::GetNamespacedSettingsLocator());
        }

        if (name == UsdImagingUsdRenderSettingsSchemaTokens->camera) {
            SdfPathVector targets; 
            _usdRenderSettings.GetCameraRel().GetForwardedTargets(&targets);
            if (!targets.empty()) {
                return HdRetainedTypedSampledDataSource<SdfPath>::New(
                    targets[0]);
            }
            return nullptr;
        }

        if (name == UsdImagingUsdRenderSettingsSchemaTokens->products) {
            SdfPathVector targets; 
            _usdRenderSettings.GetProductsRel().GetForwardedTargets(&targets);

            VtArray<SdfPath> vTargets(targets.begin(), targets.end());
            return HdRetainedTypedSampledDataSource<VtArray<SdfPath>>::New(
                    vTargets);
        }

        if (UsdAttribute attr =
                _usdRenderSettings.GetPrim().GetAttribute(name)) {

            return UsdImagingDataSourceAttributeNew(
                    attr,
                    _stageGlobals,
                    _sceneIndexPath,
                    UsdImagingUsdRenderSettingsSchema::GetDefaultLocator().
                        Append(name));
        } else {
            TF_WARN("Unhandled attribute %s in _DataSourceRenderSettings",
                    name.GetText());
            return nullptr;
        }
    }

private:

    // Private constructor, use static New() instead.
    _DataSourceRenderSettings(
            const SdfPath &sceneIndexPath,
            UsdRenderSettings usdRenderSettings,
            const UsdImagingDataSourceStageGlobals &stageGlobals)
        : _sceneIndexPath(sceneIndexPath)
        , _usdRenderSettings(usdRenderSettings)
        , _stageGlobals(stageGlobals)
    {}

private:
    const SdfPath _sceneIndexPath;
    UsdRenderSettings _usdRenderSettings;
    const UsdImagingDataSourceStageGlobals & _stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(_DataSourceRenderSettings);

}

UsdImagingDataSourceRenderSettingsPrim::UsdImagingDataSourceRenderSettingsPrim(
    const SdfPath &sceneIndexPath,
    UsdPrim usdPrim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourcePrim(sceneIndexPath, usdPrim, stageGlobals)
{
}

TfTokenVector 
UsdImagingDataSourceRenderSettingsPrim::GetNames()
{
    // Note: Skip properties on UsdImagingDataSourcePrim.
    return { UsdImagingUsdRenderSettingsSchema::GetSchemaToken() };
}

HdDataSourceBaseHandle 
UsdImagingDataSourceRenderSettingsPrim::Get(const TfToken & name)
{
    if (name == UsdImagingUsdRenderSettingsSchema::GetSchemaToken()) {
        return _DataSourceRenderSettings::New(
                    _GetSceneIndexPath(),
                    UsdRenderSettings(_GetUsdPrim()),
                    _GetStageGlobals());
    } 

    // Note: Skip properties on UsdImagingDataSourcePrim.
    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingDataSourceRenderSettingsPrim::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties,
    const UsdImagingPropertyInvalidationType invalidationType)

{
    TRACE_FUNCTION();

    const TfTokenVector &names = _DataSourceRenderSettings::GetPropertyNames();
    static TfToken::HashSet tokensSet(names.begin(), names.end());

    HdDataSourceLocatorSet locators;

    for (const TfToken &propertyName : properties) {
        if (tokensSet.find(propertyName) != tokensSet.end()) {
            locators.insert(
                UsdImagingUsdRenderSettingsSchema::GetDefaultLocator()
                    .Append(propertyName));
        } else {
            // It is likely that the property is an attribute that's
            // aggregated under "namespaced settings". For performance, skip
            // validating whether that is the case.
            locators.insert(
                UsdImagingUsdRenderSettingsSchema::
                    GetNamespacedSettingsLocator());
        }
        // Note: Skip UsdImagingDataSourcePrim::Invalidate(...)
        // since none of the "base" set of properties are relevant here.
    }

    return locators;
}

// ----------------------------------------------------------------------------
//                              RENDER PRODUCT
// ----------------------------------------------------------------------------
namespace {

///
/// A container data source representing render product info.
///
class _DataSourceRenderProduct : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_DataSourceRenderProduct);

    static
    const TfTokenVector& GetPropertyNames() {
        static TfTokenVector names = _Concat(
            UsdRenderProduct::GetSchemaAttributeNames(
                /* includeInherited = */ true),
            { UsdImagingUsdRenderProductSchemaTokens->namespacedSettings,
              // Relationships need to be explicitly specified.
              UsdImagingUsdRenderProductSchemaTokens->camera,
              UsdImagingUsdRenderProductSchemaTokens->orderedVars });

        return names;
    }

    TfTokenVector GetNames() override
    {
        return GetPropertyNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == UsdImagingUsdRenderProductSchemaTokens->namespacedSettings)
        {
            return _DataSourceNamespacedSettings::New(
                _sceneIndexPath,
                _usdRenderProduct.GetPrim(),
                _stageGlobals,
                UsdImagingUsdRenderProductSchema::GetNamespacedSettingsLocator());
        }

        if (name == UsdImagingUsdRenderProductSchemaTokens->camera) {
            SdfPathVector targets; 
            _usdRenderProduct.GetCameraRel().GetForwardedTargets(&targets);
            if (!targets.empty()) {
                return HdRetainedTypedSampledDataSource<SdfPath>::New(
                    targets[0]);
            }
            return nullptr;
        }

        if (name == UsdImagingUsdRenderProductSchemaTokens->orderedVars) {
            SdfPathVector targets; 
            _usdRenderProduct.GetOrderedVarsRel().GetForwardedTargets(&targets);

            VtArray<SdfPath> vTargets(targets.begin(), targets.end());
            return HdRetainedTypedSampledDataSource<VtArray<SdfPath>>::New(
                vTargets);
        }

        if (UsdAttribute attr =
                _usdRenderProduct.GetPrim().GetAttribute(name)) {

            // Only consider authored attributes in UsdRenderSettingsBase,
            // to allow the targeting render settings prim's opinion to be
            // inherited by the product via
            // UsdImagingRenderSettingsFlatteningSceneIndex.
            const TfTokenVector &settingsBaseTokens =
                UsdRenderSettingsBase::GetSchemaAttributeNames();
            static const TfToken::HashSet settingsBaseTokenSet(
                settingsBaseTokens.begin(), settingsBaseTokens.end());
            const bool attrInSettingsBase =
                settingsBaseTokenSet.find(name) != settingsBaseTokenSet.end();

            if (attrInSettingsBase && !attr.HasAuthoredValue()) {
                return nullptr;
            }

            return UsdImagingDataSourceAttributeNew(
                    attr,
                    _stageGlobals,
                    _sceneIndexPath,
                    UsdImagingUsdRenderProductSchema::GetDefaultLocator().
                        Append(name));
        } else {
            TF_WARN("Unhandled attribute %s in _DataSourceRenderProduct",
                    name.GetText());
            return nullptr;
        }
    }

private:

    // Private constructor, use static New() instead.
    _DataSourceRenderProduct(
            const SdfPath &sceneIndexPath,
            UsdRenderProduct usdRenderProduct,
            const UsdImagingDataSourceStageGlobals &stageGlobals)
        : _sceneIndexPath(sceneIndexPath)
        , _usdRenderProduct(usdRenderProduct)
        , _stageGlobals(stageGlobals)
    {}

private:
    const SdfPath _sceneIndexPath;
    UsdRenderProduct _usdRenderProduct;
    const UsdImagingDataSourceStageGlobals & _stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(_DataSourceRenderProduct);

}

UsdImagingDataSourceRenderProductPrim::UsdImagingDataSourceRenderProductPrim(
    const SdfPath &sceneIndexPath,
    UsdPrim usdPrim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourcePrim(sceneIndexPath, usdPrim, stageGlobals)
{
}

TfTokenVector 
UsdImagingDataSourceRenderProductPrim::GetNames()
{
    // Note: Skip properties on UsdImagingDataSourcePrim.
    return { UsdImagingUsdRenderProductSchema::GetSchemaToken() };
}

HdDataSourceBaseHandle 
UsdImagingDataSourceRenderProductPrim::Get(const TfToken & name)
{
    if (name == UsdImagingUsdRenderProductSchema::GetSchemaToken()) {
        return _DataSourceRenderProduct::New(
                    _GetSceneIndexPath(),
                    UsdRenderProduct(_GetUsdPrim()),
                    _GetStageGlobals());
    } 

    // Note: Skip properties on UsdImagingDataSourcePrim.
    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingDataSourceRenderProductPrim::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    TRACE_FUNCTION();

    const TfTokenVector &names = _DataSourceRenderProduct::GetPropertyNames();
    static TfToken::HashSet tokensSet(names.begin(), names.end());

    HdDataSourceLocatorSet locators;

    for (const TfToken &propertyName : properties) {
        if (tokensSet.find(propertyName) != tokensSet.end()) {
            locators.insert(
                UsdImagingUsdRenderProductSchema::GetDefaultLocator()
                    .Append(propertyName));
        }
        else {
            // It is likely that the property is an attribute that's
            // aggregated under "namespaced settings". For performance, skip
            // validating whether that is the case.
            locators.insert(
                UsdImagingUsdRenderProductSchema::
                    GetNamespacedSettingsLocator());
        }
        // Note: Skip UsdImagingDataSourcePrim::Invalidate(...)
        // since none of the "base" set of properties are relevant here.
    }

    return locators;
}

// ----------------------------------------------------------------------------
//                               RENDER VAR
// ----------------------------------------------------------------------------

namespace {

///
/// A container data source representing render var info.
///
class _DataSourceRenderVar : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_DataSourceRenderVar);

    static
    const TfTokenVector& GetPropertyNames() {
        static TfTokenVector names = _Concat(
            UsdRenderVar::GetSchemaAttributeNames(
                /* includeInherited = */ true),
            {UsdImagingUsdRenderVarSchemaTokens->namespacedSettings});

        return names;
    }

    TfTokenVector GetNames() override
    {
        return GetPropertyNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == UsdImagingUsdRenderVarSchemaTokens->namespacedSettings)
        {
            return _DataSourceNamespacedSettings::New(
                _sceneIndexPath,
                _usdRenderVar.GetPrim(),
                _stageGlobals,
                UsdImagingUsdRenderVarSchema::GetNamespacedSettingsLocator());
        }

        if (UsdAttribute attr =
                _usdRenderVar.GetPrim().GetAttribute(name)) {

            return UsdImagingDataSourceAttributeNew(
                    attr,
                    _stageGlobals,
                    _sceneIndexPath,
                    UsdImagingUsdRenderVarSchema::GetDefaultLocator().
                        Append(name));
        } else {
            TF_WARN("Unhandled attribute %s in _DataSourceRenderVar",
                    name.GetText());
            return nullptr;
        }
    }

private:

    // Private constructor, use static New() instead.
    _DataSourceRenderVar(
            const SdfPath &sceneIndexPath,
            UsdRenderVar usdRenderVar,
            const UsdImagingDataSourceStageGlobals &stageGlobals)
        : _sceneIndexPath(sceneIndexPath)
        , _usdRenderVar(usdRenderVar)
        , _stageGlobals(stageGlobals)
    {}

private:
    const SdfPath _sceneIndexPath;
    UsdRenderVar _usdRenderVar;
    const UsdImagingDataSourceStageGlobals & _stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(_DataSourceRenderVar);

}

UsdImagingDataSourceRenderVarPrim::UsdImagingDataSourceRenderVarPrim(
    const SdfPath &sceneIndexPath,
    UsdPrim usdPrim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourcePrim(sceneIndexPath, usdPrim, stageGlobals)
{
}

TfTokenVector 
UsdImagingDataSourceRenderVarPrim::GetNames()
{
    // Note: Skip properties on UsdImagingDataSourcePrim.
    return { UsdImagingUsdRenderVarSchema::GetSchemaToken() };
}

HdDataSourceBaseHandle 
UsdImagingDataSourceRenderVarPrim::Get(const TfToken & name)
{
    if (name == UsdImagingUsdRenderVarSchema::GetSchemaToken()) {
        return _DataSourceRenderVar::New(
                    _GetSceneIndexPath(),
                    UsdRenderVar(_GetUsdPrim()),
                    _GetStageGlobals());
    } 

    // Note: Skip properties on UsdImagingDataSourcePrim.
    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingDataSourceRenderVarPrim::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    TRACE_FUNCTION();

    const TfTokenVector &names = _DataSourceRenderVar::GetPropertyNames();
    static TfToken::HashSet tokensSet(names.begin(), names.end());

    HdDataSourceLocatorSet locators;

    for (const TfToken &propertyName : properties) {
        if (tokensSet.find(propertyName) != tokensSet.end()) {
            locators.insert(
                UsdImagingUsdRenderVarSchema::GetDefaultLocator()
                    .Append(propertyName));
        }
        else {
            // It is likely that the property is an attribute that's
            // aggregated under "namespaced settings". For performance, skip
            // validating whether that is the case.
            locators.insert(
                UsdImagingUsdRenderVarSchema::GetNamespacedSettingsLocator());
        }
    }
    // Note: Skip UsdImagingDataSourcePrim::Invalidate(...)
    // since none of the "base" set of properties are relevant here.

    return locators;
}

PXR_NAMESPACE_CLOSE_SCOPE
