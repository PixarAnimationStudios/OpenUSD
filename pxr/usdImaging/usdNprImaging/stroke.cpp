//
// Copyright 2020 benmalartre
//
#include "pxr/usdImaging/usdNprImaging/stroke.h"
#include "pxr/usdImaging/usdNprImaging/mesh.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

void UsdNprStrokeChain::FromEdge(UsdNprHalfEdge* edge, float width, 
  const GfVec3f* positions, const GfVec3f* normals, const GfVec3f& color)
{
  _start = edge;
  _type = EDGE_SILHOUETTE;
  _width = width;
  _nodes.push_back(UsdNprStrokeNode(edge, width, 
    positions[edge->vertex], normals[edge->vertex], color));
  _nodes.push_back(UsdNprStrokeNode(edge->next, width,
    positions[edge->next->vertex], normals[edge->next->vertex], color));
}

void UsdNprStrokeChain::Init(UsdNprHalfEdge* edge, short type, 
  float width, const GfVec3f& position, const GfVec3f& normal, 
  const GfVec3f& color)
{
  _start = edge;
  _type = type;
  _width = width;
  _nodes.push_back(UsdNprStrokeNode(_start, 1.0, position, normal, color));
}

static UsdNprHalfEdge* 
_GetNextEdge(UsdNprHalfEdge* edge, const std::vector<short>& classifications, 
  short type)
{
  UsdNprHalfEdge* next = edge->next;
  while(next != edge)
  {
    if(!(classifications[next->index] & EDGE_CHAINED)) {
      short flags = classifications[next->index];
      if(flags & EDGE_TWIN) {
        flags = classifications[next->twin->index];
      } 
      if(flags & type){
        return next;
      }
    }
    next = next->next;
  }
  if(edge->twin) {
    next = edge->twin->next;
    while(next != edge->twin)
    {
      if(!(classifications[next->index] & EDGE_CHAINED)) {
        short flags = classifications[next->index];
        if(flags & EDGE_TWIN) {
          flags = classifications[next->twin->index];
        }
        if(flags & type) {
          return next;
        }
      }
      next = next->next;
    }
  }
  return NULL;
}

void UsdNprStrokeChain::Build(const UsdNprStrokeGraph* graph,
  std::vector<short>& classifications, short type)
{
  UsdNprStrokeNode* node = &_nodes.back();
  UsdNprHalfEdge* edge = node->edge;
  classifications[edge->index] |= EDGE_CHAINED;
  if(edge->twin)classifications[edge->twin->index] |= EDGE_CHAINED;
  UsdNprHalfEdge* current = edge;
  const UsdNprHalfEdgeMesh* mesh = graph->GetMesh();
  const GfVec3f* positions = mesh->GetPositionsPtr();
  const GfVec3f* normals = mesh->GetVertexNormalsPtr();
  bool edgesWeighted = (type == EDGE_SILHOUETTE);
  while(true)
  {
    UsdNprHalfEdge* last = _nodes.back().edge;
    UsdNprHalfEdge* next = _GetNextEdge(current, classifications, type);
    GfVec3f position, normal, color;

    if(next)
    {
      if(edgesWeighted) {
        if(classifications[next->index] & EDGE_TWIN){
          float weight = graph->GetSilhouetteWeight(next->twin->index);
          next->twin->GetWeightedPositionAndNormal(positions, normals, 
            weight, position, normal);
          _nodes.push_back(UsdNprStrokeNode(next->twin, 5.0, 
            position, normal, color));
        } else {
          float weight = graph->GetSilhouetteWeight(next->index);
          next->GetWeightedPositionAndNormal(positions, normals, 
            weight, position, normal);
          _nodes.push_back(UsdNprStrokeNode(next, 5.0, 
            position, normal, color));
        }
      } else {
        //GfVec3f n1, n2;
        //next->GetVertexNormal(normals, n1);
        //next->GetVertexNormal(normals, n2);
        _nodes.push_back(UsdNprStrokeNode(next, 5.0, 
          positions[next->vertex], normals[next->vertex], color));
        _nodes.push_back(UsdNprStrokeNode(next, 5.0,
          positions[next->next->vertex], normals[next->next->vertex], color));
      }
      classifications[next->index] |= EDGE_CHAINED;
      if(next->twin)classifications[next->twin->index] |= EDGE_CHAINED;
      if(next == edge || next == edge->twin)return;
      else current = next;
    }
    else break;
  }
  
};

