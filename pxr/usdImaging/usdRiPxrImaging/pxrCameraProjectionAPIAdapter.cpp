//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdRiPxrImaging/pxrCameraProjectionAPIAdapter.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/vt/value.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/cameraSchema.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/dependencySchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usdShade/connectableAPIBehavior.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/output.h"
#include "pxr/usdImaging/usdImaging/apiSchemaAdapter.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"
#include "pxr/usdImaging/usdImaging/types.h"
#include "pxr/usdImaging/usdImaging/dataSourceMapped.h"

#include "pxr/usd/usd/prim.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

#include "pxr/pxr.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (PxrCameraProjectionAPI)
    (projection)
    (outputs)
    (ri)
    ((primDepProjectionPrim, "prim_dep_projection_prim")));

namespace
{

std::pair<TfToken, TfToken>
_SplitNamespace(const TfToken &name)
{
    static const char namespaceDelimiter =
        SdfPathTokens->namespaceDelimiter.GetText()[0];
    
    const std::string &str = name.GetString();
    const size_t i = str.find(namespaceDelimiter);
    if (i == std::string::npos) {
        return { TfToken(), TfToken() };
    }

    return { TfToken(str.substr(0, i)),
             TfToken(str.substr(i + 1)) };
}

class _ConnectedPathDataSource final : public HdTypedSampledDataSource<SdfPath>
{
public:
    using Time = HdSampledDataSource::Time;
    HD_DECLARE_DATASOURCE(_ConnectedPathDataSource);

    _ConnectedPathDataSource(
        const UsdShadeOutput &output)
        : _output(output) {}

    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    SdfPath GetTypedValue(
            HdSampledDataSource::Time shutterOffset) override {
        SdfPathVector paths;
        _output.GetRawConnectedSourcePaths(&paths);
        if (paths.size() > 0) {
            return  paths[0].GetPrimPath();
        }
        return SdfPath();
    }

    bool GetContributingSampleTimesForInterval(
            HdSampledDataSource::Time startTime,
            HdSampledDataSource::Time endTime,
            std::vector<HdSampledDataSource::Time> *outSampleTimes) override {
        return false;
    }

private:
    const UsdShadeOutput _output;
};

HD_DECLARE_DATASOURCE_HANDLES(_ConnectedPathDataSource);

HdSampledDataSourceHandle
_ConnectedPathDataSourceNew(
    const UsdAttribute &usdAttr,
    const UsdImagingDataSourceStageGlobals &stageGlobals,
    const SdfPath &sceneIndexPath,
    const HdDataSourceLocator &timeVaryingFlagLocator)
{
    UsdShadeOutput output(usdAttr);
    return _ConnectedPathDataSource::New(output);
}

//
// This method or a generalization of this method might be useful for other
// adapters. Consider moving it to a more central place such as UsdImaging.
//
std::vector<UsdImagingDataSourceMapped::PropertyMapping>
_GetNamespacedPropertyMappingsForAppliedSchema(
    const TfToken &appliedSchemaName)
{
    std::vector<UsdImagingDataSourceMapped::PropertyMapping> result;

    const UsdPrimDefinition * const primDef =
        UsdSchemaRegistry::GetInstance().FindAppliedAPIPrimDefinition(
            appliedSchemaName);
    if (!primDef) {
        return result;
    }

    for (const TfToken &usdName : primDef->GetPropertyNames()) {
        std::pair<TfToken, TfToken> namespaceAndName =
            _SplitNamespace(usdName);
        if (namespaceAndName.first == _tokens->outputs) {
            namespaceAndName = 
            _SplitNamespace(namespaceAndName.second);
        }

        if (namespaceAndName.second.IsEmpty()) {
            continue;
        }

        result.push_back(
            UsdImagingDataSourceMapped::AttributeMapping{
                { usdName,
                  HdDataSourceLocator(namespaceAndName.first,
                                      namespaceAndName.second) },
                _ConnectedPathDataSourceNew});
    }

    return result;
}

const UsdImagingDataSourceMapped::PropertyMappings &
_GetMappings() {
    static const UsdImagingDataSourceMapped::PropertyMappings result(
        _GetNamespacedPropertyMappingsForAppliedSchema(
            _tokens->PxrCameraProjectionAPI),
        HdCameraSchema::GetNamespacedPropertiesLocator());
    return result;
}

}

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdRiPxrImagingCameraProjectionAPIAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter>>();
    t.SetFactory<UsdImagingAPISchemaAdapterFactory<Adapter>>();
}

