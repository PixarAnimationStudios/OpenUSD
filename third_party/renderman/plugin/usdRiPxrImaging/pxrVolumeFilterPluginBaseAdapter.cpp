//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "usdRiPxrImaging/pxrVolumeFilterPluginBaseAdapter.h"

#include "usdRiPxrImaging/tokens.h"

#include "pxr/imaging/hd/containerDataSourceEditor.h"
#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usdImaging/usdImaging/dataSourceMaterial.h"
#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/materialParamUtils.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/usd/ar/resolverScopedCache.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (shaderId)
);

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdRiPxrImagingVolumeFilterAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

namespace {

TfToken
_ResolveVolumeFilterShaderId(
    const UsdPrim& prim,
    const TfToken& renderContext)
{
    static const TfToken volumefilterShaderId(
        SdfPath::JoinIdentifier(
            HdMaterialTerminalTokens->volumeFilter, _tokens->shaderId));

    if (!renderContext.IsEmpty()) {
        const TfToken attrName(
            SdfPath::JoinIdentifier(renderContext, volumefilterShaderId));
        if (const auto attr = prim.GetAttribute(attrName)) {
            TfToken id;
            if (attr.Get(&id) && !id.IsEmpty()) {
                return id;
            }
        }
    }

    // As a fallback, try to get the attribute without a render context.
    if (const auto attr = prim.GetAttribute(volumefilterShaderId)) {
        TfToken id;
        if (attr.Get(&id) && !id.IsEmpty()) {
            return id;
        }
    }

    return {};
}

class _VolumeFilterMaterialDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_VolumeFilterMaterialDataSource);

    TfTokenVector GetNames() override
    {
        return _materialDs->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        // Fix the shader ID, which otherwise might not be resolved properly.
        // Note: we expect the name argument to be a render context.
        auto result = _materialDs->Get(name);
        if (!result) {
            return nullptr;
        }

        // Get the correct shader ID.
        const auto shaderId = _ResolveVolumeFilterShaderId(_prim, name);
        if (shaderId.IsEmpty()) {
            return result;
        }

        auto networkDs = HdContainerDataSource::Cast(result);
        if (!networkDs) {
            return result;
        }

        return HdContainerDataSourceEditor(networkDs)
            .Set(
                HdDataSourceLocator(
                    HdMaterialNetworkSchemaTokens->nodes,
                    _prim.GetPath().GetToken(),
                    HdMaterialNodeSchemaTokens->nodeIdentifier),
                HdRetainedTypedSampledDataSource<TfToken>::New(shaderId))
            .Finish();
    }

private:
    _VolumeFilterMaterialDataSource(
        const HdContainerDataSourceHandle& materialDs,
        const UsdPrim& prim)
        : _materialDs(materialDs)
        , _prim(prim) {}

    HdContainerDataSourceHandle _materialDs;
    UsdPrim _prim;
};

} // anonymous namespace

UsdRiPxrImagingVolumeFilterAdapter::~UsdRiPxrImagingVolumeFilterAdapter()
{
}

bool
UsdRiPxrImagingVolumeFilterAdapter::IsSupported(
    UsdImagingIndexProxy const* index) const
{
    return index->IsSprimTypeSupported(
        UsdRiPxrImagingPrimTypeTokens->volumeFilter);
}

SdfPath
UsdRiPxrImagingVolumeFilterAdapter::Populate(
    UsdPrim const& prim,
    UsdImagingIndexProxy* index,
    UsdImagingInstancerContext const* instancerContext)
{
    index->InsertSprim(UsdRiPxrImagingPrimTypeTokens->volumeFilter,
                       prim.GetPath(), prim);
    HD_PERF_COUNTER_INCR(UsdRiPxrImagingPrimTypeTokens->volumeFilter);

    return prim.GetPath();
}

void
UsdRiPxrImagingVolumeFilterAdapter::_RemovePrim(
    SdfPath const& cachePath,
    UsdImagingIndexProxy* index)
{
    index->RemoveSprim(UsdRiPxrImagingPrimTypeTokens->volumeFilter, cachePath);
}

void
UsdRiPxrImagingVolumeFilterAdapter::TrackVariability(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits* timeVaryingBits,
    UsdImagingInstancerContext const* instancerContext) const
{
    // Discover time-varying transforms.
    _IsTransformVarying(prim,
        HdLight::DirtyBits::DirtyTransform,
        UsdImagingTokens->usdVaryingXform,
        timeVaryingBits);

    // Determine if the volume filter material network is time varying.
    if (UsdImagingIsHdMaterialNetworkTimeVarying(prim)) {
        *timeVaryingBits |= HdLight::DirtyBits::DirtyResource;
    }

    // If any of the attributes is time varying
    // we will assume all params are time varying.
    const std::vector<UsdAttribute> &attrs = prim.GetAttributes();
    for (UsdAttribute const& attr : attrs) {
        if (UsdGeomXformable::IsTransformationAffectedByAttrNamed(
                attr.GetName())) {
            continue;
        }
        if (attr.GetNumTimeSamples() > 1) {
            *timeVaryingBits |= HdLight::DirtyBits::DirtyParams;
            break;
        }
    }
}

