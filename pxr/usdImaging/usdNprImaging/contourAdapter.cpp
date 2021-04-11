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
#include "pxr/base/tf/stopwatch.h"


#include "pxr/base/tf/type.h"
#include "pxr/base/work/loops.h"
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
  SdfPath const& contourPath = prim.GetPath();
  UsdGeomGprim gprim(prim);
  gprim.CreateDisplayColorPrimvar(UsdGeomTokens->faceVarying);

  if (_contourDataCache.find(contourPath) == _contourDataCache.end()) {
      auto contourData = std::make_shared<_ContourData>();
      _contourDataCache[contourPath] = contourData;
  }
  _ContourData* contourData = _GetContourData(contourPath);
  UsdNprContour contour(prim); 
  UsdGeomXformCache xformCache(UsdTimeCode::Default());
  std::vector<UsdPrim> contourSurfaces = contour.GetContourSurfaces();
  for(int i=0;i<contourSurfaces.size();++i)
  {
    SdfPath contourSurfacePath = contourSurfaces[i].GetPath();
    if(_halfEdgeMeshes.find(contourSurfacePath) == 
      _halfEdgeMeshes.end())
    {
      const UsdImagingPrimAdapterSharedPtr& adapter = 
        _GetPrimAdapter(contourSurfaces[i], false);

      SdfPath cachePath = 
        _ResolveCachePath(contourSurfaces[i].GetPath(), nullptr);
      HdDirtyBits varyingBits = HdChangeTracker::Clean;
      adapter->TrackVariability(
        contourSurfaces[i], cachePath, &varyingBits, nullptr );

      UsdNprHalfEdgeMeshSharedPtr halfEdgeMesh(
        new UsdNprHalfEdgeMesh(contourSurfacePath, varyingBits));

      halfEdgeMesh->Init(
        UsdGeomMesh(contourSurfaces[i]), UsdTimeCode::EarliestTime());
      halfEdgeMesh->SetMatrix(
        xformCache.GetLocalToWorldTransform(contourSurfaces[i]));
      _halfEdgeMeshes[contourSurfacePath] = halfEdgeMesh;
    }
    contourData->halfEdgeMeshes[contourSurfacePath] =
      _halfEdgeMeshes[contourSurfacePath];
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
  *timeVaryingBits |= HdChangeTracker::DirtyTopology;
}

