//
// Copyright 2020 benmalartre
//
// unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_TOPOLOGY_H
#define PXR_IMAGING_PLUGIN_LOFI_TOPOLOGY_H

#include "pxr/pxr.h"
#include "pxr/base/vt/array.h"
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


/// \struct LoFiTopology
///
struct LoFiTopology
{
    enum Type {
        POINTS,
        LINES,
        TRIANGLES,
        TRIANGLES_ADJACENCY
    };
    Type                type;
    const int*          samples;
    const int*          bases;
    size_t              numElements;
    size_t              numBases;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_TOPOLOGY_H
