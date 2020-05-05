//
// Copyright 2016 Pixar
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
#include "pxr/usdImaging/usdNprImaging/contourAdapter.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/implicitSurfaceMeshUtils.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usdGeom/cube.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdNpr/contour.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/usdImaging/usdNprImaging/halfEdge.h"

#include "pxr/base/tf/type.h"
#include <iostream>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingContourAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingContourAdapter::~UsdImagingContourAdapter() 
{
}

bool
UsdImagingContourAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->mesh);
}

SdfPath
UsdImagingContourAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
  UsdNprContour contour(prim);
  std::vector<UsdGeomMesh> contourSurfaces = contour.GetContourSurfaces();

  _dualMesh.reset(new UsdNprDualMesh());
  for(UsdGeomMesh& contourSurface: contourSurfaces)
  {
    UsdPrim surfacePrim = contourSurface.GetPrim();
    const UsdImagingPrimAdapterSharedPtr& adapter = 
      _GetPrimAdapter(surfacePrim, false);
    std::cout << "GET CONTOUR SURFACE PRIM ADAPTER : " << adapter.get() << std::endl;

    SdfPath cachePath = _ResolveCachePath(surfacePrim.GetPath(), nullptr);
    HdDirtyBits varyingBits = HdChangeTracker::Clean;
    adapter->TrackVariability(contourSurface.GetPrim(), cachePath, &varyingBits, nullptr );
    std::cout << "GET VARYING BITS : " << varyingBits << std::endl;

    _surfacePrims.push_back(surfacePrim);
    _dualMesh->AddMesh(contourSurface, varyingBits);
  }
  _dualMesh->Build();
  _dualMesh->ComputeOutputGeometry();
  return _AddRprim(HdPrimTypeTokens->mesh,
                    prim, index, GetMaterialUsdPath(prim), instancerContext);
}

void 
UsdImagingContourAdapter::TrackVariability(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          HdDirtyBits* timeVaryingBits,
                                          UsdImagingInstancerContext const* 
                                          instancerContext) const
{
  BaseAdapter::TrackVariability(
        prim, cachePath, timeVaryingBits, instancerContext);
  *timeVaryingBits = HdChangeTracker::AllDirty;
}

void 
UsdImagingContourAdapter::UpdateForTime(UsdPrim const& prim,
                                        SdfPath const& cachePath, 
                                        UsdTimeCode time,
                                        HdDirtyBits requestedBits,
                                        UsdImagingInstancerContext const* 
                                        instancerContext) const
{
  UsdGeomXformCache xformCache(time);
  UsdNprContour contour(prim);
  std::vector<UsdGeomMesh> contourSurfaces = contour.GetContourSurfaces();
  size_t index = 0;
  for(UsdGeomMesh& contourSurface: contourSurfaces)
  {
    char varyingBits = _dualMesh->GetMeshVaryingBits(index);
    if(varyingBits & UsdHalfEdgeMeshVaryingBits::VARYING_TOPOLOGY)
      _dualMesh->UpdateMesh(contourSurface, time, true, index);
    else if(varyingBits & UsdHalfEdgeMeshVaryingBits::VARYING_DEFORM)
      _dualMesh->UpdateMesh(contourSurface, time, false, index);
    index++;
  }

  UsdRelationship viewPointRel = contour.GetContourViewPointRel();
  if(viewPointRel.HasAuthoredTargets())
  {
    SdfPathVector viewPointTargets;
    if(viewPointRel.GetTargets(&viewPointTargets))
    {
      SdfPath viewPointPath = viewPointTargets[0];

      
      GfMatrix4d viewPointMatrix = 
        xformCache.GetLocalToWorldTransform(
          prim.GetStage()->GetPrimAtPath(viewPointPath));

      _dualMesh->SetViewPoint(GfVec3f(
        viewPointMatrix[3][0],
        viewPointMatrix[3][1],
        viewPointMatrix[3][2]
      ));
      std::cout << "VIEW POINT : (" << viewPointMatrix[3][0] << "," << 
        viewPointMatrix[3][1] << "," << viewPointMatrix[3][2] << ")" << std::endl;
    }
  }

  _dualMesh->ComputeOutputGeometry();

  BaseAdapter::UpdateForTime(
      prim, cachePath, time, requestedBits, instancerContext);

  UsdImagingValueCache* valueCache = _GetValueCache();

  if (requestedBits & HdChangeTracker::DirtyTopology) {
    std::cout << "USD NPR GET TOPOLOGY !!!" << std::endl;
    valueCache->GetTopology(cachePath) = 
      VtValue(_dualMesh->GetOutputTopology());
  }
}

// ---------------------------------------------------------------------- //
/// Change Processing
// ---------------------------------------------------------------------- //
HdDirtyBits
UsdImagingContourAdapter::ProcessPropertyChange(
  const UsdPrim& prim,
  const SdfPath& cachePath,
  const TfToken& propertyName)
{
  std::cout << "USD NPR PROCESS PROPERTY CHANGE : " << propertyName.GetText() << std::endl;
  return HdChangeTracker::Clean;
}

void
UsdImagingContourAdapter::ProcessPrimResync(
  SdfPath const& primPath,
  UsdImagingIndexProxy* index)
{
  std::cout << "USD NPR PROCESS PRIM RESYNC : " << primPath.GetText() << std::endl;
}

void
UsdImagingContourAdapter::ProcessPrimRemoval(
  SdfPath const& primPath,
  UsdImagingIndexProxy* index)
{
  std::cout << "USD NPR PROCESS PRIM REMOVAL : " << primPath.GetText() << std::endl;
  // Note: _RemovePrim removes the Hydra prim and the UsdImaging primInfo
  // entries as well (unlike the pattern followed in PrimAdapter)
  _RemovePrim(primPath, index);
}

void 
UsdImagingContourAdapter::MarkDirty(UsdPrim const& prim,
                                    SdfPath const& cachePath,
                                    HdDirtyBits dirty,
                                    UsdImagingIndexProxy* index)
{
  UsdImagingValueCache* valueCache = _GetValueCache();
  UsdStageRefPtr stage = prim.GetStage();
  //UsdImagingDelegate::_GetHdPrimInfo(const SdfPath &cachePath)
  for(const auto& surfacePrim: _surfacePrims)
  {
    SdfPath surfacePrimCachePath = _ResolveCachePath(surfacePrim.GetPath(), nullptr);
    VtValue& points = valueCache->GetPoints(surfacePrimCachePath);
    std::cout << "POINTS HASH : " << points.GetHash() << std::endl;

    std::cout << "----------------------------------------------------------------" << std::endl;
    std::cout << "CHECK DIRTY STATE FOR " << surfacePrimCachePath << " : " << cachePath.GetText() << std::endl;
  }
  index->MarkRprimDirty(cachePath, dirty);
  std::cout << "===================================================================" << std::endl;
}

/*virtual*/
VtValue
UsdImagingContourAdapter::GetPoints(UsdPrim const& prim,
                                 SdfPath const& cachePath,
                                 UsdTimeCode time) const
{
  std::cout << "USD NPR GET POINTS !!!" << std::endl;
  VtValue points(_dualMesh->GetOutputPoints());
  UsdImagingValueCache* valueCache = _GetValueCache();
  valueCache->GetPoints(cachePath) = points;
  return points;
}



PXR_NAMESPACE_CLOSE_SCOPE

