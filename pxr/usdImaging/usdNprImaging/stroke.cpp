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
_GetNextEdge(UsdNprHalfEdge* edge, const std::vector<short>& classifications,
  short type, std::vector<bool>& visited)
{
  UsdNprHalfEdge* next = edge->next;

  while(next && next != edge->twin)
  {
    if(!visited[next->index]) {
      short flags = classifications[next->index];
      if(flags & STROKE_TWIN)
        flags = classifications[next->twin->index];
      if(flags & type)return next;
    }
    if(next->twin) next = next->twin->next;
    else next = NULL;
  }

  next = edge->next->next;
  while(next && next != edge->twin)
  {
    if(!visited[next->index]) {
      short flags = classifications[next->index];
      if(flags & STROKE_TWIN)
        flags = classifications[next->twin->index];
      if(flags & type)return next;
    }
    if(next->twin) next = next->twin->next->next;
    else next = NULL;
  }
  return NULL;
}

void UsdNprStrokeChain::Build(const std::vector<short>& classifications, short type, std::vector<bool>& visited)
{
  UsdNprStrokeNode* node = &_nodes.back();
  UsdNprHalfEdge* edge = node->edge;
  visited[edge->index] = true;
  UsdNprHalfEdge* current = edge;
  while(true)
  {
    UsdNprHalfEdge* next = _GetNextEdge(current, classifications, type, visited);
    if(next)
    {
      _nodes.push_back(UsdNprStrokeNode(next, 1.0));
      visited[next->index] = true;
      if(next->twin)visited[next->index] = true;
      if(next == edge || next ==edge->twin)return;
      else current = next;
    }
    else break;
  }
};

PXR_NAMESPACE_CLOSE_SCOPE
