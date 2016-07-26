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
///
/// \file pxOsd/subdivTags.h
///

#ifndef PXOSD_SUBDIV_TAGS_H
#define PXOSD_SUBDIV_TAGS_H

#include "pxr/base/vt/array.h"
#include "pxr/base/tf/token.h"

///
/// Tags for non-hierarchial subdiv surfaces.
///
class PxOsdSubdivTags {

public:

    /// Returns the vertex boundary interpolation rule
    TfToken GetVertexInterpolationRule() const {
        return _vtxInterpolationRule;
    }

    /// Set the vertex boundary interpolation rule
    void SetVertexInterpolationRule(TfToken vtxInterp) {
        _vtxInterpolationRule = vtxInterp;
    }

    /// Returns the face-varying boundary interpolation rule
    TfToken GetFaceVaryingInterpolationRule() const {
        return _fvarInterpolationRule;
    }

    /// Set the face-varying boundary interpolation rule
    void SetFaceVaryingInterpolationRule(TfToken fvarInterp) {
       _fvarInterpolationRule = fvarInterp;
    }

    /// Returns the creasing method
    TfToken GetCreaseMethod() const {
        return _creaseMethod;
    }

    /// Set the creasing method
    void SetCreaseMethod(TfToken creaseMethod) {
        _creaseMethod = creaseMethod;
    }

    /// Returns the triangle subdivision method
    TfToken GetTriangleSubdivision() const {
        return _trianglesSubdivision;
    }

    /// Set the triangle subdivision method
    void SetTriangleSubdivision(TfToken triangleSubdivision) {
        _trianglesSubdivision = triangleSubdivision;
    }


    ///
    /// \name Crease
    /// @{

    /// Returns the edge crease indices
    VtIntArray const &GetCreaseIndices() const {
        return _creaseIndices;
    }

    /// Set the edge crease indices
    void SetCreaseIndices(VtIntArray const &creaseIndices) {
        _creaseIndices = creaseIndices;
    }

    /// Returns the edge crease loop lengths
    VtIntArray const &GetCreaseLengths() const {
        return _creaseLengths;
    }

    /// Set the edge crease loop lengths
    void SetCreaseLengths(VtIntArray const &creaseLengths) {
        _creaseLengths = creaseLengths;
    }

    /// Returns the edge crease weights
    VtFloatArray const &GetCreaseWeights() const {
        return _creaseWeights;
    }

    /// Set the edge crease weights
    void SetCreaseWeights(VtFloatArray const &creaseWeights) {
        _creaseWeights = creaseWeights;
    }
    /// @}


    ///
    /// \name Corner
    /// @{

    /// Returns the edge corner indices
    VtIntArray const &GetCornerIndices() const {
        return _cornerIndices;
    }

    /// Set the edge corner indices
    void SetCornerIndices(VtIntArray const &cornerIndices) {
        _cornerIndices = cornerIndices;
    }

    /// Returns the edge corner weights
    VtFloatArray const &GetCornerWeights() const {
        return _cornerWeights;
    }

    /// Set the edge corner weights
    void SetCornerWeights(VtFloatArray const &cornerWeights) {
        _cornerWeights = cornerWeights;
    }
    /// @}


    /// @}

    ///
    /// \name Holes
    /// @{

    /// Returns the edge corner indices
    VtIntArray const &GetHoleIndices() const {
        return _holeIndices;
    }

    /// Returns faces indices for holes
    void SetHoleIndices(VtIntArray const &holeIndices) {
        _holeIndices = holeIndices;
    }
    /// @}

    typedef size_t ID;

    /// Returns the hash value of this topology to be used for instancing.
    ID ComputeHash() const;

private:

    // note: if you're going to add more members, make sure
    // ComputeHash will be updated too.

    TfToken _vtxInterpolationRule,
            _fvarInterpolationRule,
            _creaseMethod,
            _trianglesSubdivision;

    VtIntArray   _creaseIndices,
                 _creaseLengths;
    VtFloatArray _creaseWeights;

    VtIntArray   _cornerIndices;
    VtFloatArray _cornerWeights;

    VtIntArray   _holeIndices;
};

std::ostream& operator << (std::ostream &out, PxOsdSubdivTags const &);
bool operator==(const PxOsdSubdivTags& lhs, const PxOsdSubdivTags& rhs);
bool operator!=(const PxOsdSubdivTags& lhs, const PxOsdSubdivTags& rhs);

#endif // PXOSD_SUBDIV_TAGS_H


