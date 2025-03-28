//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_GF_QUATERNION_H
#define PXR_BASE_GF_QUATERNION_H

/// \file gf/quaternion.h
/// \ingroup group_gf_LinearAlgebra

#include "pxr/pxr.h"
#include "pxr/base/gf/api.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/hash.h"

#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

/// \class GfQuaternion
/// \ingroup group_gf_LinearAlgebra
///
/// Basic type: complex number with scalar real part and vector imaginary
/// part.
///
/// This class represents a generalized complex number that has a scalar real
/// part and a vector of three imaginary values. Quaternions are used by the
/// \c GfRotation class to represent arbitrary-axis rotations.
///
class GfQuaternion
{
  public:

    /// The default constructor leaves the quaternion undefined.
    GfQuaternion() {
    }

    /// This constructor initializes the real part to the argument and
    /// the imaginary parts to zero.
    ///
    /// Since quaternions typically need to be normalized, the only reasonable
    /// values for \p realVal are -1, 0, or 1.  Other values are legal but are
    /// likely to be meaningless.
    explicit GfQuaternion(int realVal)
        : _real(realVal), _imaginary(0)
    {
    }

    /// This constructor initializes the real and imaginary parts.
    GfQuaternion(double real, const GfVec3d &imaginary)
        : _real(real), _imaginary(imaginary) {
    }

    /// Sets the real part of the quaternion.
    void                SetReal(double real) {
        _real  = real;
    }

    /// Sets the imaginary part of the quaternion.
    void                SetImaginary(const GfVec3d &imaginary) {
        _imaginary  = imaginary;
    }

    /// Returns the real part of the quaternion.
    double              GetReal() const {
        return _real;
    }

    /// Returns the imaginary part of the quaternion.
    const GfVec3d &     GetImaginary() const {
        return _imaginary;
    }

    /// Returns the zero quaternion, which has a real part of 0 and
    /// an imaginary part of (0,0,0).
    static GfQuaternion GetZero() {
        return GfQuaternion(0.0, GfVec3d(0.0, 0.0, 0.0));
    }

    /// Returns the identity quaternion, which has a real part of 1 and 
    /// an imaginary part of (0,0,0).
    static GfQuaternion GetIdentity() {
        return GfQuaternion(1.0, GfVec3d(0.0, 0.0, 0.0));
    }

    /// Returns geometric length of this quaternion.
    GF_API
    double              GetLength() const;

    /// Returns a normalized (unit-length) version of this quaternion.
    /// direction as this. If the length of this quaternion is smaller than \p
    /// eps, this returns the identity quaternion.
    GF_API
    GfQuaternion        GetNormalized(double eps = GF_MIN_VECTOR_LENGTH) const;

    /// Normalizes this quaternion in place to unit length, returning the
    /// length before normalization. If the length of this quaternion is
    /// smaller than \p eps, this sets the quaternion to identity.
    GF_API
    double              Normalize(double eps = GF_MIN_VECTOR_LENGTH);

    /// Returns the inverse of this quaternion.
    GF_API
    GfQuaternion        GetInverse() const;

    /// Hash.
    friend inline size_t hash_value(const GfQuaternion &q) {
        return TfHash::Combine(q.GetReal(), q.GetImaginary());
    }

    /// Component-wise quaternion equality test. The real and imaginary parts
    /// must match exactly for quaternions to be considered equal.
    bool		operator ==(const GfQuaternion &q) const {
	return (GetReal()      == q.GetReal() &&
		GetImaginary() == q.GetImaginary());
    }

    /// Component-wise quaternion inequality test. The real and imaginary
    /// parts must match exactly for quaternions to be considered equal.
    bool		operator !=(const GfQuaternion &q) const {
        return ! (*this == q);
    }

    /// Post-multiplies quaternion \p q into this quaternion.
    GF_API
    GfQuaternion &      operator *=(const GfQuaternion &q);

    /// Scales this quaternion by \p s.
    GF_API
    GfQuaternion &      operator *=(double s);

    /// Scales this quaternion by 1 / \p s.
    GfQuaternion &      operator /=(double s) {
        return (*this) *= 1.0 / s;
    }

    /// Component-wise unary sum operator.
    GfQuaternion &      operator +=(const GfQuaternion &q)  {
        _real      += q._real;
        _imaginary += q._imaginary;
        return *this;
    }

    /// Component-wise unary difference operator.
    GfQuaternion &      operator -=(const GfQuaternion &q)  {
        _real      -= q._real;
        _imaginary -= q._imaginary;
        return *this;
    }

    /// Component-wise binary sum operator.
    friend GfQuaternion	operator +(const GfQuaternion &q1,
                    const GfQuaternion &q2) {
        GfQuaternion qt = q1;
        return qt += q2;
    }

    /// Component-wise binary difference operator.
    friend GfQuaternion	operator -(const GfQuaternion &q1,
                    const GfQuaternion &q2) {
        GfQuaternion qt = q1;
        return qt -= q2;
    }

    /// Returns the product of quaternions \p q1 and \p q2.
    friend GfQuaternion	operator *(const GfQuaternion &q1,
                    const GfQuaternion &q2) {
        GfQuaternion qt  = q1;
        return       qt *= q2;
    }

    /// Returns the product of quaternion \p q and scalar \p s.
    friend GfQuaternion	operator *(const GfQuaternion &q, double s) {
        GfQuaternion qt  = q;
        return       qt *= s;
    }

    /// Returns the product of quaternion \p q and scalar \p s.
    friend GfQuaternion	operator *(double s, const GfQuaternion &q) {
        GfQuaternion qt  = q;
        return       qt *= s;
    }

    /// Returns the product of quaternion \p q and scalar 1 / \p s.
    friend GfQuaternion	operator /(const GfQuaternion &q, double s) {
        GfQuaternion qt  = q;
        return       qt /= s;
    }

    /// Spherically interpolate between \p q0 and \p q1.
    ///
    /// If the interpolant \p alpha
    /// is zero, then the result is \p q0, while \p alpha of one yields
    /// \p q1.
    GF_API
    friend GfQuaternion GfSlerp(double alpha,
                                const GfQuaternion& q0,
                                const GfQuaternion& q1);

    // TODO Remove this legacy alias/overload.
    friend GF_API GfQuaternion GfSlerp(const GfQuaternion& q0,
                                       const GfQuaternion& q1,
                                       double alpha);

  private:
    /// Real part
    double              _real;
    /// Imaginary part
    GfVec3d             _imaginary;

    /// Returns the square of the length
    double              _GetLengthSquared() const {
        return (_real * _real + GfDot(_imaginary, _imaginary));
    }
};

// Friend functions must be declared.
GF_API GfQuaternion GfSlerp(double alpha, const GfQuaternion& q0, const GfQuaternion& q1);
GF_API GfQuaternion GfSlerp(const GfQuaternion& q0, const GfQuaternion& q1, double alpha);

/// Output a GfQuaternion using the format (r + (x, y, z)).
/// \ingroup group_gf_DebuggingOutput
GF_API std::ostream& operator<<(std::ostream& out, const GfQuaternion& q);


/// Returns the dot (inner) product of two quaternions.
inline double
GfDot(const GfQuaternion &q1, const GfQuaternion &q2) {
    return  (q1.GetReal() * q2.GetReal()) + GfDot(q1.GetImaginary(), q2.GetImaginary());
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_GF_QUATERNION_H