void UsdNprStrokeChain::_ComputePoint(const GfVec3f* positions, const GfMatrix4f& xform,
  size_t index, const GfVec3f& V, float width, GfVec3f* p1, GfVec3f* p2) const
{
  const UsdNprStrokeNode* node = &_nodes[index];
  const GfVec3f worldPosition = xform.Transform(node->position);
  const GfVec3f worldNormal = xform.TransformDir(node->normal);
  *p1 = worldPosition - worldNormal * width;
  *p2 = worldPosition + worldNormal * width;
}

void UsdNprStrokeChain::ComputeOutputPoints( const UsdNprHalfEdgeMesh* mesh, 
  const GfVec3f& viewPoint, GfVec3f* points) const
{
  const GfVec3f* positions = mesh->GetPositionsPtr();
  const GfMatrix4f& xform = mesh->GetMatrix();
  
  for(size_t i=0;i<_nodes.size();++i)
    _ComputePoint(positions, xform, i, viewPoint, _width, 
      &points[i*2], &points[i*2+1]);
}

void
UsdNprStrokeGraph::Init(UsdNprHalfEdgeMesh* mesh, const GfMatrix4f& view, const GfMatrix4f& proj)
{
  _mesh = mesh;
  _viewMatrix = view;
  _projectionMatrix = proj;

  size_t numHalfEdges = _mesh->GetNumHalfEdges();
  _allFlags.resize(numHalfEdges);
  memset(&_allFlags[0], 0, numHalfEdges * sizeof(short));

}

void
UsdNprStrokeGraph::Prepare(const UsdNprStrokeParams& params)
{
  // classify edges
  const GfVec3f viewPoint = _mesh->GetMatrix().GetInverse().Transform(
    GfVec3f(_viewMatrix[3][0],_viewMatrix[3][1],_viewMatrix[3][2])
  );

  const GfVec3f* positions = _mesh->GetPositionsPtr();
  const GfVec3f* vertexNormals = _mesh->GetVertexNormalsPtr();
  const GfVec3f* polygonNormals = _mesh->GetPolygonNormalsPtr();

  size_t edgeIndex = 0;
  const std::vector<UsdNprHalfEdge>& halfEdges = _mesh->GetHalfEdges();
  size_t numHalfEdges = halfEdges.size();
  _silhouetteWeights.resize(numHalfEdges);
  memset(&_silhouetteWeights[0], 0.f, numHalfEdges * sizeof(float));
  float weight;
  for(auto& halfEdge: halfEdges)
  {
    short flags = halfEdge.GetFlags(positions, vertexNormals, 
      polygonNormals, viewPoint, 0.25, &weight);
    
    _allFlags[edgeIndex++] = flags;
    if(flags & EDGE_BOUNDARY)
      _boundaries.push_back(&halfEdge);
    else
    {
      if(flags & EDGE_TWIN) continue;
      if(flags & EDGE_CREASE) 
        _creases.push_back(&halfEdge);
      if(flags & EDGE_SILHOUETTE) {
        _silhouettes.push_back(&halfEdge);
        _silhouetteWeights[halfEdge.index] = weight;
      }
    }
  }
}

void
UsdNprStrokeGraph::ResetChainedFlag(const std::vector<const UsdNprHalfEdge*>& edges)
{
  for(const auto& edge: edges)
    _allFlags[edge->index] &= ~EDGE_CHAINED;
}

void 
UsdNprStrokeGraph::ClearStrokeChains()
{
  _strokes.clear();
}

