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
#ifndef PXR_BASE_GF_RECT2I_H
#define PXR_BASE_GF_RECT2I_H

/// \file gf/rect2i.h
/// \ingroup group_gf_LinearAlgebra

#include "pxr/pxr.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/api.h"

#include <boost/functional/hash.hpp>

#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

/// \class GfRect2i
/// \ingroup group_gf_LinearAlgebra
///
/// A 2D rectangle with integer coordinates.
///
/// A rectangle is internally represented as two corners. We refer to these
/// as the min and max corner where the min's x-coordinate and y-coordinate
/// are assumed to be less than or equal to the max's corresponding coordinates.
/// Normally, it is expressed as a min corner and a size.
///
/// Note that the max corner is included when computing the size (width and
/// height) of a rectangle as the number of integral points in the x- and
/// y-direction. In particular, if the min corner and max corner are the same,
/// then the width and the height of the rectangle will both be one since we
/// have exactly one integral point with coordinates greater or equal to the
/// min corner and less or equal to the max corner.
///
/// Specifically, <em> width = maxX - minX + 1</em> and
/// <em>height = maxY - minY + 1.</em> 
///
class GfRect2i {
public:
    /// Constructs an empty rectangle.
    GfRect2i(): _min(0,0), _max(-1,-1)
    {
    }

    /// Constructs a rectangle with \p min and \p max corners.
    GfRect2i(const GfVec2i& min, const GfVec2i& max)
        : _min(min), _max(max)
    {
    }

    /// Constructs a rectangle with \p min corner and the indicated \p width
    /// and \p height.
    GfRect2i(const GfVec2i& min, int width, int height)
        : _min(min), _max(min + GfVec2i(width-1, height-1))
    {
    }

    /// Returns true if the rectangle is a null rectangle.
    ///
    /// A null rectangle has both the width and the height set to 0, that is
    /// \code
    ///     GetMaxX() == GetMinX() - 1
    /// \endcode
    /// and
    /// \code
    ///     GetMaxY() == GetMinY() - 1
    /// \endcode
    /// Remember that if \c GetMinX() and \c GetMaxX() return the same value
    /// then the rectangle has width 1, and similarly for the height.
    ///
    /// A null rectangle is both empty, and not valid.
    bool IsNull() const {
        return GetWidth() == 0 && GetHeight() == 0;
    }

    /// Returns true if the rectangle is empty.
    ///
    /// An empty rectangle has one or both of its min coordinates strictly
    /// greater than the corresponding max coordinate.
    ///
    /// An empty rectangle is not valid.
    bool IsEmpty() const {
        return GetWidth() <= 0 || GetHeight() <= 0;
    }

    /// Return true if the rectangle is valid (equivalently, not empty).
    bool IsValid() const {
        return !IsEmpty();
    }

    /// Returns a normalized rectangle, i.e. one that has a non-negative width
    /// and height.
    ///
    /// \c GetNormalized() swaps the min and max x-coordinates to
    /// ensure a non-negative width, and similarly for the
    /// y-coordinates.
    GF_API
    GfRect2i GetNormalized() const;

    /// Returns the min corner of the rectangle.
    const GfVec2i& GetMin() const {
        return _min;
    }

    /// Returns the max corner of the rectangle.
    const GfVec2i& GetMax() const {
        return _max;
    }

    /// Return the X value of min corner.
    ///
    int GetMinX() const {
        return _min[0];
    }

    /// Set the X value of the min corner.
    ///
    void SetMinX(int x) {
        _min[0] = x;
    }

    /// Return the X value of the max corner.
    ///
    int GetMaxX() const {
        return _max[0];
    }

    /// Set the X value of the max corner
    void SetMaxX(int x) {
        _max[0] = x;
    }

    /// Return the Y value of the min corner
    ///
    int GetMinY() const {
        return _min[1];
    }

    /// Set the Y value of the min corner.
    ///
    void SetMinY(int y) {
        _min[1] = y;
    }

    /// Return the Y value of the max corner
    int GetMaxY() const {
        return _max[1];
    }

