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
#ifndef GF_RECT2I_H
#define GF_RECT2I_H

/// \file gf/rect2i.h
/// \ingroup group_gf_LinearAlgebra

#include "pxr/pxr.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/gf/vec2i.h"

#include <boost/functional/hash.hpp>

#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

/// \class GfRect2i
/// \ingroup group_gf_LinearAlgebra
///
/// A 2D rectangle with integer coordinates for windowing operations.
///
/// A rectangle is internally represented as an upper left corner and a
/// bottom right corner, but it is normally expressed as an upper left
/// corner and a size.
/// 
/// Note that the size (width and height) of a rectangle might be
/// different from what you are used to. If the top left corner and the
/// bottom right corner are the same, then the height and the width of
/// the rectangle will both be one.
///
/// Specifically, <em> width = right - left + 1</em> and
/// <em>height = bottom - top + 1.</em> The design corresponds to
/// rectangular spaces used by drawing functions, where
/// the width and height denote a number of pixels. For example,
/// drawing a rectangle with width and height one draws a single pixel.
///
/// The default coordinate system has origin (0,0) in the top left
/// corner, the positive direction of the y axis is downward and the
/// positive x axis is to the right.
///
class GfRect2i {
public:
    /// Constructs an empty rectangle.
    GfRect2i(): _lower(0,0), _higher(-1,-1)
    {
    }

    /// Constructs a rectangle with \p topLeft as the top left corner and \p
    /// bottomRight as the bottom right corner.
    GfRect2i(const GfVec2i& topLeft, const GfVec2i& bottomRight)
        : _lower(topLeft), _higher(bottomRight)
    {
    }

    /// Constructs a rectangle with \p topLeft as the top left corner and with
    /// the indicated width and height.
    GfRect2i(const GfVec2i& topLeft, int width, int height)
        : _lower(topLeft), _higher(topLeft + GfVec2i(width-1, height-1))
    {
    }

    /// Returns true if the rectangle is a null rectangle.
    ///
    /// A null rectangle has both the width and the height set to 0, that is
    /// \code
    ///     GetRight() == GetLeft() - 1
    /// \endcode
    /// and
    /// \code
    ///     GetBottom() == GetTop() - 1
    /// \endcode
    /// Remember that if \c GetRight() and \c GetLeft() return the same value
    /// then the rectangle has width 1, and similarly for the height.
    ///
    /// A null rectangle is both empty, and not valid.
    bool IsNull() const {
        return GetWidth() == 0 && GetHeight() == 0;
    }

    /// Returns true if the rectangle is empty.
    ///
    /// An empty rectangle has its left side strictly greater than its right
    /// side or its top strictly greater than its bottom.
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
    /// \c GetNormalized() swaps left and right to ensure a non-negative
    /// width, and similarly for top and bottom.
    GfRect2i GetNormalized() const;

    /// Returns the lower corner of the rectangle.
    const GfVec2i& GetLower() const {
        return _lower;
    }

    /// Returns the upper corner of the rectangle.
    const GfVec2i& GetHigher() const {
        return _higher;
    }

    /// Return the X value of the left edge.
    int GetLeft() const {
        return _lower[0];
    }

    /// Set the X value of the left edge.
    void SetLeft(int x) {
        _lower[0] = x;
    }

    /// Return the X value of the right edge.
    int GetRight() const {
        return _higher[0];
    }

    /// Set the X value of the right edge.
    void SetRight(int x) {
        _higher[0] = x;
    }

    /// Return the Y value of the top edge.
    int GetTop() const {
        return _lower[1];
    }

    /// Set the Y value of the top edge.
    void SetTop(int y) {
        _lower[1] = y;
    }

    /// Return the Y value of the bottom edge.
    int GetBottom() const {
        return _higher[1];
    }

    /// Set the Y value of the bottom edge.
    void SetBottom(int y) {
        _higher[1] = y;
    }

    /// Sets the lower corner of the rectangle.
    void SetLower(const GfVec2i& lower) {
        _lower = lower;
    }

    /// Sets the upper corner of the rectangle.
    void SetHigher(const GfVec2i& higher) {
        _higher = higher;
    }

    /// Returns the center point of the rectangle.
    GfVec2i GetCenter() const {
        return (_lower + _higher) / 2;
    }

    /// Move the rectangle by \p displ.
    void Translate(const GfVec2i& displacement) {
        _lower += displacement;
        _higher += displacement;
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
    /// \note If the left and right sides are coincident, the width is one.
    int GetWidth() const {
        return (_higher[0] - _lower[0]) + 1;
    }

    /// Returns the height of the rectangle.
    ///
    /// \note If the top and bottom sides are coincident, the height is one.
    int GetHeight() const {
        return (_higher[1] - _lower[1]) + 1;
    }

    /// Computes the intersection of two rectangles.
    GfRect2i GetIntersection(const GfRect2i& that) const {
        if(IsEmpty())
            return *this;
        else if(that.IsEmpty())
            return that;
        else
            return GfRect2i(GfVec2i(GfMax(_lower[0], that._lower[0]),
                                    GfMax(_lower[1], that._lower[1])),
                            GfVec2i(GfMin(_higher[0], that._higher[0]),
                                    GfMin(_higher[1], that._higher[1])));
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
            return GfRect2i(GfVec2i(GfMin(_lower[0], that._lower[0]),
                                    GfMin(_lower[1], that._lower[1])),
                            GfVec2i(GfMax(_higher[0], that._higher[0]),
                                    GfMax(_higher[1], that._higher[1])));
    }

    /// Computes the union of two rectangles
    /// \deprecated Use GetUnion() instead.
    GfRect2i Union(const GfRect2i& that) const {
        return GetUnion(that);
    }

    /// Returns true if the specified point in the rectangle.
    bool Contains(const GfVec2i& p) const {
        return ((p[0] >= _lower[0]) && (p[0] <= _higher[0]) &&
                (p[1] >= _lower[1]) && (p[1] <= _higher[1]));
    }

    friend inline size_t hash_value(const GfRect2i &r) {
        size_t h = 0;
        boost::hash_combine(h, r._lower);
        boost::hash_combine(h, r._higher);
        return h;
    }        

    /// Returns true if \p r1 and \p r2 are equal.
    friend bool operator==(const GfRect2i& r1, const GfRect2i& r2) {
	return r1._lower == r2._lower && r1._higher == r2._higher;
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
    GfVec2i _lower, _higher;
};

/// Output a GfRect2i using the format [(x y):(x y)].
/// \ingroup group_gf_DebuggingOutput
std::ostream& operator<<(std::ostream&, const GfRect2i&);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
