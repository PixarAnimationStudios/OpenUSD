//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/coordSysAPIAdapter.h"

#include "pxr/usd/usdShade/coordSysAPI.h"
#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/imaging/hd/coordSysBindingSchema.h"
#include "pxr/imaging/hd/coordSysSchema.h"
#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/xformSchema.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdImagingCoordSysAPIAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingAPISchemaAdapterFactory<Adapter> >();
}

// ----------------------------------------------------------------------------

namespace
{

// Container with the binding for a single coord sys name. The binding is
// resolved on each query (rather than when the data source is created) so
// that edits to the binding relationship's targets are picked up after the
// coordSysBinding locator is dirtied, even by clients (such as the
// flattening scene index) that hold on to the prim's data source.
class _CoordSysBindingDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_CoordSysBindingDataSource);

    _CoordSysBindingDataSource(const UsdPrim &prim, const TfToken &name)
    : _api(prim, name) {
    }

    TfTokenVector GetNames() override {
        if (_api.GetLocalBinding().name.IsEmpty()) {
            return {};
        }
        return { _api.GetName() };
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name != _api.GetName()) {
            return nullptr;
        }
        const UsdShadeCoordSysAPI::Binding binding = _api.GetLocalBinding();
        if (binding.name.IsEmpty()) {
            return nullptr;
        }
        return HdRetainedTypedSampledDataSource<SdfPath>::New(
            binding.coordSysPrimPath);
    }

private:
    const UsdShadeCoordSysAPI _api;
};
HD_DECLARE_DATASOURCE_HANDLES(_CoordSysBindingDataSource);

} // anonymous namespace

HdContainerDataSourceHandle
UsdImagingCoordSysAPIAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (appliedInstanceName.IsEmpty()) {
        return nullptr;
    }

    if (subprim.IsEmpty()) {
        return HdRetainedContainerDataSource::New(
            HdCoordSysBindingSchemaTokens->coordSysBinding,
            _CoordSysBindingDataSource::New(prim, appliedInstanceName));
    }

    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingCoordSysAPIAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    if (appliedInstanceName.IsEmpty()) {
        return HdDataSourceLocatorSet();
    }

    if (subprim.IsEmpty()) {
        for (const TfToken &propertyName : properties) {
             // Could use coord sys name for more targeted invalidation
             // to improve performance.
            if (UsdShadeCoordSysAPI::CanContainPropertyName(propertyName)) {
                return HdCoordSysBindingSchema::GetDefaultLocator();
            }
        }
    }

    return HdDataSourceLocatorSet();
}

PXR_NAMESPACE_CLOSE_SCOPE
