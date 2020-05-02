//
// Copyright 2020 benmalartre
//
// unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_HALFEDGE_H
#define PXR_IMAGING_PLUGIN_LOFI_HALFEDGE_H

#include "pxr/pxr.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/gf/vec3i.h"
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE

struct LoFiHalfEdge
{
    uint32_t              vertex;    // vertex index
    uint32_t              sample;    // sample index
    uint32_t              triangle;  // triangle index
    struct LoFiHalfEdge*  twin;      // opposite half-edge
    struct LoFiHalfEdge*  next;      // next half-edge

    LoFiHalfEdge():vertex(0),twin(NULL),next(NULL){};
};

/// \class LoFiAdjacency
///
class LoFiAdjacency
{
public:
    void Compute(const VtArray<GfVec3i>& samples);
    const VtArray<int>& Get() const {return _adjacency;};
    const std::vector<LoFiHalfEdge>& GetHalfEdges() const {return _halfEdges;};

private:
    VtArray<int> _adjacency;
    std::vector<LoFiHalfEdge> _halfEdges;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_HALFEDGE_H
