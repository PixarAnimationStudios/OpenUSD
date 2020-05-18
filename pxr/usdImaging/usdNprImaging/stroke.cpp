//
// Copyright 2020 benmalartre
//
#include "pxr/usdImaging/usdNprImaging/stroke.h"
#include "pxr/usdImaging/usdNprImaging/mesh.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

void UsdNprStrokeChain::Init(UsdNprHalfEdge* edge, short type, float width)
{
  _start = edge;
  _type = type;
  _width = width;
  _nodes.push_back(UsdNprStrokeNode(_start, 1.0));
}

static UsdNprHalfEdge* 
_GetNextEdge(UsdNprHalfEdge* edge, const std::vector<short>& classifications, short type)
{
  UsdNprHalfEdge* next = edge->next;
  while(next && next != edge->twin)
  {
    if(!(classifications[next->index] & EDGE_CHAINED)) {
      short flags = classifications[next->index];
      if(flags & EDGE_TWIN)
        flags = classifications[next->twin->index];
      if(flags & type)return next;
    }
    if(next->twin) next = next->twin->next;
    else next = NULL;
  }

  next = edge->next->next;
  while(next && next != edge->twin)
  {
    if(!(classifications[next->index] & EDGE_CHAINED)) {
      short flags = classifications[next->index];
      if(flags & EDGE_TWIN)
        flags = classifications[next->twin->index];
      if(flags & type)return next;
    }
    if(next->twin) next = next->twin->next->next;
    else next = NULL;
  }

  return NULL;
}

void UsdNprStrokeChain::Build(std::vector<short>& classifications, short type)
{
  UsdNprStrokeNode* node = &_nodes.back();
  UsdNprHalfEdge* edge = node->edge;
  classifications[edge->index] |= EDGE_CHAINED;
  UsdNprHalfEdge* current = edge;
  //_nodes.push_back(UsdNprStrokeNode(edge, 1.0));
  while(true)
  {
    UsdNprHalfEdge* next = _GetNextEdge(current, classifications, type);
    
    if(next)
    {
      _nodes.push_back(UsdNprStrokeNode(next, 1.0));
      classifications[next->index] |= EDGE_CHAINED;
      if(next->twin)classifications[next->twin->index] |= EDGE_CHAINED;
      if(next == edge || next == edge->twin)return;
      else current = next;
    }
    else break;
  }
  if(edge->twin)classifications[edge->twin->index] |= EDGE_CHAINED;
};

void UsdNprStrokeChain::_ComputePoint(const GfVec3f* positions, const GfMatrix4f& xform,
  size_t index, const GfVec3f& V, float width, GfVec3f* p1, GfVec3f* p2) const
{
  size_t lastNode = _nodes.size() - 1;
  GfVec3f A, B, C, N;
  if(index == 0)
  {
    const UsdNprStrokeNode* node1 = &_nodes[0];
    const UsdNprStrokeNode* node2 = &_nodes[1];

    A = xform.Transform(
      positions[node1->edge->vertex] * node1->weight +
      positions[node1->edge->next->vertex] * (node1->weight));

    B = xform.Transform(
      positions[node2->edge->vertex] * node2->weight +
      positions[node2->edge->next->vertex] * (1.0-node2->weight));

    const GfVec3f T = (B-A);
    const GfVec3f D = ((A+B)*0.5 - V);
    N = (T ^ D).GetNormalized();

    *p1 = (A * 0.99 + V * 0.01) - N * width;
    *p2 = (A * 0.99 + V * 0.01) + N * width;
  }

  else if(index == lastNode)
  {
    const UsdNprStrokeNode* node1 = &_nodes[lastNode-1];
    const UsdNprStrokeNode* node2 = &_nodes[lastNode];

    A = xform.Transform(
      positions[node1->edge->vertex] * node1->weight +
      positions[node1->edge->next->vertex] * (node1->weight));

    B = xform.Transform(
      positions[node2->edge->vertex] * node2->weight +
      positions[node2->edge->next->vertex] * (1.0-node2->weight));

    const GfVec3f T = (B-A);
    const GfVec3f D = ((A+B)*0.5 - V);
    N = (T ^ D).GetNormalized();

    *p1 = (B * 0.99 + V * 0.01) - N * width;
    *p2 = (B * 0.99 + V * 0.01) + N * width;
  }

  else
  {
    const UsdNprStrokeNode* node1 = &_nodes[index-1];
    const UsdNprStrokeNode* node2 = &_nodes[index];
    const UsdNprStrokeNode* node3 = &_nodes[index+1];

    A = xform.Transform(
      positions[node1->edge->vertex] * node1->weight +
      positions[node1->edge->next->vertex] * (node1->weight));

    B = xform.Transform(
      positions[node2->edge->vertex] * node2->weight +
      positions[node2->edge->next->vertex] * (1.0-node2->weight));

    C = xform.Transform(
      positions[node3->edge->vertex] * node3->weight +
      positions[node3->edge->next->vertex] * (1.0-node3->weight));

    const GfVec3f T = ((B-A) + (C-B)) * 0.5f;
    const GfVec3f D = (B - V);
    N = (T ^ D).GetNormalized();

    *p1 = (B * 0.99 + V * 0.01) - N * width;
    *p2 = (B * 0.99 + V * 0.01) + N * width;
  }
}