HdContainerDataSourceHandle
UsdRiPxrImagingCameraProjectionAPIAdapter::GetImagingSubprimData(
    const UsdPrim& prim,
    const TfToken& subprim,
    const TfToken& appliedInstanceName,
    const UsdImagingDataSourceStageGlobals& stageGlobals)
{
    if (!subprim.IsEmpty() || !appliedInstanceName.IsEmpty()) {
        return nullptr;
    }

    static const HdDataSourceLocator projectionLocator = 
        HdDataSourceLocator(HdCameraSchemaTokens->namespacedProperties)
            .Append(_tokens->ri)
            .Append(_tokens->projection);

    static const HdDataSourceLocator fullProjectionLocator = 
    HdCameraSchema::GetDefaultLocator()
        .Append(projectionLocator);

    static const auto emptyLocatorDs =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdDataSourceLocator::EmptyLocator());

    HdContainerDataSourceHandle apiSource = HdCameraSchema::Builder()
        .SetNamespacedProperties(
            UsdImagingDataSourceMapped::New(
                prim, prim.GetPath(), _GetMappings(), stageGlobals))
        .Build();

    TfTokenVector names;
    std::vector<HdDataSourceBaseHandle> sources;

    if (auto projectionDs = HdPathDataSource::Cast(
        HdContainerDataSource::Get(apiSource, projectionLocator))) {
        const SdfPath projectionPath = projectionDs->GetTypedValue(0.f);
        if (!projectionPath.IsEmpty()) {
            auto pathDs = HdRetainedTypedSampledDataSource<SdfPath>::New(
                projectionDs->GetTypedValue(0.f));
            names.push_back(_tokens->primDepProjectionPrim);
            sources.push_back(
                HdDependencySchema::Builder()
                    .SetDependedOnDataSourceLocator(emptyLocatorDs)
                    .SetAffectedDataSourceLocator(emptyLocatorDs)
                    .SetDependedOnPrimPath(pathDs)
                    .Build()
            );
        }
    }

    HdContainerDataSourceHandle dependenciesDs = 
        HdDependenciesSchema::BuildRetained(
            names.size(),
            names.data(),
            sources.data());
    
    return
        HdRetainedContainerDataSource::New(
            HdCameraSchema::GetSchemaToken(),
            apiSource,
            HdDependenciesSchema::GetSchemaToken(),
            dependenciesDs);
}

HdDataSourceLocatorSet
UsdRiPxrImagingCameraProjectionAPIAdapter::InvalidateImagingSubprim(
    const UsdPrim& prim,
    const TfToken& subprim,
    const TfToken& appliedInstanceName,
    const TfTokenVector& properties,
    UsdImagingPropertyInvalidationType invalidationType)
{
    if (!subprim.IsEmpty() || !appliedInstanceName.IsEmpty()) {
        return HdDataSourceLocatorSet();
    }

    HdDataSourceLocatorSet inval = UsdImagingDataSourceMapped::Invalidate(
        properties, _GetMappings());
    static const HdDataSourceLocator projectionLocator = 
        HdDataSourceLocator(HdCameraSchemaTokens->namespacedProperties)
            .Append(_tokens->ri)
            .Append(_tokens->projection);
    if (inval.Contains(HdCameraSchema::GetDefaultLocator()
        .Append(projectionLocator))) {
        return { HdDataSourceLocator::EmptyLocator() };
    }
    return inval;
}

PXR_NAMESPACE_CLOSE_SCOPE
