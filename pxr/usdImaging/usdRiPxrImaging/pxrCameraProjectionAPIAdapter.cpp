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
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usdRi/tokens.h"
#include "pxr/usd/usdShade/connectableAPIBehavior.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/output.h"
#include "pxr/usd/usdShade/tokens.h"
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

TF_DEFINE_ENV_SETTING(
    LEGACY_PXR_CAMERA_PROJECTION_TERMINAL_ALLOWED_AND_WARN, true,
    "By default, we warn and allow specification of connections for the camera "
    "projection to propagate to the camera prim. We require relationships to "
    "specify the projection propagation to the camera instead. If set to true, "
    "warnings will be emitted when connections are used, otherwise connections "
    "will be disallowed.");

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (PxrCameraProjectionAPI)
    (projection)
    ((primDepProjectionPrim, "prim_dep_projection_prim")));

namespace
{

TfToken
_RelName()
{
    static const TfToken relName = TfToken(SdfPath::JoinIdentifier(
        UsdRiTokens->renderContext, _tokens->projection));
    return relName;
}

TfToken
_AttrName()
{
    static const TfToken attrName = TfToken(SdfPath::JoinIdentifier(
        TfToken(UsdShadeTokens->outputs.GetString() + 
                UsdRiTokens->renderContext.GetString()),
        _tokens->projection));
    return attrName;
}

bool 
_RelInSchema()
{
    using UsdPropertyDefinition = UsdPrimDefinition::Property;
    const UsdPrimDefinition * const camProjAPIDef =
        UsdSchemaRegistry::GetInstance().FindAppliedAPIPrimDefinition(
            _tokens->PxrCameraProjectionAPI);
    if (!camProjAPIDef) {
        return false;
    }
    if (camProjAPIDef->GetPropertyDefinition(_RelName())) {
        return true;
    }
    return false;
}

// A unified data source to get the projection path from either a ri:projection
// relationship or the now deprecated outputs:ri:projection attribute
// connection.
// We only fallback to the attribute connection if:
// - LEGACY_PXR_CAMERA_PROJECTION_TERMINAL_ALLOWED_AND_WARN is true 
//   the relationship spec is not found on the prim, or if relationship spec is
//   found, but relationship is not authored on the prim, in which case we warn 
//   and use the attribute connection.
//
class _ProjectionPathDataSource final : public HdTypedSampledDataSource<SdfPath>
{
public:
    using Time = HdSampledDataSource::Time;
    HD_DECLARE_DATASOURCE(_ProjectionPathDataSource);

    _ProjectionPathDataSource(const UsdPrim &prim) : _prim(prim)
    {
    }

    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    SdfPath GetTypedValue(HdSampledDataSource::Time shutterOffset) override {

        // Try to use the relationship first.
        if (_RelInSchema()) {
            if (const UsdRelationship rel = _prim.GetRelationship(_RelName())) {
                if (rel.HasAuthoredTargets()) {
                    std::vector<SdfPath> targets;
                    rel.GetTargets(&targets);
                    if (targets.size() > 0) {
                        return targets[0];
                    }
                }
            }
        }

        // Fallback to the attribute connection for legacy support.
        if (TfGetEnvSetting(
                LEGACY_PXR_CAMERA_PROJECTION_TERMINAL_ALLOWED_AND_WARN)) {
            if (UsdAttribute attr = _prim.GetAttribute(_AttrName())) {
                if (attr.HasAuthoredConnections()) {
                    UsdShadeOutput output(attr);
                    SdfPathVector paths;
                    output.GetRawConnectedSourcePaths(&paths);
                    if (paths.size() > 0) {
                        TF_WARN("%s on PxrCameraProjectionAPI is deprecated in "
                                "favor of %s relationship. Please update your "
                                "USD assets accordingly.",
                                _AttrName().GetText(), _RelName().GetText());
                        return paths[0].GetPrimPath();
                    }
                }
            }
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
    const UsdPrim _prim;
};

HD_DECLARE_DATASOURCE_HANDLES(_ProjectionPathDataSource);

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

    static const auto emptyLocatorDs =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdDataSourceLocator::EmptyLocator());

    HdDataSourceBaseHandle projectionDs = _ProjectionPathDataSource::New(prim);

    HdContainerDataSourceHandle namespacedPropertiesDs =
        HdRetainedContainerDataSource::New(
            UsdRiTokens->renderContext,
            HdRetainedContainerDataSource::New(
                _tokens->projection,
                projectionDs));

    HdContainerDataSourceHandle apiSource =
        HdCameraSchema::Builder()
            .SetNamespacedProperties(namespacedPropertiesDs)
            .Build();

    TfTokenVector names;
    std::vector<HdDataSourceBaseHandle> sources;

    if (projectionDs) {
        const SdfPath projectionPath = 
            HdPathDataSource::Cast(projectionDs)->GetTypedValue(0.f);
        if (!projectionPath.IsEmpty()) {
            auto pathDs = HdRetainedTypedSampledDataSource<SdfPath>::New(
                projectionPath);
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

    bool projectionChanged = false;

    if (_RelInSchema()) {
        for (const TfToken &prop : properties) {
            if (prop == _RelName()) {
                projectionChanged = true;
                break;
            }
        }
    }

    // Deprecated attribute connection support.
    if (!projectionChanged &&
        TfGetEnvSetting(
            LEGACY_PXR_CAMERA_PROJECTION_TERMINAL_ALLOWED_AND_WARN)) {
        for (const TfToken &prop : properties) {
            if (prop == _AttrName()) {
                projectionChanged = true;
                break;
            }
        }
    }

    if (projectionChanged) {
        return { HdDataSourceLocator::EmptyLocator() };
    }

    return HdDataSourceLocatorSet();
}

PXR_NAMESPACE_CLOSE_SCOPE