    /// Set the Y value of the max corner
    void SetMaxY(int y) {
        _max[1] = y;
    }

    /// Sets the min corner of the rectangle.
    void SetMin(const GfVec2i& min) {
        _min = min;
    }

    /// Sets the max corner of the rectangle.
    void SetMax(const GfVec2i& max) {
        _max = max;
    }

    /// Returns the center point of the rectangle.
    GfVec2i GetCenter() const {
        return (_min + _max) / 2;
    }

    /// Move the rectangle by \p displ.
    void Translate(const GfVec2i& displacement) {
        _min += displacement;
        _max += displacement;
    }

    /// Return the area of the rectangle.
    unsigned long GetArea() const {
        return (unsigned long)GetWidth() * (unsigned long)GetHeight();
    }

    /// Returns the size of the rectangle as a vector (width,height).
    GfVec2i GetSize() const {
        return GfVec2i(GetWidth(), GetHeight());
    }

    /// Returns the width of the rectangle.
    ///
    /// \note If the min and max x-coordinates are coincident, the width is
    /// one.
    int GetWidth() const {
        return (_max[0] - _min[0]) + 1;
    }

    /// Returns the height of the rectangle.
    ///
    /// \note If the min and max y-coordinates are coincident, the height is
    /// one.
    int GetHeight() const {
        return (_max[1] - _min[1]) + 1;
    }

    /// Computes the intersection of two rectangles.
    GfRect2i GetIntersection(const GfRect2i& that) const {
        if(IsEmpty())
            return *this;
        else if(that.IsEmpty())
            return that;
        else
            return GfRect2i(GfVec2i(GfMax(_min[0], that._min[0]),
                                    GfMax(_min[1], that._min[1])),
                            GfVec2i(GfMin(_max[0], that._max[0]),
                                    GfMin(_max[1], that._max[1])));
    }

    /// Computes the intersection of two rectangles.
    /// \deprecated Use GetIntersection() instead
    GfRect2i Intersect(const GfRect2i& that) const {
        return GetIntersection(that);
    }

    /// Computes the union of two rectangles.
    GfRect2i GetUnion(const GfRect2i& that) const {
        if(IsEmpty())
            return that;
        else if(that.IsEmpty())
            return *this;
        else
            return GfRect2i(GfVec2i(GfMin(_min[0], that._min[0]),
                                    GfMin(_min[1], that._min[1])),
                            GfVec2i(GfMax(_max[0], that._max[0]),
                                    GfMax(_max[1], that._max[1])));
    }

    /// Computes the union of two rectangles
    /// \deprecated Use GetUnion() instead.
    GfRect2i Union(const GfRect2i& that) const {
        return GetUnion(that);
    }

    /// Returns true if the specified point in the rectangle.
    bool Contains(const GfVec2i& p) const {
        return ((p[0] >= _min[0]) && (p[0] <= _max[0]) &&
                (p[1] >= _min[1]) && (p[1] <= _max[1]));
    }

    friend inline size_t hash_value(const GfRect2i &r) {
        size_t h = 0;
        boost::hash_combine(h, r._min);
        boost::hash_combine(h, r._max);
        return h;
    }        

    /// Returns true if \p r1 and \p r2 are equal.
    friend bool operator==(const GfRect2i& r1, const GfRect2i& r2) {
	return r1._min == r2._min && r1._max == r2._max;
    }

    /// Returns true if \p r1 and \p r2 are different.
    friend bool operator!=(const GfRect2i& r1, const GfRect2i& r2) {
	return !(r1 == r2);
    }

    /// Computes the union of two rectangles.
    /// \see GetUnion()
    GfRect2i operator += (const GfRect2i& that) {
        *this = GetUnion(that);
        return *this;
    }

    friend GfRect2i operator + (const GfRect2i r1, const GfRect2i& r2) {
        GfRect2i tmp(r1);
        tmp += r2;
        return tmp;
    }

private:
    GfVec2i _min, _max;
};

/// Output a GfRect2i using the format [(x y):(x y)].
/// \ingroup group_gf_DebuggingOutput
GF_API std::ostream& operator<<(std::ostream&, const GfRect2i&);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