static void 
_BuildStrokes(_ContourAdapterComputeDatas& datas)
{
  UsdNprStrokeGraph* graph = datas.graph;
  UsdNprHalfEdgeMesh* halfEdgeMesh = graph->GetMesh();
  if(!halfEdgeMesh)return;
  
  std::unique_lock<std::mutex> lock(halfEdgeMesh->GetMutex());
  if(halfEdgeMesh->IsVarying()) {
    
    if(halfEdgeMesh->GetLastTime() != datas.time)
    {
      char varyingBits = halfEdgeMesh->GetVaryingBits();

      const UsdPrim& contourSurface = *(datas.prim);
      if(varyingBits & UsdHalfEdgeMeshVaryingBits::VARYING_TOPOLOGY)
      {
        halfEdgeMesh->Init(UsdGeomMesh(contourSurface), UsdTimeCode(datas.time));
      }
        
      else if(varyingBits & UsdHalfEdgeMeshVaryingBits::VARYING_DEFORM)
      {
        halfEdgeMesh->Update(UsdGeomMesh(contourSurface), UsdTimeCode(datas.time));
      }

      halfEdgeMesh->SetLastTime(datas.time);
    }
  }
  lock.unlock();

  graph->Prepare(*datas.strokeParams);
  graph->ClearStrokeChains();
  graph->BuildRawStrokes(EDGE_SILHOUETTE, GfVec3f(1.f, 0.f, 0.f));
  graph->BuildRawStrokes(EDGE_BOUNDARY, GfVec3f(0.f, 1.f, 0.f));
  graph->BuildRawStrokes(EDGE_CREASE, GfVec3f(0.f, 0.f, 1.f));
  //graph->BuildStrokeChains(EDGE_SILHOUETTE, GfVec3f(0.f, 1.f, 0.f));
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
    UsdNprContour contour(prim);
    std::vector<UsdPrim> contourSurfaces = contour.GetContourSurfaces();
    if(!contourSurfaces.size()) return;
    
    BaseAdapter::UpdateForTime(
        prim, cachePath, time, requestedBits, instancerContext);
    
    UsdGeomXformCache xformCache(time);

    UsdRelationship viewPointRel = contour.GetContourViewPointRel();
    GfMatrix4d viewMatrix;
    GfMatrix4d projMatrix(1);
    if(viewPointRel.HasAuthoredTargets())
    {
      SdfPathVector viewPointTargets;
      if(viewPointRel.GetTargets(&viewPointTargets))
      {
        SdfPath viewPointPath = viewPointTargets[0];

        viewMatrix = 
          xformCache.GetLocalToWorldTransform(
            prim.GetStage()->GetPrimAtPath(viewPointPath));
      }
    }

    _ContourData* contourData = _GetContourData(prim.GetPath());
    std::vector<_ContourAdapterComputeDatas> datas(contourSurfaces.size());
    UsdNprStrokeGraphList strokeGraphs(contourSurfaces.size());
    UsdNprStrokeParams strokeParams;

    size_t index = 0;
    for(const UsdPrim& contourSurface: contourSurfaces)
    {
      SdfPath contourSurfacePath = contourSurface.GetPath();
      UsdNprHalfEdgeMeshMap::const_iterator it = 
        contourData->halfEdgeMeshes.find(contourSurfacePath);

      if(it != contourData->halfEdgeMeshes.end())
      {
        const UsdNprHalfEdgeMeshSharedPtr& halfEdgeMesh = it->second;
        char varyingBits = halfEdgeMesh->GetVaryingBits();

        if(varyingBits & UsdHalfEdgeMeshVaryingBits::VARYING_TRANSFORM)
          halfEdgeMesh->SetMatrix(
            xformCache.GetLocalToWorldTransform(contourSurface));

        _ContourAdapterComputeDatas* threadData = &datas[index];
        threadData->prim = &contourSurface;
        threadData->time = time.GetValue();
        threadData->strokeParams = &strokeParams;

        strokeGraphs[index].Init(
          halfEdgeMesh.get(), 
          GfMatrix4f(viewMatrix), 
          GfMatrix4f(projMatrix));
        threadData->graph = &strokeGraphs[index];   

        threadData->halfEdgeMesh = halfEdgeMesh.get();
        threadData->viewPointMatrix = viewMatrix;
      }
      
      index++;
    }

    WorkParallelForEach(datas.begin(), datas.end(), _BuildStrokes);

    UsdImagingPrimvarDescCache* valueCache = _GetPrimvarDescCache();
    _ComputeOutputGeometry(contourData, strokeGraphs, valueCache, cachePath);
    //_ComputeNormalsGeometry(contourData, strokeGraphs);
    //_ComputeHalfEdgesGeometry(contourData,strokeGraphs);
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
  // Allow base class to handle change processing.
  return BaseAdapter::ProcessPropertyChange(prim, cachePath, propertyName);
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

// parameters
void
UsdImagingContourAdapter::_PopulateStrokeParams(UsdPrim const& prim,
  UsdNprStrokeParams* params)
{

}

void
UsdImagingContourAdapter::_ComputeOutputGeometry(
  _ContourData* contourData, 
  const UsdNprStrokeGraphList& strokeGraphs,
  UsdImagingPrimvarDescCache* primvarDescCache, 
  SdfPath const& cachePath) const
{
  size_t numPoints = 0;
  size_t numCounts = 0;
  size_t numIndices = 0;
  for(const auto& strokeGraph: strokeGraphs) {
    size_t s = strokeGraph.GetNumStrokes();
    size_t n = strokeGraph.GetNumNodes();
    if(n>1) {
      numPoints += n * 2;
      numCounts += n - s;
      numIndices += (n-s) * 4;
    }
  }

  VtArray<int> faceVertexCounts(numCounts);
  for(int i=0;i<numCounts;++i)faceVertexCounts[i] = 4;

  VtArray<int> faceVertexIndices(numIndices);
  contourData->points.resize(numPoints);

  VtArray<GfVec3f> colors(numIndices);

  size_t pointsIndex = 0;
  size_t indicesIndex = 0;
  size_t colorIndex = 0;
  size_t offsetIndex = 0;

  /*
  GfVec3f color(
        (float)rand() / float(RAND_MAX),
        (float)rand() / float(RAND_MAX),
        (float)rand() / float(RAND_MAX)
      );
  */ 
 //const GfVec3f color(0.1f, 0.1f, .1f);
  for(const auto& strokeGraph: strokeGraphs) {
    const UsdNprHalfEdgeMesh* mesh = strokeGraph.GetMesh();
    GfVec3f viewPoint = strokeGraph.GetViewPoint();
    for(const auto& stroke: strokeGraph.GetStrokes()) {
      
      size_t numNodes = stroke.GetNumNodes();
      if(numNodes >1) {
        size_t numPoints =  numNodes * 2;
        if(numPoints) {
          stroke.ComputeOutputPoints( mesh, viewPoint, 
          &contourData->points[pointsIndex]);
        }
        pointsIndex += numPoints;
       
        size_t numIndices = (numNodes - 1) * 4;
        if(numIndices) {
          for(int i=0; i < numNodes - 1; ++i) {
            faceVertexIndices[indicesIndex++] = offsetIndex + i * 2;
            faceVertexIndices[indicesIndex++] = offsetIndex + i * 2 + 1;
            faceVertexIndices[indicesIndex++] = offsetIndex + i * 2 + 3;
            faceVertexIndices[indicesIndex++] = offsetIndex + i * 2 + 2;
            const GfVec3f& color = stroke.GetNode(i)->color;
            colors[colorIndex++] = color;
            colors[colorIndex++] = color;
            colors[colorIndex++] = color;
            colors[colorIndex++] = color;
          }
        }
        offsetIndex += numNodes * 2;
      }
    }
  }

  contourData->topology = HdMeshTopology(PxOsdOpenSubdivTokens->none,
                                         UsdGeomTokens->rightHanded,
                                         faceVertexCounts,
                                         faceVertexIndices);

  contourData->colors = colors;
}

struct _DebugMesh{
  pxr::VtArray<int> faceCounts;
  pxr::VtArray<int> faceIndices;
  pxr::VtArray<GfVec3f> positions;
  pxr::VtArray<GfVec3f> colors;
};

static void _AddNormal(
  _DebugMesh* mesh, const GfVec3f& position, const GfVec3f& normal, 
  const GfVec3f& color, const GfVec3f& view)
{
  size_t baseIndex = mesh->faceIndices.size();
  mesh->faceCounts.push_back(4);
  for(size_t i = 0; i < 4; ++i) {
    mesh->faceIndices.push_back(baseIndex + i);
    mesh->colors.push_back(color);
  }

  GfVec3f side = (position - view).GetNormalized() ^ normal;
  mesh->positions.push_back(position - side * 0.1f);
  mesh->positions.push_back(position + side * 0.1f);
  mesh->positions.push_back(position + side * 0.1f + normal);
  mesh->positions.push_back(position - side * 0.1f + normal);
}

static void _AddHalfEdge(
  const UsdNprHalfEdgeMesh* mesh,
  _DebugMesh* output, const UsdNprHalfEdge* edge, 
  const GfVec3f& color, const GfVec3f& view)
{
  const GfVec3f* positions = mesh->GetPositionsPtr();
  const GfVec3f* normals = mesh->GetVertexNormalsPtr();
  {
    size_t baseIndex = output->faceIndices.size();
    output->faceCounts.push_back(4);
    const GfVec3f p0 = positions[edge->vertex];
    const GfVec3f p1 = positions[edge->next->vertex];
    const GfVec3f n0 = normals[edge->vertex];
    const GfVec3f n1 = normals[edge->next->vertex];
    
    for(size_t i = 0; i < 4; ++i) {
      output->faceIndices.push_back(baseIndex + i);
      output->colors.push_back(color);
    }

    const GfVec3f side = (p1 - p0).GetNormalized() ^ (n0 + n1) * 0.5f;
    output->positions.push_back(p0);
    output->positions.push_back(p1);
    output->positions.push_back(p1 + side * 0.1f);
    output->positions.push_back(p0 + side * 0.1f);
  }

  if(edge->twin) {
    size_t baseIndex = output->faceIndices.size();
    output->faceCounts.push_back(4);
    const GfVec3f p0 = positions[edge->vertex];
    const GfVec3f p1 = positions[edge->next->vertex];
    const GfVec3f n0 = normals[edge->vertex];
    const GfVec3f n1 = normals[edge->next->vertex];
    
    for(size_t i = 0; i < 4; ++i) {
      output->faceIndices.push_back(baseIndex + i);
      output->colors.push_back(color);
    }

    const GfVec3f side = (p0 - p1).GetNormalized() ^ (n0 + n1) * 0.5f;
    output->positions.push_back(p0);
    output->positions.push_back(p1);
    output->positions.push_back(p1 + side * 0.1f);
    output->positions.push_back(p0 + side * 0.1f);
  }
}



void
UsdImagingContourAdapter::_ComputeNormalsGeometry(
  _ContourData* contourData, 
  const UsdNprStrokeGraphList& strokeGraphs) const
{
  _DebugMesh debugMesh;
  GfVec3f color(1.f, 0.f, 0.f);
  
  for(const auto& strokeGraph: strokeGraphs) {
    const UsdNprHalfEdgeMesh* mesh = strokeGraph.GetMesh();
    GfVec3f viewPoint = strokeGraph.GetViewPoint();
    const GfVec3f* positions = mesh->GetPositionsPtr();
    const GfVec3f* normals = mesh->GetVertexNormalsPtr();
    size_t numPoints = mesh->GetNumPoints();
    for(size_t p = 0; p < numPoints; ++p) {
      _AddNormal(&debugMesh, positions[p], normals[p], color, viewPoint);
    }
  }

  contourData->topology = HdMeshTopology(PxOsdOpenSubdivTokens->none,
                                         UsdGeomTokens->rightHanded,
                                         debugMesh.faceCounts,
                                         debugMesh.faceIndices);

  contourData->points = debugMesh.positions;
  contourData->colors = debugMesh.colors;
}

void
UsdImagingContourAdapter::_ComputeHalfEdgesGeometry(
  _ContourData* contourData, 
  const UsdNprStrokeGraphList& strokeGraphs) const
{
  _DebugMesh debugMesh;
  
  for(const auto& strokeGraph: strokeGraphs) {
    const UsdNprHalfEdgeMesh* mesh = strokeGraph.GetMesh();
    size_t numHalfEdges = mesh->GetNumHalfEdges();
    std::vector<int> flags(numHalfEdges);
    memset(&flags[0], 0, sizeof(int) * numHalfEdges);

    GfVec3f viewPoint = strokeGraph.GetViewPoint();
    const GfVec3f* positions = mesh->GetPositionsPtr();
    const GfVec3f* normals = mesh->GetVertexNormalsPtr();
    
    const UsdNprHalfEdge* halfEdges = mesh->GetHalfEdgesPtr();
    for(size_t e = 0; e < numHalfEdges; ++e) {
      const UsdNprHalfEdge* halfEdge = &halfEdges[e];
      if(!flags[e]) {
        GfVec3f color(
          (float)rand()/(float)RAND_MAX,
          (float)rand()/(float)RAND_MAX,
          (float)rand()/(float)RAND_MAX
        );
        _AddHalfEdge(mesh, &debugMesh, halfEdge, color, viewPoint);
        flags[e] = true;
        if(halfEdge->twin) {
          flags[halfEdge->twin->index] = true;
        }
      }
    }
  }

  contourData->topology = HdMeshTopology(PxOsdOpenSubdivTokens->none,
                                         UsdGeomTokens->rightHanded,
                                         debugMesh.faceCounts,
                                         debugMesh.faceIndices);

  contourData->points = debugMesh.positions;
  contourData->colors = debugMesh.colors;
}

/*virtual*/ 
VtValue
UsdImagingContourAdapter::GetTopology(UsdPrim const& prim,
                                   SdfPath const& cachePath,
                                   UsdTimeCode time) const
{
  _ContourData* contourData = _GetContourData(prim.GetPath());
  if(contourData) {
    return VtValue(contourData->topology);
  } else {
    return VtValue();
  }
}

/*
bool
UsdImagingContourAdapter::GetColor(UsdPrim const& prim,
                                 UsdTimeCode time,
                                 TfToken* interpolation,
                                 VtValue* color,
                                 VtIntArray *indices)
{
  _ContourData* contourData = 
    UsdImagingContourAdapter::_GetContourData(prim.GetPath());
  if(contourData) {
    if (interpolation) {
        *interpolation = UsdGeomTokens->faceVarying;
    }
    if (color) {
        *color = VtValue(contourData->colors);
    }
    return true;
  } else {
    return false;
  }
}
*/

/*virtual*/
VtValue
UsdImagingContourAdapter::Get(UsdPrim const& prim,
                           SdfPath const& cachePath,
                           TfToken const &key,
                           UsdTimeCode time,
                           VtIntArray *outIndices) const
{
  TRACE_FUNCTION();
  HF_MALLOC_TAG_FUNCTION();

  _ContourData* contourData = _GetContourData(prim.GetPath());
  if(contourData) {
    if (key == HdTokens->points) {
      return VtValue(contourData->points);
    } else if(key == HdTokens->displayColor) {
      return VtValue(contourData->colors);
    }
  }

  return BaseAdapter::Get(prim, cachePath, key, time, outIndices);
}

UsdImagingContourAdapter::_ContourData*
UsdImagingContourAdapter::_GetContourData(const SdfPath& cachePath) const
{
    auto it = _contourDataCache.find(cachePath);
    return it != _contourDataCache.end() ? it->second.get() : nullptr;
}



PXR_NAMESPACE_CLOSE_SCOPE

