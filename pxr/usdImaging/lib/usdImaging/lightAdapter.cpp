//
// Copyright 2017 Pixar
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
#include "pxr/usdImaging/usdImaging/lightAdapter.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingLightAdapter Adapter;
    TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    // No factory here, UsdImagingLightAdapter is abstract.
}

UsdImagingLightAdapter::~UsdImagingLightAdapter() 
{
}

void 
UsdImagingLightAdapter::TrackVariabilityPrep(UsdPrim const& prim,
                                            SdfPath const& cachePath,
                                            HdDirtyBits requestedBits,
                                            UsdImagingInstancerContext const* 
                                                instancerContext)
{    
}

void 
UsdImagingLightAdapter::TrackVariability(UsdPrim const& prim,
                                        SdfPath const& cachePath,
                                        HdDirtyBits requestedBits,
                                        HdDirtyBits* dirtyBits,
                                        UsdImagingInstancerContext const* 
                                            instancerContext)
{
}


// Thread safe.
//  * Populate dirty bits for the given \p time.
void 
UsdImagingLightAdapter::UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext)
{
}

int
UsdImagingLightAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath, 
                                      TfToken const& propertyName)
{
    return HdChangeTracker::AllDirty;
}

void
UsdImagingLightAdapter::ProcessPrimResync(SdfPath const& usdPath, 
                                         UsdImagingIndexProxy* index) 
{
    // XXX : This will become RemoveSprims when we standarize shaders/lights.
    index->RemoveLight(/*cachePath*/usdPath);
    index->RemoveDependency(/*usdPrimPath*/usdPath);

    if (_GetPrim(usdPath)) {
        // The prim still exists, so repopulate it.
        index->Repopulate(/*cachePath*/usdPath);
    }
}

void
UsdImagingLightAdapter::ProcessPrimRemoval(SdfPath const& usdPath, 
                                          UsdImagingIndexProxy* index) 
{
    // XXX : This will become RemoveSprims when we standarize shaders/lights.
    index->RemoveLight(/*cachePath*/usdPath);
    index->RemoveDependency(/*usdPrimPath*/usdPath);
}

PXR_NAMESPACE_CLOSE_SCOPE
