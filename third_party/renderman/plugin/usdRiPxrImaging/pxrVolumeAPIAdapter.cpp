//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "usdRiPxrImaging/pxrVolumeAPIAdapter.h"

#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/volumeSchema.h"

#include "pxr/usdImaging/usdImaging/dataSourceMaterial.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    // XXX -- Is there a central place to define this?
    //        It comes from usdRiPxr, a codeless schema.
    ((riVolumeFilters, "ri:volume:filters"))
);

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdRiPxrImagingVolumeAPIAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingAPISchemaAdapterFactory<Adapter> >();
}

namespace
{

class _VolumeDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_VolumeDataSource);

    TfTokenVector
    GetNames() override
    {
        return _GetNames();
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        if (name == HdTokens->filters) {
            SdfPathVector filterPaths;
            const auto filtersRel = _prim.GetRelationship(_tokens->riVolumeFilters);
            filtersRel.GetForwardedTargets(&filterPaths);
            return HdCreateTypedRetainedDataSource(VtValue(filterPaths));
        }

        return nullptr;
    }

private:

    static const TfTokenVector & _GetNames()
    {
        static const TfTokenVector names = {
            HdTokens->filters,
        };

        return names;
    }

    explicit _VolumeDataSource(
        const UsdPrim& prim)
      : _prim(prim)
    {
    }

    UsdPrim _prim;
};

} // namespace anonymous

TfToken
UsdRiPxrImagingVolumeAPIAdapter::GetImagingSubprimType(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName)
{
    // UsdRiPxrVolumeAPI is not a multi-apply schema.
    if (!appliedInstanceName.IsEmpty()) {
        return TfToken();
    }
    // Override the prim type only for the primary hydra prim.
    // This adapter does not define any additional subprims.
    if (!subprim.IsEmpty()) {
        return TfToken();
    }
    // Note that this opinion weakly composes with opinions from other
    // adapters servicing this prim.
    return HdVolumeSchemaTokens->volume;
}

HdContainerDataSourceHandle
UsdRiPxrImagingVolumeAPIAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (!subprim.IsEmpty() || !appliedInstanceName.IsEmpty()) {
        return nullptr;
    }

    if (subprim.IsEmpty()) {
        return HdRetainedContainerDataSource::New(
            HdMaterialSchema::GetSchemaToken(),
            UsdImagingDataSourceMaterial::New(
                prim,
                stageGlobals,
                HdMaterialTerminalTokens->volume),
            HdVolumeSchemaTokens->volume,
            _VolumeDataSource::New(prim));
    }

    return nullptr;
}


HdDataSourceLocatorSet
UsdRiPxrImagingVolumeAPIAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    if (!subprim.IsEmpty() || !appliedInstanceName.IsEmpty()) {
        return HdDataSourceLocatorSet();
    }

    HdDataSourceLocatorSet result;
    for (const TfToken &propertyName : properties) {
        if (TfStringStartsWith(propertyName.GetString(), "volume:")) {
            result.insert(HdVolumeSchema::GetDefaultLocator());
            break;
        }
    }
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
