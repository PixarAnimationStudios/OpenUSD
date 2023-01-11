//
// Copyright 2023 Pixar
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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_MATH_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_MATH_H

#include "definitions.h"
#include "globals.h"
#include <cmath>
#include <climits>

#define IsFloatEqual(a, b) GfIsClose(a, b, M_EPSILON)

PXR_NAMESPACE_OPEN_SCOPE


/// \struct CommonTextBox2
///
/// Minimal 2d box structure, made of 2 GfVec2is.
///
template <class VecType>
struct CommonTextBox2
{
private:
    /// Page box min & max point
    VecType _min, _max;

    /// Creates an empty box, we cannot have a working parameterless constructor on structures.
public:
    /// The default constructor.
    inline CommonTextBox2()
    {
        // Makes a valid empty box.
        Clear();
    }

    /// The constructor from another Box2.
    template <class VecType2>
    inline CommonTextBox2(const CommonTextBox2<VecType2>& rhs)
    {
        _min = rhs.Min();
        _max = rhs.Max();
    }

    /// Creates a box2.
    inline CommonTextBox2(VecType min, 
                          VecType max) 
        : _min(min)
        , _max(max) 
    {}

    /// Creates a box2.
    inline CommonTextBox2(typename VecType::ScalarType xMin, 
                          typename VecType::ScalarType yMin,
                          typename VecType::ScalarType xMax, 
                          typename VecType::ScalarType yMax)
    {
        _min = VecType(xMin, yMin);
        _max = VecType(xMax, yMax);
    }

    /// Copies a box2's contents.
    inline CommonTextBox2& operator=(const CommonTextBox2& rhs)
    {
        // Avoids self-assignment.
        if (this == &rhs)
            return *this;

        _min = rhs._min;
        _max = rhs._max;
        return *this;
    }

    /// Sets a box2.
    inline void Set(typename VecType::ScalarType xMin,
                    typename VecType::ScalarType yMin,
                    typename VecType::ScalarType xMax, 
                    typename VecType::ScalarType yMax)
    {
        _min.Set(xMin, yMin);
        _max.Set(xMax, yMax);
    }

    /// Gets the box minimum.
    inline VecType Min() const { return _min; }

    /// Sets the box minimum.
    inline void Min(VecType value) { _min = value; }

    /// Gets the box maximum.
    inline VecType Max() const { return _max; }

    /// Sets the box maximum.
    inline void Max(VecType value) { _max = value; }

    /// Gets and sets the box size.
    inline VecType Size() const { return _max - _min; }

    /// Gets and sets the box radius.
    inline float Radius() const
    {
        VecType d = _max - _min;
        float length = static_cast<float>(sqrt((double)(d * d)));
        float len    = 0.5f * length;
        return len;
    }

    /// Gets and sets the box width.
    inline typename VecType::ScalarType Width() const { return _max[0] - _min[0]; }

    /// Gets and sets the box height.
    inline typename VecType::ScalarType Height() const { return _max[1] - _min[1]; }

    /// Checks if the box is empty.
    inline bool IsEmpty() const { return _min[0] == INT_MAX && _max[0] == INT_MIN; }

    /// clear the box to empty.
    inline void Clear()
    {
        _min.Set(std::numeric_limits<typename VecType::ScalarType>::max(),
            std::numeric_limits<typename VecType::ScalarType>::max());
        _max.Set(std::numeric_limits<typename VecType::ScalarType>::min(),
            std::numeric_limits<typename VecType::ScalarType>::min());
    }

    /// Checks if the point contained is in the box.
    inline bool IsInBox(VecType p) const
    {
        if ((p[0] >= _min[0]) && (p[0] <= _max[0]) && (p[1] >= _min[1]) && (p[1] <= _max[1]))
            return true;
        else
            return false;
    }

    /// Checks if this box is within the containing box.
    inline bool IsWithinBox(CommonTextBox2& containingBox) const
    {
        if (containingBox.IsInBox(_min) && containingBox.IsInBox(_max))
            return true;
        else
            return false;
    }

    /// Checks if the 2 boxes touches or crosses each other.
    inline bool Intersects(CommonTextBox2& b) const
    {
        if ((_max[0] < b._min[0]) || (_max[1] < b._min[1]) || (_min[0] > b._max[0]) ||
            (_min[1] > b._max[1]))
            return false;
        else
            return true;
    }