void UsdNprStrokeChain::ComputeOutputPoints( const UsdNprHalfEdgeMesh* mesh, 
  const GfVec3f& viewPoint, GfVec3f* points) const
{
  size_t numPoints = _nodes.size()  * 2;

  // points
  size_t index = 0;
  float width = 0.04;

  const GfVec3f* positions = mesh->GetPositionsPtr();
  const GfMatrix4f& xform = mesh->GetMatrix();
  
  for(size_t i=0;i<_nodes.size();++i)
    _ComputePoint(positions, xform, i, viewPoint, _width, &points[i*2], &points[i*2+1]);
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
  const GfVec3f v = _mesh->GetMatrix().GetInverse().Transform(
    GfVec3f(_viewMatrix[3][0],_viewMatrix[3][1],_viewMatrix[3][2])
  );
  const GfVec3f* positions = _mesh->GetPositionsPtr();
  const GfVec3f* normals = _mesh->GetNormalsPtr();

  size_t numVertices = _mesh->GetNumPoints();
  std::vector<float> dots(numVertices);
  for(int i=0;i<numVertices;++i)
    dots[i] = GfDot((positions[i] - v).GetNormalized(), normals[i]);

  size_t edgeIndex = 0;
  const std::vector<UsdNprHalfEdge>& halfEdges = _mesh->GetHalfEdges();
  for(const auto& halfEdge: halfEdges)
  {
    short flags = halfEdge.GetFlags(positions, normals, v, 0.25);
    _allFlags[edgeIndex++] = flags;
    if(flags & EDGE_BOUNDARY)
      _boundaries.push_back(&halfEdge);
    else
    {
      if(flags & EDGE_TWIN) continue;
      if(flags & EDGE_CREASE) 
        _creases.push_back(&halfEdge);
      if(flags & EDGE_SILHOUETTE)
        _silhouettes.push_back(&halfEdge);
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
UsdNprStrokeGraph::BuildStrokeChains(short edgeType)
{
  std::vector<const UsdNprHalfEdge*>* edges;
  if(edgeType == EDGE_SILHOUETTE)edges = &_silhouettes;
  else if(edgeType == EDGE_BOUNDARY)edges = &_boundaries;
  else if(edgeType == EDGE_CREASE)edges = &_creases;

  ResetChainedFlag(*edges);
  size_t numEdges = (*edges).size();
  
  if(numEdges) {
    size_t startId = 0;
    while(startId < numEdges)
    {
      if(!(_allFlags[(*edges)[startId]->index] & EDGE_CHAINED)) {
        UsdNprStrokeChain stroke;
        stroke.Init((UsdNprHalfEdge*)(*edges)[startId], edgeType, 0.04);

        stroke.Build(_allFlags, edgeType);
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

PXR_NAMESPACE_CLOSE_SCOPE