void
UsdRiPxrImagingVolumeFilterAdapter::UpdateForTime(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    UsdImagingInstancerContext const* instancerContext) const
{
}

HdDirtyBits
UsdRiPxrImagingVolumeFilterAdapter::ProcessPropertyChange(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    TfToken const& propertyName)
{
    if (UsdGeomXformable::IsTransformationAffectedByAttrNamed(propertyName)) {
        return HdLight::DirtyBits::DirtyTransform;
    }
    return HdLight::DirtyBits::DirtyParams;
}

void
UsdRiPxrImagingVolumeFilterAdapter::MarkDirty(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits dirty,
    UsdImagingIndexProxy* index)
{
    index->MarkSprimDirty(cachePath, dirty);
}

void
UsdRiPxrImagingVolumeFilterAdapter::MarkTransformDirty(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    UsdImagingIndexProxy* index)
{
    static const HdDirtyBits transformDirty = HdLight::DirtyTransform;
    index->MarkSprimDirty(cachePath, transformDirty);
}

void
UsdRiPxrImagingVolumeFilterAdapter::MarkVisibilityDirty(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    UsdImagingIndexProxy* index)
{
    index->MarkSprimDirty(cachePath, HdLight::DirtyBits::DirtyParams);
}

VtValue
UsdRiPxrImagingVolumeFilterAdapter::GetMaterialResource(
    UsdPrim const &prim,
    SdfPath const& cachePath,
    UsdTimeCode time) const
{
    // Bind the usd stage's resolver context for correct asset resolution.
    ArResolverContextBinder binder(prim.GetStage()->GetPathResolverContext());
    ArResolverScopedCache resolverCache;

    HdMaterialNetworkMap networkMap;

    UsdImagingBuildHdMaterialNetworkFromTerminal(
        prim,
        HdMaterialTerminalTokens->volumeFilter,
        _GetShaderSourceTypes(),
        _GetMaterialRenderContexts(),
        &networkMap,
        time);

    // XXX -- Fix shader ID here too?

    return VtValue(networkMap);
}

TfTokenVector
UsdRiPxrImagingVolumeFilterAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdRiPxrImagingVolumeFilterAdapter::GetImagingSubprimType(
    UsdPrim const& prim,
    TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return UsdRiPxrImagingPrimTypeTokens->volumeFilter;
    }

    return TfToken();
}

HdContainerDataSourceHandle
UsdRiPxrImagingVolumeFilterAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (!subprim.IsEmpty()) {
        return nullptr;
    }

    // Overlay the material data source, which computes the node
    // network, over the base prim data source, which provides
    // other needed data like xform and visibility.
    return HdOverlayContainerDataSource::New(
        HdRetainedContainerDataSource::New(
            HdMaterialSchema::GetSchemaToken(),
            _VolumeFilterMaterialDataSource::New(
                UsdImagingDataSourceMaterial::New(
                    prim,
                    stageGlobals,
                    HdMaterialTerminalTokens->volumeFilter),
                prim)),
        UsdImagingDataSourcePrim::New(
            prim.GetPath(), prim, stageGlobals));
}

HdDataSourceLocatorSet
UsdRiPxrImagingVolumeFilterAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    HdDataSourceLocatorSet result = UsdImagingDataSourcePrim::Invalidate(
        prim, subprim, properties, invalidationType);

    if (!subprim.IsEmpty()) {
        return result;
    }

    for (const TfToken &propertyName : properties) {
        if (TfStringStartsWith(propertyName.GetString(), "inputs:")) {
            result.insert(HdMaterialSchema::GetDefaultLocator());
            break;
        }
    }

    return result;
}

UsdImagingPrimAdapter::PopulationMode
UsdRiPxrImagingVolumeFilterAdapter::GetPopulationMode()
{
    return RepresentsSelfAndDescendents;
}

HdDataSourceLocatorSet
UsdRiPxrImagingVolumeFilterAdapter::InvalidateImagingSubprimFromDescendent(
    UsdPrim const& prim,
    UsdPrim const& descendentPrim,
    TfToken const& subprim,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    HdDataSourceLocatorSet result;

    result.insert(HdMaterialSchema::GetDefaultLocator());

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
