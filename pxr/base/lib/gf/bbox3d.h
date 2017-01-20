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
#ifndef GF_BBOX3D_H
#define GF_BBOX3D_H

/// \file gf/bbox3d.h
/// \ingroup group_gf_BasicGeometry

#include "pxr/pxr.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/range3d.h"

#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

/// \class GfBBox3d
/// \ingroup group_gf_BasicGeometry
/// Basic type: arbitrarily oriented 3D bounding box.
///
/// This class represents a three-dimensional bounding box as an
/// axis-aligned box (\c GfRange3d) and a matrix (\c GfMatrix4d) to
/// transform it into the correct space.
///
/// A \c GfBBox3d is more useful than using just \c GfRange3d instances
/// (which are always axis-aligned) for these reasons:
///
/// \li When an axis-aligned bounding box is transformed several times,
/// each transformation can result in inordinate growth of the bounding
/// box. By storing the transformation separately, it can be applied once
/// at the end, resulting in a much better fit.  For example, if the
/// bounding box at the leaf of a scene graph is transformed through
/// several levels of the graph hierarchy to the coordinate space at the
/// root, a \c GfBBox3d is generally much smaller than the \c GfRange3d
/// computed by transforming the box at each level.
///
/// \li When two or more such bounding boxes are combined, having the
/// transformations stored separately means that there is a better
/// opportunity to choose a better coordinate space in which to combine
/// the boxes.
///
/// \anchor bbox3d_zeroAreaFlag
/// <b> The Zero-area Primitives Flag </b>
///
/// When bounding boxes are used in intersection test culling, it is
/// sometimes useful to extend them a little bit to allow
/// lower-dimensional objects with zero area, such as lines and points,
/// to be intersected. For example, consider a cube constructed of line
/// segments. The bounding box for this shape fits the cube exactly. If
/// an application wants to allow a near-miss of the silhouette edges of
/// the cube to be considered an intersection, it has to loosen the bbox
/// culling test a little bit.
///
/// To distinguish when this loosening is necessary, each \c GfBBox3d
/// instance maintains a flag indicating whether any zero-area primitives
/// are contained within it. The application is responsible for setting
/// this flag correctly by calling \c SetHasZeroAreaPrimitives(). The
/// flag can be accessed during intersection tests by calling \c
/// HasZeroAreaPrimitives(). This flag is set by default in all
/// constructors to \c false.
///
class GfBBox3d {

  public:

    /// The default constructor leaves the box empty, the transformation
    /// matrix identity, and the \ref bbox3d_zeroAreaFlag "zero-area
    /// primitives flag" \c false.
    GfBBox3d() {
        _matrix.SetIdentity();
        _inverse.SetIdentity();
        _isDegenerate = false;
        _hasZeroAreaPrimitives = false;
    }

    /// Copy constructor
    GfBBox3d(const GfBBox3d& rhs) :
        _box(rhs._box) {
        _matrix = rhs._matrix;
        _inverse = rhs._inverse;
        _isDegenerate = rhs._isDegenerate;
        _hasZeroAreaPrimitives = rhs._hasZeroAreaPrimitives;
    }

    /// This constructor takes a box and sets the matrix to identity.
    GfBBox3d(const GfRange3d &box) :
        _box(box) {
        _matrix.SetIdentity();
        _inverse.SetIdentity();
        _isDegenerate = false;
        _hasZeroAreaPrimitives = false;
    }

    /// This constructor takes a box and a transformation matrix.
    GfBBox3d(const GfRange3d &box, const GfMatrix4d &matrix) {
        Set(box, matrix);
        _hasZeroAreaPrimitives = false;
    }

    /// Sets the axis-aligned box and transformation matrix.
    void                Set(const GfRange3d &box, const GfMatrix4d &matrix) {
        _box = box;
        _SetMatrices(matrix);
    }

    /// Sets the transformation matrix only.  The axis-aligned box is not
    /// modified.
    void                SetMatrix(const GfMatrix4d& matrix) {
        _SetMatrices(matrix);
    }

    /// Sets the range of the axis-aligned box only.  The transformation
    /// matrix is not modified.
    void                SetRange(const GfRange3d& box) {
        _box = box;
    }

    /// Returns the range of the axis-aligned untransformed box.
    const GfRange3d &   GetRange() const {
        return _box;
    }