    /// Clip the box to the box b.
    inline void Clip(CommonTextBox2& b)
    {
        _min.Set((std::max)(_min[0], b._min[0]), (std::max)(_min[1], b._min[1]));
        _max.Set((std::min)(_max[0], b._max[0]), (std::min)(_max[1], b._max[1]));
    }

    /// Adds new box to this box.
    inline void AddBox(CommonTextBox2& b)
    {
        // enlarge box by merging with new box
        if (b._min[0] < _min[0])
            _min[0] = b._min[0];
        if (b._min[1] < _min[1])
            _min[1] = b._min[1];

        if (b._max[0] > _max[0])
            _max[0] = b._max[0];
        if (b._max[1] > _max[1])
            _max[1] = b._max[1];
    }

    /// Adds a point to this box.
    inline void AddPoint(typename VecType::ScalarType x, 
                         typename VecType::ScalarType y)
    { 
        AddPoint(VecType(x, y));
    }

    /// Adds a point to this box.
    inline void AddPoint(const VecType& pt)
    {
        // enlarge box by merging point with new box
        if (pt[0] < _min[0])
            _min[0] = pt[0];
        if (pt[1] < _min[1])
            _min[1] = pt[1];

        if (pt[0] > _max[0])
            _max[0] = pt[0];
        if (pt[1] > _max[1])
            _max[1] = pt[1];
    }

    /// Find intersection of the two boxes, i.e. where both boxes exist.
    /// This method takes the original box and "applies" the input box to it,
    /// finding the overlap of the two. If the resulting box is "empty",
    /// no overlap, "false" is returned (the box is also given "empty"
    /// coordinates by using Clear()). True is returned if the resulting
    /// rectangle is not empty, i.e. there is overlap.
    /// \returns true if the boxes overlap, false if no overlap.
    inline bool IntersectBox(CommonTextBox2& b)
    {
        if (Intersects(b))
        {
            // minimize box by intersecting with input box
            if (b._min[0] > _min[0])
                _min[0] = b._min[0];
            if (b._min[1] > _min[1])
                _min[1] = b._min[1];

            if (b._max[0] < _max[0])
                _max[0] = b._max[0];
            if (b._max[1] < _max[1])
                _max[1] = b._max[1];
            return true;
        }
        else
        {
            // the boxes don't overlap, so the new box is empty.
            Clear();
            return false;
        }
    }

    /// Returns the diagonal vector of the box..
    /// \returns The vector running from the maximum to the minimum corner.
    inline VecType Diagonal() const
    {
        return _max - _min;
    }

    /// The indexer gets the points of the 4 corners of the box, min y quad then max y quad.
    inline VecType operator[](int index) const
    {
        switch (index)
        {
        case 0:
            return VecType(_min[0], _min[1]);
        case 1:
            return VecType(_max[0], _min[1]);
        case 2:
            return VecType(_min[0], _max[1]);
        case 3:
            return VecType(_max[0], _max[1]);
        default:
            // TODO need to add error message here.
            return _min;
        }
    }

    /// Computes most distant corner index of the box for a given direction vector.
    static inline int FarthestBoxCornerIndex(const VecType& v)
    {
        // Calculates the fastest corner index. The idea is that any given vector points towards one
        // of the 4 quadrants of 2D space. For each quadrant, there is one box corner that
        // is the farthest along that vector, i.e. that gives the largest value when
        // a dot-product with the vector is done. This method identifies that corner index.
        // The box itself is not necessary; all box corners at the index will be the farthest
        // along the vector.
        //
        // If plane points to -X, -Y, value is 0.
        // +X, -Y == 1
        // -X, +Y == 2
        // on up to +X, +Y == 3.
        // This matches the CommonTextBox2's VecType this[ int index ] ordering.
        return (((v[0] < 0) ? 0 : 1) + ((v[1] < 0) ? 0 : 2));
    }

    /// Equality operator.
    inline bool operator==(const CommonTextBox2& box) const
    {
        return (_min == box._min) && (_max == box._max);
    }

    /// Translate the box in x direction.
    void TranslateInX(typename VecType::ScalarType value)
    {
        _min[0] += value;
        _max[0] += value;
    }

}; // CommonTextBox2

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_STYLE_H
