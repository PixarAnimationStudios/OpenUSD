//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/coordSysAdapter.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hd/coordSys.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingCoordSysAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingCoordSysAdapter::~UsdImagingCoordSysAdapter() 
{
}

bool
UsdImagingCoordSysAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return index->IsSprimTypeSupported(HdPrimTypeTokens->coordSys);
}

SdfPath
UsdImagingCoordSysAdapter::Populate(UsdPrim const& usdPrim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    UsdImaging_CoordSysBindingStrategy::value_type bindings =
        _GetCoordSysBindings(usdPrim);
    if (bindings.idVecPtr) {
        SdfPathVector const& idVec = *(bindings.idVecPtr);
        std::vector<UsdShadeCoordSysAPI::Binding> const& bindingVec =
            *(bindings.usdBindingVecPtr);
        TF_VERIFY(idVec.size() == bindingVec.size());
        for (size_t i=0, n=idVec.size(); i<n; ++i) {
            // Verify that target path exists
            TF_VERIFY(_GetPrim(bindingVec[i].coordSysPrimPath));
            if (!index->IsPopulated(idVec[i])) {
                index->InsertSprim(HdPrimTypeTokens->coordSys, idVec[i],
                                   _GetPrim(bindingVec[i].coordSysPrimPath),
                                   shared_from_this());
                index->AddDependency(idVec[i],
                    _GetPrim(bindingVec[i].bindingRelPath.GetPrimPath()));
            }
        }
    }
    return SdfPath();
}

void
UsdImagingCoordSysAdapter::_RemovePrim(SdfPath const& cachePath,
                                         UsdImagingIndexProxy* index)
{
    index->RemoveSprim(HdPrimTypeTokens->coordSys, cachePath);
}

void 
UsdImagingCoordSysAdapter::TrackVariability(UsdPrim const& prim,
                                        SdfPath const& cachePath,
                                        HdDirtyBits* timeVaryingBits,
                                        UsdImagingInstancerContext const* 
                                            instancerContext) const
{
    // Discover time-varying transform on the target prim.
    _IsTransformVarying(prim,
        HdCoordSys::DirtyTransform,
        UsdImagingTokens->usdVaryingXform,
        timeVaryingBits);
}

void 
UsdImagingCoordSysAdapter::UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext) const
{
}

void
UsdImagingCoordSysAdapter::ProcessPrimResync(SdfPath const& primPath,
                                             UsdImagingIndexProxy* index)
{
    // If we get a resync notice, remove the coord sys object, and rely on
    // the delegate resync function to re-populate.
    _RemovePrim(primPath, index);
}

HdDirtyBits
UsdImagingCoordSysAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath, 
                                      TfToken const& propertyName)
{
    if (UsdGeomXformable::IsTransformationAffectedByAttrNamed(propertyName)) {
        return HdCoordSys::DirtyTransform;
    }
    return HdChangeTracker::Clean;
}

void
UsdImagingCoordSysAdapter::MarkDirty(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  HdDirtyBits dirty,
                                  UsdImagingIndexProxy* index)
{
    index->MarkSprimDirty(cachePath, dirty);
}

void
UsdImagingCoordSysAdapter::MarkTransformDirty(UsdPrim const& prim,
                                           SdfPath const& cachePath,
                                           UsdImagingIndexProxy* index)
{
    index->MarkSprimDirty(cachePath, HdCoordSys::DirtyTransform);
}

PXR_NAMESPACE_CLOSE_SCOPE
