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
  std::vector<UsdPrim> contourSurfaces = contour.GetContourSurfaces();
  for(int i=0;i<contourSurfaces.size();++i)
  {
    SdfPath contourSurfacePath = contourSurfaces[i].GetPath();
    if(_dualMeshes.find(contourSurfacePath) == _dualMeshes.end())
    {
      UsdNprDualMeshSharedPtr dualMesh(new UsdNprDualMesh());

      const UsdImagingPrimAdapterSharedPtr& adapter = 
        _GetPrimAdapter(contourSurfaces[i], false);

      SdfPath cachePath = _ResolveCachePath(contourSurfaces[i].GetPath(), nullptr);
      HdDirtyBits varyingBits = HdChangeTracker::Clean;
      adapter->TrackVariability(contourSurfaces[i], cachePath, &varyingBits, nullptr );
      
      dualMesh->InitMesh(UsdGeomMesh(contourSurfaces[i]), varyingBits);
      dualMesh->Build();
      _dualMeshes[contourSurfacePath] = dualMesh;
    }
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

    BaseAdapter::UpdateForTime(
        prim, cachePath, time, requestedBits, instancerContext);

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
      }
    }

    std::vector<UsdPrim> contourSurfaces = contour.GetContourSurfaces();
    UsdNprOutputBufferVector outputBuffers(contourSurfaces.size());

    /*
    WorkParallelForEach(range.begin(), range.end(),
                        [this](UsdPrim const &desc) { _VisitPrim(desc); });
                        */
    size_t index = 0;
    for(const UsdPrim& contourSurface: contourSurfaces)
    {
      SdfPath contourSurfacePath = contourSurface.GetPath();

      UsdNprDualMeshMap::const_iterator it = _dualMeshes.find(contourSurfacePath);
      if(it != _dualMeshes.end())
      {
        UsdNprDualMeshSharedPtr dualMesh = it->second;

        if(dualMesh->GetLastTime() != time)
        {
          char varyingBits = dualMesh->GetMeshVaryingBits();
          if(varyingBits & UsdHalfEdgeMeshVaryingBits::VARYING_TOPOLOGY)
          {
            dualMesh->UpdateMesh(UsdGeomMesh(contourSurface), time, true);
            dualMesh->Build();
          }
            
          else if(varyingBits & UsdHalfEdgeMeshVaryingBits::VARYING_DEFORM)
          {
            dualMesh->UpdateMesh(UsdGeomMesh(contourSurface), time, false);
            dualMesh->Build();
          }
          dualMesh->SetMatrix(xformCache.GetLocalToWorldTransform(contourSurface));
          dualMesh->SetLastTime(time);
        }

        std::vector<const UsdNprHalfEdge*> silhouettes;
        size_t numDualEdges = dualMesh->GetNumDualEdges();
        std::vector<bool> checked(numDualEdges);
        memset(&checked[0], 0, numDualEdges * sizeof(bool));

        dualMesh->FindSilhouettes(viewPointMatrix, silhouettes, checked);

        UsdNprOutputBuffer* outputBuffer = &outputBuffers[index];

        dualMesh->ComputeOutputGeometry(
          silhouettes,
          GfVec3f(
            viewPointMatrix[3][0],
            viewPointMatrix[3][1],
            viewPointMatrix[3][2]
          ), 
          outputBuffer->points, 
          outputBuffer->faceVertexCounts,
          outputBuffer->faceVertexIndices
        );
      }
      
      index++;
    }

    UsdImagingValueCache* valueCache = _GetValueCache();

    _ComputeOutputGeometry(outputBuffers, valueCache, cachePath);

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
UsdImagingContourAdapter::_ComputeOutputGeometry(const UsdNprOutputBufferVector& buffers,
  UsdImagingValueCache* valueCache, SdfPath const& cachePath) const
{
  
  size_t numPoints = 0;
  size_t numCounts = 0;
  size_t numIndices = 0;
  for(const auto& buffer: buffers){
    numPoints += buffer.points.size();
    numCounts += buffer.faceVertexCounts.size();
    numIndices += buffer.faceVertexIndices.size();
  }

  VtArray<int> faceVertexCounts(numCounts);
  VtArray<int> faceVertexIndices(numIndices);
  VtArray<GfVec3f> points(numPoints);

  size_t pointsIndex = 0;
  size_t countsIndex = 0;
  size_t indicesIndex = 0;

  for(const auto& buffer: buffers) {
    size_t numPoints =  buffer.points.size();
    if(numPoints) {
      memcpy(&points[pointsIndex], &buffer.points[0], 
        numPoints * sizeof(GfVec3f));
      pointsIndex += numPoints;
    }
  
    size_t numCounts = buffer.faceVertexCounts.size();
    if(numCounts) {
      memcpy(&faceVertexCounts[countsIndex], &buffer.faceVertexCounts[0], 
        numCounts * sizeof(int));
      countsIndex += numCounts;
    }
    
    size_t numIndices = buffer.faceVertexIndices.size();
    if(numIndices) {
      for(int i=0;i<numIndices;++i)
        faceVertexIndices[indicesIndex + i] = buffer.faceVertexIndices[i] + indicesIndex;
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

