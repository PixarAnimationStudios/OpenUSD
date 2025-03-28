//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_PX_OSD_SUBDIV_TAGS_H
#define PXR_IMAGING_PX_OSD_SUBDIV_TAGS_H

/// \file pxOsd/subdivTags.h

#include "pxr/pxr.h"
#include "pxr/imaging/pxOsd/api.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/tf/token.h"

#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE


/// \class PxOsdSubdivTags
///
/// Tags for non-hierarchial subdiv surfaces.
///
class PxOsdSubdivTags {

public:

    PxOsdSubdivTags() = default;
    PxOsdSubdivTags(PxOsdSubdivTags const&) = default;
    PxOsdSubdivTags(PxOsdSubdivTags&&) = default;
    PxOsdSubdivTags& operator=(PxOsdSubdivTags const&) = default;
    PxOsdSubdivTags& operator=(PxOsdSubdivTags&&) = default;

    PxOsdSubdivTags(
        const TfToken& vertexInterpolationRule,
        const TfToken& faceVaryingInterpolationRule,
        const TfToken& creaseMethod,
        const TfToken& triangleSubdivision,
        const VtIntArray& creaseIndices,
        const VtIntArray& creaseLengths,
        const VtFloatArray& creaseWeights,
        const VtIntArray& cornerIndices,
        const VtFloatArray& cornerWeights)
        : _vtxInterpolationRule(vertexInterpolationRule)
        , _fvarInterpolationRule(faceVaryingInterpolationRule)
        , _creaseMethod(creaseMethod)
        , _trianglesSubdivision(triangleSubdivision)
        , _creaseIndices(creaseIndices)
        , _creaseLengths(creaseLengths)
        , _creaseWeights(creaseWeights)
        , _cornerIndices(cornerIndices)
        , _cornerWeights(cornerWeights) {}

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

    typedef size_t ID;

    /// Returns the hash value of this topology to be used for instancing.
    PXOSD_API
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
};

PXOSD_API
std::ostream& operator<<(std::ostream &out, PxOsdSubdivTags const &);
PXOSD_API
bool operator==(const PxOsdSubdivTags& lhs, const PxOsdSubdivTags& rhs);
PXOSD_API
bool operator!=(const PxOsdSubdivTags& lhs, const PxOsdSubdivTags& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PX_OSD_SUBDIV_TAGS_H
