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
// You may obtain a copy of the Apache License atNAMESPACE
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usdImaging/usdImaging/dataSourceRenderPrims.h"

#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"
#include "pxr/usdImaging/usdImaging/usdRenderProductSchema.h"
#include "pxr/usdImaging/usdImaging/usdRenderSettingsSchema.h"
#include "pxr/usdImaging/usdImaging/usdRenderVarSchema.h"

#include "pxr/usd/usdRender/pass.h"
#include "pxr/usd/usdRender/product.h"
#include "pxr/usd/usdRender/settings.h"
#include "pxr/usd/usdRender/spec.h"
#include "pxr/usd/usdRender/var.h"

#include "pxr/imaging/hd/renderPassSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

static HdContainerDataSourceHandle
_ToContainerDS(const VtDictionary &dict)
{
    std::vector<TfToken> names;
    std::vector<HdDataSourceBaseHandle> values;
    const size_t numDictEntries = dict.size();
    names.reserve(numDictEntries);
    values.reserve(numDictEntries);

    for (const auto &pair : dict) {
        names.push_back(TfToken(pair.first));
        values.push_back(
            HdRetainedSampledDataSource::New(pair.second));
    }
    return HdRetainedContainerDataSource::New(
        names.size(), names.data(), values.data());
}

inline TfTokenVector
_Concat(const TfTokenVector &a, const TfTokenVector &b)
{
    TfTokenVector result;
    result.reserve(a.size() + b.size());
    result.insert(result.end(), a.begin(), a.end());
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

VtDictionary
_ComputeNamespacedSettings(const UsdPrim &prim)
{
    // Note that we don't filter by namespaces (as we do in the 1.0 API;
    // see UsdImagingRenderSettingsAdapter::Get). A downstream renderer-specific
    // scene index plugin will provide the necessary filtering instead.
    return UsdRenderComputeNamespacedSettings(
            prim, /* namespaces */ TfTokenVector());
}

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
            VtDictionary settingsDict =
                _ComputeNamespacedSettings(_usdRenderSettings.GetPrim());

            return _ToContainerDS(settingsDict);
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
            VtDictionary settingsDict =
                _ComputeNamespacedSettings(_usdRenderProduct.GetPrim());

            return _ToContainerDS(settingsDict);
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
            VtDictionary settingsDict =
                _ComputeNamespacedSettings(_usdRenderVar.GetPrim());

            return _ToContainerDS(settingsDict);

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
