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
#include "pxr/imaging/hd/bufferArrayRange.h"

PXR_NAMESPACE_OPEN_SCOPE


// DrawingCoord is a bundle of buffer array ranges that are used for drawing.
//
// Hd_DrawBatch requires drawitem to have 7 different types of buffer
// resources (some of them are optional) as a drawing coordinate.
//
// a. Constant
// b. Vertex Primvars
// c. Topology
// d. Element Primvars  (optional)
// e. Instance PrimVars (optional)
// f. Instance Index    (optional)
// g. FaceVarying Primvars (optional)
//

/// \class HdDrawingCoord
///
/// A tiny 6-integers entity which provides a mapping between the client
/// facing interface to the actual internal location stored in the
/// HdBufferArrayRangeContainer, which is owned by rprim.
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
    static const int CustomSlotsBegin = 6;
    static const int DefaultNumSlots = 3; /* Constant, Vertex, Topology */
    static const int Unassigned = -1;

    HdDrawingCoord() :
        // default slots:
        _constantPrimVar(0),
        _vertexPrimVar(1),
        _topology(2),
        _elementPrimVar(3),
        _instanceIndex(4),
        _faceVaryingPrimVar(5),
        _instancePrimVarNumLevels(0),
        _instancePrimVar(Unassigned) {
    }

    int GetConstantPrimVarIndex() const    { return _constantPrimVar; }
    void SetConstantPrimVarIndex(int slot) { _constantPrimVar = slot; }
    int GetVertexPrimVarIndex() const      { return _vertexPrimVar; }
    void SetVertexPrimVarIndex(int slot)   { _vertexPrimVar = slot; }
    int GetTopologyIndex() const           { return _topology; }
    void SetTopologyIndex(int slot)        { _topology = slot; }
    int GetElementPrimVarIndex() const     { return _elementPrimVar; }
    void SetElementPrimVarIndex(int slot)  { _elementPrimVar = slot; }
    int GetInstanceIndexIndex() const      { return _instanceIndex; }
    void SetInstanceIndexIndex(int slot)   { _instanceIndex = slot; }
    int GetFaceVaryingPrimVarIndex() const    { return _faceVaryingPrimVar; }
    void SetFaceVaryingPrimVarIndex(int slot) { _faceVaryingPrimVar = slot; }

    // instance primvars
    int GetInstancePrimVarIndex(int level) const {
        return _instancePrimVar + level;
    }
    void SetInstancePrimVarIndex(int level, int slot) {
        if (_instancePrimVar == Unassigned) {
            _instancePrimVar = slot - level;
        } else {
            TF_VERIFY(_instancePrimVar == (slot - level));
        }
        // update num levels
        _instancePrimVarNumLevels
            = std::max(int8_t(level+1), _instancePrimVarNumLevels);
    }
    int GetInstancePrimVarNumLevels() const { return _instancePrimVarNumLevels; }

private:
    int8_t _constantPrimVar;
    int8_t _vertexPrimVar;
    int8_t _topology;
    int8_t _elementPrimVar;
    int8_t _instanceIndex;
    int8_t _faceVaryingPrimVar;
    int8_t _instancePrimVarNumLevels;
    int8_t _instancePrimVar;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_DRAWING_COORD_H
