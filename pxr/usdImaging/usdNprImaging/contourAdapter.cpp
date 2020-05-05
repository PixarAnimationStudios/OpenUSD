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
#include "pxr/imaging/pxOsd/tokens.h"

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
  //_dualMeshes.resize(contourSurfaces.size());
  for(int i=0;i<contourSurfaces.size();++i)
  {
    UsdNprDualMeshSharedPtr dualMesh(new UsdNprDualMesh() );
    UsdPrim surfacePrim = contourSurfaces[i].GetPrim();
    const UsdImagingPrimAdapterSharedPtr& adapter = 
      _GetPrimAdapter(surfacePrim, false);
    std::cout << "GET CONTOUR SURFACE PRIM ADAPTER : " << adapter.get() << std::endl;

    SdfPath cachePath = _ResolveCachePath(surfacePrim.GetPath(), nullptr);
    HdDirtyBits varyingBits = HdChangeTracker::Clean;
    adapter->TrackVariability(surfacePrim, cachePath, &varyingBits, nullptr );
    std::cout << "GET VARYING BITS : " << varyingBits << std::endl;

    _surfacePrims.push_back(surfacePrim);
    dualMesh->InitMesh(contourSurfaces[i], varyingBits);
    _dualMeshes.push_back(dualMesh);
  }

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
  if (requestedBits & HdChangeTracker::DirtyTopology) {
    std::cout << "CONTOUR NEED A DAMNED UPDATE !!! " << std::endl;
    UsdGeomXformCache xformCache(time);
    UsdNprContour contour(prim);
    

    UsdRelationship viewPointRel = contour.GetContourViewPointRel();
    GfMatrix4d viewPointMatrix;
    if(viewPointRel.HasAuthoredTargets())
    {
      SdfPathVector viewPointTargets;
      if(viewPointRel.GetTargets(&viewPointTargets))
      {
        SdfPath viewPointPath = viewPointTargets[0];

        viewPointMatrix = 
          xformCache.GetLocalToWorldTransform(
            prim.GetStage()->GetPrimAtPath(viewPointPath));
        
        std::cout << "VIEW POINT : (" << viewPointMatrix[3][0] << "," << 
          viewPointMatrix[3][1] << "," << viewPointMatrix[3][2] << ")" << std::endl;
      }
    }

    std::vector<UsdGeomMesh> contourSurfaces = contour.GetContourSurfaces();
    size_t index = 0;

    /*
    WorkParallelForEach(range.begin(), range.end(),
                        [this](UsdPrim const &desc) { _VisitPrim(desc); });
                        */
    
    for(UsdGeomMesh& contourSurface: contourSurfaces)
    {
      char varyingBits = _dualMeshes[index]->GetMeshVaryingBits();
      if(varyingBits & UsdHalfEdgeMeshVaryingBits::VARYING_TOPOLOGY)
        _dualMeshes[index]->UpdateMesh(contourSurface, time, true);
      else if(varyingBits & UsdHalfEdgeMeshVaryingBits::VARYING_DEFORM)
        _dualMeshes[index]->UpdateMesh(contourSurface, time, false);

      _dualMeshes[index]->SetViewPoint(GfVec3f(
          viewPointMatrix[3][0],
          viewPointMatrix[3][1],
          viewPointMatrix[3][2]
      ));

      _dualMeshes[index]->Build();
      _dualMeshes[index]->FindSilhouettes(viewPointMatrix);

      _dualMeshes[index]->ComputeOutputGeometry();
      index++;
    }

    BaseAdapter::UpdateForTime(
        prim, cachePath, time, requestedBits, instancerContext);

    UsdImagingValueCache* valueCache = _GetValueCache();

    _ComputeOutputGeometry(valueCache, cachePath);

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
  index->MarkRprimDirty(cachePath, dirty);
}

void
UsdImagingContourAdapter::_ComputeOutputGeometry(UsdImagingValueCache* valueCache, 
    SdfPath const& cachePath) const
{
  
  size_t numPoints = 0;
  size_t numCounts = 0;
  size_t numIndices = 0;
  for(const auto& dualMesh: _dualMeshes){
    numPoints += dualMesh->GetNumOutputPoints();
    numCounts += dualMesh->GetNumOutputFaceVertexCounts();
    numIndices += dualMesh->GetNumOutputFaceVertexIndices();
  }

  VtArray<int> faceVertexCounts(numCounts);
  VtArray<int> faceVertexIndices(numIndices);
  VtArray<GfVec3f> points(numPoints);

  size_t pointsIndex = 0;
  size_t countsIndex = 0;
  size_t indicesIndex = 0;

  for(const auto& dualMesh: _dualMeshes) {
    size_t numPoints =  dualMesh->GetNumOutputPoints();
    if(numPoints) {
      memcpy(&points[pointsIndex], &dualMesh->GetOutputPoints()[0], 
        numPoints * sizeof(GfVec3f));
      pointsIndex += numPoints;
    }
  
    size_t numCounts =  dualMesh->GetNumOutputFaceVertexCounts();
    if(numCounts) {
      memcpy(&faceVertexCounts[countsIndex], &dualMesh->GetOutputFaceVertexCounts()[0], 
        numCounts * sizeof(int));
      countsIndex += numCounts;
    }
    
    size_t numIndices =  dualMesh->GetNumOutputFaceVertexIndices();
    if(numIndices) {
      const VtArray<int>& src = dualMesh->GetOutputFaceVertexIndices();
      for(int i=0;i<numIndices;++i)faceVertexIndices[indicesIndex + i] = src[i] + indicesIndex;
      indicesIndex += numIndices;
    }
  }

  HdMeshTopology topology(PxOsdOpenSubdivTokens->none,
                          UsdGeomTokens->rightHanded,
                          faceVertexCounts,
                          faceVertexIndices);
  
  valueCache->GetTopology(cachePath) = VtValue(topology);
  valueCache->GetPoints(cachePath) = VtValue(points);
}

PXR_NAMESPACE_CLOSE_SCOPE