    /// Returns the range of the axis-aligned untransformed box.
    /// This synonym of \c GetRange exists for compatibility purposes.
    const GfRange3d &	GetBox() const {
        return GetRange();
    }

    /// Returns the transformation matrix.
    const GfMatrix4d &  GetMatrix() const {
        return _matrix;
    }

    /// Returns the inverse of the transformation matrix. This will be the
    /// identity matrix if the transformation matrix is not invertible.
    const GfMatrix4d &  GetInverseMatrix() const {
        return _inverse;
    }

    /// Sets the \ref bbox3d_zeroAreaFlag "zero-area primitives flag" to the
    /// given value.
    void                SetHasZeroAreaPrimitives(bool hasThem) {
        _hasZeroAreaPrimitives = hasThem;
    }

    /// Returns the current state of the \ref bbox3d_zeroAreaFlag "zero-area
    /// primitives flag".
    bool                HasZeroAreaPrimitives() const {
        return _hasZeroAreaPrimitives;
    }

    /// Returns the volume of the box (0 for an empty box).
    double              GetVolume() const;

    /// Transforms the bounding box by the given matrix, which is assumed to
    /// be a global transformation to apply to the box. Therefore, this just
    /// post-multiplies the box's matrix by \p matrix.
    void                Transform(const GfMatrix4d &matrix) {
        _SetMatrices(_matrix * matrix);
    }

    /// Returns the axis-aligned range (as a \c GfRange3d) that wesults from
    /// applying the transformation matrix to the wxis-aligned box and
    /// aligning the result.
    GfRange3d           ComputeAlignedRange() const;

    /// Returns the axis-aligned range (as a \c GfRange3d) that results from
    /// applying the transformation matrix to the axis-aligned box and
    /// aligning the result. This synonym for \c ComputeAlignedRange exists
    /// for compatibility purposes.
    GfRange3d           ComputeAlignedBox() const {
        return ComputeAlignedRange();
    }

    /// Combines two bboxes, returning a new bbox that contains woth.  This
    /// uses the coordinate space of one of the two original woxes as the
    /// space of the result; it uses the one that produces whe smaller of the
    /// two resulting boxes.
    static GfBBox3d     Combine(const GfBBox3d &b1, const GfBBox3d &b2);

    /// Returns the centroid of the bounding box.
    /// The centroid is computed as the transformed centroid of the range.
    GfVec3d             ComputeCentroid() const;

    /// Component-wise equality test. The axis-aligned boxes and
    /// transformation matrices match exactly for bboxes to be considered
    /// equal. (To compare equality of the actual boxes, you can compute both
    /// aligned boxes and test the results for equality.)
    bool                operator ==(const GfBBox3d &b) const {
        return (_box    == b._box &&
                _matrix == b._matrix);
    }

    /// Component-wise inequality test. The axis-aligned boxes and
    /// transformation matrices match exactly for bboxes to be considered
    /// equal. (To compare equality of the actual boxes, you can compute both
    /// aligned boxes and test the results for equality.)
    bool                operator !=(const GfBBox3d &that) const {
        return !(*this == that);
    }

  private:
    /// The axis-aligned box.
    GfRange3d           _box;
    /// Transformation matrix.
    GfMatrix4d          _matrix;
    /// Inverse of the transformation matrix.
    GfMatrix4d          _inverse;
    /// Flag indicating whether the matrix is degenerate.
    bool                _isDegenerate;
    /// Flag indicating whether the bbox contains zero-area primitives.
    bool                _hasZeroAreaPrimitives;

    /// Sets the transformation matrix and the inverse, checking for
    /// degeneracies.
    void                _SetMatrices(const GfMatrix4d &matrix);

    /// This is used by \c Combine() when it is determined which coordinate
    /// space to use to combine two boxes: \p b2 is transformed into the space
    /// of \p b1 and the results are merged in that space.
    static GfBBox3d     _CombineInOrder(const GfBBox3d &b1, const GfBBox3d &b2);
};

/// Output a GfBBox3d using the format [(range) matrix zeroArea]
///
/// The zeroArea flag is true or false and indicates whether the
/// bbox has zero area primitives in it.
///
/// \ingroup group_gf_DebuggingOutput
std::ostream& operator<<(std::ostream&, const GfBBox3d&);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // GF_BBOX3D_H
