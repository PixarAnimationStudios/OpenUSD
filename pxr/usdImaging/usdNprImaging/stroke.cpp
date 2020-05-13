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
  size_t index, const GfVec3f& V, float width, GfVec3f* p1, GfVec3f* p2)
{
  size_t lastNode = _nodes.size() - 1;
  GfVec3f A, B, C, N;
  if(index == 0)
  {
    UsdNprStrokeNode* node1 = &_nodes[0];
    UsdNprStrokeNode* node2 = &_nodes[1];

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
    UsdNprStrokeNode* node1 = &_nodes[lastNode-1];
    UsdNprStrokeNode* node2 = &_nodes[lastNode];

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
    UsdNprStrokeNode* node1 = &_nodes[index-1];
    UsdNprStrokeNode* node2 = &_nodes[index];
    UsdNprStrokeNode* node3 = &_nodes[index+1];

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
  const GfMatrix4f& xform, const GfVec3f& viewPoint, VtArray<GfVec3f>& points)
{
  size_t numSegments = _nodes.size() - 1;
  size_t numPoints = numSegments * 2 + 2;

  // points
  points.resize(numPoints);
  size_t index = 0;
  float width = 0.04;

  const GfVec3f* positions = mesh->GetPositionsPtr();
  
  for(size_t i=0;i<_nodes.size();++i)
  {
    _ComputePoint(positions, xform, i, viewPoint, _width, &points[i*2], &points[i*2+1]);
  }
}

static void
_ResetChainedFlag(const std::vector<UsdNprHalfEdge*>& edges, 
  std::vector<short>& flags)
{
  for(const auto& edge: edges)
    flags[edge->index] &= ~EDGE_CHAINED;
}

static void 
_BuildStrokeChains(const std::vector<UsdNprHalfEdge*>& edges, 
  std::vector<short>& flags, float width, short type,
  std::vector<UsdNprStrokeChain>* strokes)
{
  _ResetChainedFlag(edges, flags);
  size_t numEdges = edges.size();
  
  if(numEdges) {
    size_t startId = 0;
    while(startId < numEdges)
    {
      if(!(flags[edges[startId]->index] & EDGE_CHAINED)) {
        UsdNprStrokeChain stroke;
        stroke.Init((UsdNprHalfEdge*)edges[startId], type, width);

        stroke.Build(flags, type);
        strokes->push_back(stroke);
      }
      else startId++;
    }
  }
}

PXR_NAMESPACE_CLOSE_SCOPE
