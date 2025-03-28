//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_DRAWING_COORD_H
#define PXR_IMAGING_HD_DRAWING_COORD_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/base/tf/diagnostic.h"
#include <stdint.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdDrawingCoord
///
/// A tiny set of integers, which provides an indirection mapping from the 
/// conceptual space of an HdRprim's resources (topological, primvar & 
/// instancing) to the index within HdBufferArrayRangeContainer, where the
/// resource is stored.
/// 
/// Each HdDrawItem contains a HdDrawingCoord, with the relevant compositional
/// hierarchy being:
/// 
///  HdRprim
///  |
///  +--HdRepr(s)
///  |    |
///  |    +--HdDrawItem(s)----------.
///  |         |                    |
///  |         +--HdDrawingCoord    |
///  |                              | (mapping provided by HdDrawingCoord)
///  +--HdRprimSharedData           |
///     |                           |
///     +--HdBARContainer  <--------+
///  
///
/// Having this indirection provides a recipe for how to configure
/// a drawing coordinate, which is a bundle of HdBufferArrayRanges, while
/// they are shared or not shared across different representations
/// constructed on the same prim.
///
///    HullRepr --------- Rprim --------- RefinedRepr
///       |                 |                  |
///    DrawItem             |              DrawItem
///       |                 |                  |
///  DrawingCoord       Container        DrawingCoord
///     constant -------> [ 0 ] <------    constant
///     vertex   -------> [ 1 ]
///     topology -------> [ 2 ]
///                       [ 3 ]
///                       [ 4 ]
///                       [ 5 ]
///                       [ 6 ]
///                       [ 7 ]
///                       [ 8 ] <------    vertex   (refined)
///                       [ 9 ] <------    topology (refined)
///                        ...
/// instance level=0 ---> [ k ]
/// instance level=1 ---> [k+1]
/// instance level=2 ---> [k+2]
///
class HdDrawingCoord {
public:
    static const int CustomSlotsBegin = 8;
    static const int DefaultNumSlots = 3; /* Constant, Vertex, Topology */
    static const int Unassigned = -1;

    HdDrawingCoord() :
        // default slots:
        _topology(2),
        _instancePrimvar(Unassigned),
        _constantPrimvar(0),
        _vertexPrimvar(1),
        _elementPrimvar(3),
        _instanceIndex(4),
        _faceVaryingPrimvar(5),
        _topologyVisibility(6),
        _varyingPrimvar(7) {
    }

    int GetConstantPrimvarIndex() const    { return _constantPrimvar; }
    void SetConstantPrimvarIndex(int slot) { _constantPrimvar = slot; }
    int GetVertexPrimvarIndex() const      { return _vertexPrimvar; }
    void SetVertexPrimvarIndex(int slot)   { _vertexPrimvar = slot; }
    int GetTopologyIndex() const           { return _topology; }
    void SetTopologyIndex(int slot)        { _topology = slot; }
    int GetElementPrimvarIndex() const     { return _elementPrimvar; }
    void SetElementPrimvarIndex(int slot)  { _elementPrimvar = slot; }
    int GetInstanceIndexIndex() const      { return _instanceIndex; }
    void SetInstanceIndexIndex(int slot)   { _instanceIndex = slot; }
    int GetFaceVaryingPrimvarIndex() const    { return _faceVaryingPrimvar; }
    void SetFaceVaryingPrimvarIndex(int slot) { _faceVaryingPrimvar = slot; }
    int GetTopologyVisibilityIndex() const    { return _topologyVisibility; }
    void SetTopologyVisibilityIndex(int slot) { _topologyVisibility = slot; }
    int GetVaryingPrimvarIndex() const      { return _varyingPrimvar; }
    void SetVaryingPrimvarIndex(int slot)   { _varyingPrimvar = slot; }

    // instance primvars take up a range of slots.
    void SetInstancePrimvarBaseIndex(int slot) { _instancePrimvar = slot; }
    int GetInstancePrimvarIndex(int level) const {
        TF_VERIFY(_instancePrimvar != Unassigned);
        return _instancePrimvar + level;
    }

private:
    int16_t _topology;
    int16_t _instancePrimvar;
    int8_t _constantPrimvar;
    int8_t _vertexPrimvar;
    int8_t _elementPrimvar;
    int8_t _instanceIndex;
    int8_t _faceVaryingPrimvar;
    int8_t _topologyVisibility;
    int8_t _varyingPrimvar;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_DRAWING_COORD_H
