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
#ifndef GF_TRANSFORM_H
#define GF_TRANSFORM_H

#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"

#include <iosfwd>

class GfMatrix4d;

/*!
 * \file transform.h
 * \ingroup group_gf_LinearAlgebra
 */

//!
// \class GfTransform transform.h "pxr/base/gf/transform.h"
// \ingroup group_gf_LinearAlgebra
// \brief Basic type: Compound linear transformation.
//
// This class represents a linear transformation specified as a series
// of individual components: a \em translation, a \em rotation, a \em
// scale, a \em pivotPosition, and a \em pivotOrientation.  When
// applied to a point, the point will be transformed as follows (in
// order):
//
//      - Scaled by the \em scale with respect to \em pivotPosition and 
//          the orientation specified by the \em pivotOrientation.
//      - Rotated by the \em rotation about \em pivotPosition.
//      - Translated by \em Translation
//
// That is, the cumulative matrix that this represents looks like this.
//
// \code
// M = -P * -O * S * O * R * P * T
// \endcode
//
// where
//      - \em T is the \em translation matrix
//      - \em P is the matrix that translates by \em pivotPosition
//      - \em R is the \em rotation matrix
//      - \em O is the matrix that rotates to \em pivotOrientation
//      - \em S is the \em scale matrix
//

class GfTransform {

  public:

    //!
    // The default constructor sets the component values to the
    // identity transformation.
    GfTransform() {
	SetIdentity();
    }

    //!
    // This constructor initializes the transformation from all
    // component values.  This is the constructor used by 2x code.
    GfTransform(const GfVec3d &scale,
		const GfRotation &pivotOrientation,
		const GfRotation &rotation,
		const GfVec3d &pivotPosition,
		const GfVec3d &translation) {
	Set(scale, pivotOrientation, rotation, pivotPosition, translation);
    }

    //!
    // This constructor initializes the transformation from all
    // component values.  This is the constructor used by 3x code.
    GfTransform(const GfVec3d &translation,
                const GfRotation &rotation,
                const GfVec3d &scale,
                const GfVec3d &pivotPosition,
                const GfRotation &pivotOrientation) {
        Set(translation, rotation, scale, pivotPosition, pivotOrientation);
    }

    //!
    // This constructor initializes the transformation with a matrix.  See
    // SetMatrix() for more information.
    GfTransform(const GfMatrix4d &m) {
        SetIdentity();
        SetMatrix(m);
    }

    //!
    // Sets the transformation from all component values.
    // This constructor orders its arguments the way that 2x expects.
    GfTransform &	Set(const GfVec3d &scale,
			    const GfRotation &pivotOrientation,
			    const GfRotation &rotation,
			    const GfVec3d &pivotPosition,
			    const GfVec3d &translation);

    //!
    // Sets the transformation from all component values.
    // This constructor orders its arguments the way that 3x expects.
    GfTransform &       Set(const GfVec3d &translation,
                            const GfRotation &rotation,
                            const GfVec3d &scale,
                            const GfVec3d &pivotPosition,
                            const GfRotation &pivotOrientation) {
        return Set(scale, pivotOrientation, rotation, 
                   pivotPosition, translation);
    }

    //!
    // Sets the transform components to implement the transformation
    // represented by matrix \p m , ignoring any projection. This
    // tries to leave the current center unchanged.
    GfTransform &	SetMatrix(const GfMatrix4d &m);

    //!
    // Sets the transformation to the identity transformation.
    GfTransform &	SetIdentity();

    //!
    // Sets the scale component, leaving all others untouched.
    void		SetScale(const GfVec3d &scale) {
	_scale = scale;
    }

    //!
    // Sets the pivot orientation component, leaving all others untouched.
    void		SetPivotOrientation(const GfRotation &pivotOrient) {
	_pivotOrientation = pivotOrient;
    }

    //!
    // Sets the pivot orientation component, leaving all others untouched.
    void		SetScaleOrientation(const GfRotation &pivotOrient) {
        SetPivotOrientation(pivotOrient);
    }

    //!
    // Sets the rotation component, leaving all others untouched.
    void		SetRotation(const GfRotation &rotation) {
	_rotation = rotation;
    }

    //!
    // Sets the pivot position component, leaving all others untouched.
    void                SetPivotPosition(const GfVec3d &pivPos) {
        _pivotPosition = pivPos;
    }

    //!
    // Sets the pivot position component, leaving all others untouched.
    void                SetCenter(const GfVec3d &pivPos) {
        SetPivotPosition(pivPos);
    }

    //!
    // Sets the translation component, leaving all others untouched.
    void		SetTranslation(const GfVec3d &translation) {
	_translation = translation;
    }

    //!
    // Returns the scale component.
    const GfVec3d &	GetScale() const {
	return _scale;
    }

    //!
    // Returns the pivot orientation component.
    const GfRotation &	GetPivotOrientation() const {
	return _pivotOrientation;
    }

    //!
    // Returns the scale orientation component.
    const GfRotation &	GetScaleOrientation() const {
	return GetPivotOrientation();
    }

    //!
    // Returns the rotation component.
    const GfRotation &	GetRotation() const {
	return _rotation;
    }

    //!
    // Returns the pivot position component.
    const GfVec3d &	GetPivotPosition() const {
	return _pivotPosition;
    }

    //!
    // Returns the pivot position component.
    const GfVec3d &	GetCenter() const {
	return GetPivotPosition();
    }

    //!
    // Returns the translation component.
    const GfVec3d &	GetTranslation() const {
	return _translation;
    }

    //!
    // Returns a \c GfMatrix4d that implements the cumulative
    // transformation.
    GfMatrix4d		GetMatrix() const;

    //!
    // Component-wise transform equality test. All components must
    // match exactly for transforms to be considered equal.
    bool		operator ==(const GfTransform &xf) const;

    //!
    // Component-wise transform inequality test. All components must
    // match exactly for transforms to be considered equal.
    bool		operator !=(const GfTransform &xf) const {
	return ! (*this == xf);
    }

    //!
    // Post-multiplies transform \p xf into this transform.
    GfTransform &	operator *=(const GfTransform &xf);

    //!
    // Returns the product of transforms \p xf1 and \p xf2 .
    friend GfTransform	operator *(const GfTransform &xf1,
				   const GfTransform &xf2) {
	GfTransform xf = xf1;
	return xf *= xf2;
    }

  private:
    //! translation
    GfVec3d             _translation;
    //! rotation
    GfRotation          _rotation;
    //! scale factors
    GfVec3d             _scale;
    //! orientation used for scaling and rotation
    GfRotation          _pivotOrientation;
    //! center of rotation and scaling
    GfVec3d             _pivotPosition;
};

/// Output a GfTransform using the format 
/// [scale, scaleorientation, rotation, center, translation].
/// \ingroup group_gf_DebuggingOutput
std::ostream& operator<<(std::ostream&, const GfTransform&);

#endif // GF_TRANSFORM_H
