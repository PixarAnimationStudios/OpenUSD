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
#ifndef PXR_USD_IMAGING_USD_IMAGING_TEXT_RAWGLYPH_H
#define PXR_USD_IMAGING_USD_IMAGING_TEXT_RAWGLYPH_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec2f.h"

#include <assert.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \struct UsdImagingTextCtrlPoint
///
/// TrueType's control point.
///
struct UsdImagingTextCtrlPoint
{
    /// The position of the 2D point.
    GfVec2f _pos;

    /// Whether this point is on the curve.
    bool _isOnCurve = true;

    /// The constructor of the UsdImagingTextCtrlPoint
    UsdImagingTextCtrlPoint(float x1 = 0,
        float y1 = 0,
        bool isOnCurve1 = true)
        : _pos(x1, y1)
        , _isOnCurve(isOnCurve1)
    {
    }

    /// The constructor of the UsdImagingTextCtrlPoint
    UsdImagingTextCtrlPoint(GfVec2f& vec,
        bool isOnCurve1 = true)
        : _pos(vec)
        , _isOnCurve(isOnCurve1)
    {
    }
};

/// \class UsdImagingTextCurve
///
/// A closed Bezier curve which represents a curve in the text outline.
///
class UsdImagingTextCurve
{
protected:
    /// A list of control point for this curve.
    std::vector<UsdImagingTextCtrlPoint> _listCtrlPoints;

public:
    /// The constructor.
    UsdImagingTextCurve() = default;

    /// The destructor.
    ~UsdImagingTextCurve() = default;

    /// Clear all the TTCtrlPoints in this curve.
    void Clear() { _listCtrlPoints.clear(); }

    /// Append a new point in the list.
    void AddPoint(const UsdImagingTextCtrlPoint& pt)
    {
        size_t count = _listCtrlPoints.size();

        // If this point and the last point are both off-curve point,
        // the middle point of these two points should be on the curve.
        // So add the middle point.
        if (count && (!pt._isOnCurve) && (!_listCtrlPoints[count - 1]._isOnCurve))
        {
            GfVec2f ctrlPoint = (_listCtrlPoints[count - 1]._pos + pt._pos) / 2.0;
            _listCtrlPoints.push_back(UsdImagingTextCtrlPoint(ctrlPoint, true));
        }

        _listCtrlPoints.push_back(pt);
    }

    /// Get a point in the list.
    UsdImagingTextCtrlPoint GetPoint(int index) const
    {
        int sizeOfPointList = static_cast<int>(_listCtrlPoints.size());
        assert(index < sizeOfPointList);
        if (index < sizeOfPointList)
        {
            return _listCtrlPoints.at(index);
        }
        else
            return UsdImagingTextCtrlPoint();
    }

    /// Get the count of point in the list.
    int PointsCount() const { return (int)_listCtrlPoints.size(); }

    /// Return true if the curve have no TTCtrlPoints.
    bool IsEmpty() const { return (int)_listCtrlPoints.size() == 0; }

    /// Get the last UsdImagingTextCtrlPoint in the curve.
    bool LastPoint(UsdImagingTextCtrlPoint& ctrlPoint) const
    {
        assert(!IsEmpty());
        if (IsEmpty())
        {
            ctrlPoint = (_listCtrlPoints[_listCtrlPoints.size() - 1]);
            return false;
        }
        else
            return true;
    }

    /// Reverse the whole curve
    void Reverse()
    {
        size_t count = _listCtrlPoints.size();
        size_t halfCount = _listCtrlPoints.size() / 2;
        for (size_t i = 0; i < halfCount; i++)
        {
            UsdImagingTextCtrlPoint temp = _listCtrlPoints[i];
            _listCtrlPoints[i] = _listCtrlPoints[count - 1 - i];
            _listCtrlPoints[count - 1 - i] = temp;
        }
    }
};

/// \class UsdImagingTextRawGlyph
///
/// A set of Bezier curves which can compose a glyph's outline.
///
class UsdImagingTextRawGlyph
{
protected:
    /// A list of curves in the raw glyph.
    std::vector<std::shared_ptr<UsdImagingTextCurve>> _listCurves;

    /// Pointer to The current curve.
    std::shared_ptr<UsdImagingTextCurve> _currentCurve;

    /// The bound box of the glyph.
    GfVec2i _boundBoxMin;
    GfVec2i _boundBoxMax;

    /// The count of contours in each components.
    std::vector<int> _contoursInEachComponents;

public:
    /// The constructor.
    UsdImagingTextRawGlyph() { _currentCurve = std::make_shared<UsdImagingTextCurve>(); }

    /// The destructor.
    ~UsdImagingTextRawGlyph() = default;

    /// Clear all the curves in this glyph.
    void Clear()
    {
        _listCurves.clear();
        _currentCurve->Clear();
        _contoursInEachComponents.clear();
    }

    /// Append a new UsdImagingTextCtrlPoint in the current curve of this glyph.
    void AddPoint(const UsdImagingTextCtrlPoint& ctrlPoint) { _currentCurve->AddPoint(ctrlPoint); }

    /// Return the last UsdImagingTextCtrlPoint in the current curve of this glyph.
    void LastPoint(UsdImagingTextCtrlPoint& ctrlPoint) const { _currentCurve->LastPoint(ctrlPoint); }

    /// Close the current curve.
    void CloseCurve(bool reverseOutline)
    {
        // Close the curve.
        if (_currentCurve->PointsCount() != 2)
        {
            // If it is not a line segment, add the first point as the last point of the curve.
            UsdImagingTextCtrlPoint lastPoint = _currentCurve->GetPoint(0);
            _currentCurve->AddPoint(lastPoint);
        }

        // If the curve
        if (reverseOutline)
            _currentCurve->Reverse();
        _listCurves.push_back(std::move(_currentCurve));
        _currentCurve = std::make_shared<UsdImagingTextCurve>();
    }

    /// Return the number of curves in this glyph.
    int CurvesCount() const { return (int)_listCurves.size(); }

    /// Get the specified Curve
    const std::shared_ptr<UsdImagingTextCurve> GetCurve(int index) const
    {
        assert(index < static_cast<int>(_listCurves.size()));
        return _listCurves.at(index);
    }

    /// Set the bound box of this glyph.
    void SetBoundBox(int x0,
                     int y0,
                     int x1,
                     int y1)
    {
        _boundBoxMin = GfVec2i(x0, y0);
        _boundBoxMax = GfVec2i(x1, y1);
    }

    /// Get the min corner of bound box
    const GfVec2i& GetBoundBoxMin()
    {
        return _boundBoxMin;
    }

    /// Get the max corner of bound box
    const GfVec2i& GetBoundBoxMax()
    {
        return _boundBoxMax;
    }

    /// Add the count of contours in one component
    void AddComponent(int countOfContours) { _contoursInEachComponents.push_back(countOfContours); }

    /// The count of components
    int ComponentsCount() const { return (int)_contoursInEachComponents.size(); }

    /// Get the count of contours of one component.
    int GetComponent(int index) const { return _contoursInEachComponents.at(index); }
};
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_TEXT_RAWGLYPH_H
