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

typedef struct LoFiHalfEdge_t
{
    uint32_t                vertex;      // vertex index
    uint32_t                sample;      // sample index
    struct LoFiHalfEdge_t*  twin;        // opposite half-edge
    struct LoFiHalfEdge_t*  next;        // next half-edge

    LoFiHalfEdge_t():vertex(0),twin(NULL),next(NULL){};
} LoFiHalfEdge;

/// \class LoFiAdjacency
///
class LoFiAdjacency
{
public:
    void Compute(const VtArray<GfVec3i>& samples);
    const VtArray<int>& Get(){return _adjacency;};

private:
    VtArray<int> _adjacency;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_HALFEDGE_H
