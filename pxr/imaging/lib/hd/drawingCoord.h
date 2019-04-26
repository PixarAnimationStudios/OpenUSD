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
#ifndef HD_DRAWING_COORD_H
#define HD_DRAWING_COORD_H

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
///                       [ 6 ] <------    vertex   (refined)
///                       [ 7 ] <------    topology (refined)
///                        ...
/// instance level=0 ---> [ k ]
/// instance level=1 ---> [k+1]
/// instance level=2 ---> [k+2]
///
class HdDrawingCoord {
public:
    static const int CustomSlotsBegin = 7;
    static const int DefaultNumSlots = 3; /* Constant, Vertex, Topology */
    static const int Unassigned = -1;

    HdDrawingCoord() :
        // default slots:
        _constantPrimvar(0),
        _vertexPrimvar(1),
        _topology(2),
        _elementPrimvar(3),
        _instanceIndex(4),
        _faceVaryingPrimvar(5),
        _topologyVisibility(6),
        _instancePrimvar(Unassigned) {
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

    // instance primvars take up a range of slots.
    void SetInstancePrimvarBaseIndex(int slot) { _instancePrimvar = slot; }
    int GetInstancePrimvarIndex(int level) const {
        TF_VERIFY(_instancePrimvar != Unassigned);
        return _instancePrimvar + level;
    }

private:
    int8_t _constantPrimvar;
    int8_t _vertexPrimvar;
    int8_t _topology;
    int8_t _elementPrimvar;
    int8_t _instanceIndex;
    int8_t _faceVaryingPrimvar;
    int8_t _topologyVisibility;
    int8_t _instancePrimvar;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_DRAWING_COORD_H