void 
UsdNprStrokeGraph::BuildRawStrokes(short edgeType, const GfVec3f& color)
{
  const GfVec3f* positions = _mesh->GetPositionsPtr();
  const GfVec3f* normals = _mesh->GetVertexNormalsPtr();
  std::vector<const UsdNprHalfEdge*>* edges;
  if(edgeType == EDGE_SILHOUETTE)edges = &_silhouettes;
  else if(edgeType == EDGE_BOUNDARY)edges = &_boundaries;
  else if(edgeType == EDGE_CREASE)edges = &_creases;
  else return;

  size_t numEdges = (*edges).size();
  
  if(numEdges) {
    size_t startId = 0;
    while(startId < numEdges)
    {
      UsdNprHalfEdge* currentEdge = (UsdNprHalfEdge*)(*edges)[startId];
      UsdNprStrokeChain stroke;
      stroke.FromEdge(currentEdge, 0.1f, positions, normals, color);
      _strokes.push_back(stroke);
      startId++;
    }
  }
}

void 
UsdNprStrokeGraph::BuildStrokeChains(short edgeType, const GfVec3f& color)
{
  const GfVec3f* positions = _mesh->GetPositionsPtr();
  const GfVec3f* normals = _mesh->GetVertexNormalsPtr();
  std::vector<const UsdNprHalfEdge*>* edges;
  bool edgesWeighted = false;
  if(edgeType == EDGE_SILHOUETTE) {
    edges = &_silhouettes;
    edgesWeighted = true;
  }
  else if(edgeType == EDGE_BOUNDARY)edges = &_boundaries;
  else if(edgeType == EDGE_CREASE)edges = &_creases;
  else return;

  ResetChainedFlag(*edges);
  size_t numEdges = (*edges).size();
  
  if(numEdges) {
    size_t startId = 0;
    while(startId < numEdges)
    {
      UsdNprHalfEdge* currentEdge = (UsdNprHalfEdge*)(*edges)[startId];
      if(!(_allFlags[currentEdge->index] & EDGE_CHAINED)) {
        UsdNprStrokeChain stroke;
        if(edgesWeighted) {
          if(_allFlags[currentEdge->index] & EDGE_TWIN) {
            float weight = _silhouetteWeights[currentEdge->twin->index];
            GfVec3f position, normal;
            currentEdge->twin->GetWeightedPositionAndNormal(positions, normals,
              weight, position, normal);
            stroke.Init(currentEdge->twin, edgeType, 0.1f, position, normal, color);
          } else {
            float weight = _silhouetteWeights[currentEdge->index];
            GfVec3f position, normal;
            currentEdge->GetWeightedPositionAndNormal(positions, normals,
              weight, position, normal);
            stroke.Init(currentEdge, edgeType, 0.1f, position, normal, color);
          }
          
        } else {
          //currentEdge->GetVertexNormal(normals, n1);
          //currentEdge->next->GetVertexNormal(normals, n2);
          stroke.Init(currentEdge, edgeType, 0.1f, 
            positions[currentEdge->vertex], normals[currentEdge->vertex], color);
        }
          

        stroke.Build(this, _allFlags, edgeType);
        if(stroke.GetNumNodes()>1)
          _strokes.push_back(stroke);
      }
      else startId++;
    }
  }
}

void
UsdNprStrokeGraph::ConnectChains(short edgeType)
{

}

size_t 
UsdNprStrokeGraph::GetNumNodes() const
{
  size_t numNodes = 0;
  for(const auto& stroke: _strokes)
      numNodes += stroke.GetNumNodes();
  return numNodes;
}

GfVec3f
UsdNprStrokeGraph::GetViewPoint() const
{
  return GfVec3f(
    _viewMatrix[3][0],
    _viewMatrix[3][1],
    _viewMatrix[3][2]);
}

float
UsdNprStrokeGraph::GetSilhouetteWeight(int index) const
{
  return _silhouetteWeights[index];
};

PXR_NAMESPACE_CLOSE_SCOPE
