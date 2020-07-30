//
// Copyright 2019 Pixar
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
        HdChangeTracker::DirtyTransform,
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
    UsdImagingValueCache* valueCache = _GetValueCache();
    if (requestedBits & HdChangeTracker::DirtyTransform) {
        // For coordinate system adapters, the UsdPrim will be the
        // UsdGeomXformable that the coordSys relationship targets,
        // and we compute its transform.
        valueCache->GetTransform(cachePath) = GetTransform(prim, time);
    }
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
        return HdChangeTracker::DirtyTransform;
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
    index->MarkSprimDirty(cachePath, HdChangeTracker::DirtyTransform);
}

PXR_NAMESPACE_CLOSE_SCOPE
