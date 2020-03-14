//
// Copyright 2019 Pixar
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
#ifndef PXR_USD_USD_UTILS_TIME_CODE_RANGE_H
#define PXR_USD_USD_UTILS_TIME_CODE_RANGE_H

/// \file usdUtils/timeCodeRange.h

#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/api.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/usd/timeCode.h"

#include <iosfwd>
#include <iterator>
#include <string>


PXR_NAMESPACE_OPEN_SCOPE


#define USDUTILS_TIME_CODE_RANGE_TOKENS \
    ((EmptyTimeCodeRange, "NONE")) \
    ((RangeSeparator, ":")) \
    ((StrideSeparator, "x"))

TF_DECLARE_PUBLIC_TOKENS(
    UsdUtilsTimeCodeRangeTokens,
    USDUTILS_API,
    USDUTILS_TIME_CODE_RANGE_TOKENS);


/// \class UsdUtilsTimeCodeRange
///
/// Represents a range of UsdTimeCode values as start and end time codes and a
/// stride value.
///
/// A UsdUtilsTimeCodeRange can be iterated to retrieve all time code values in
/// the range. The range may be empty, it may contain a single time code, or it
/// may represent multiple time codes from start to end. The interval defined
/// by the start and end time codes is closed on both ends.
///
/// Note that when constructing a UsdUtilsTimeCodeRange,
/// UsdTimeCode::EarliestTime() and UsdTimeCode::Default() cannot be used as
/// the start or end time codes. Also, the end time code cannot be less than
/// the start time code for positive stride values, and the end time code
/// cannot be greater than the start time code for negative stride values.
/// Finally, the stride value cannot be zero. If any of these conditions are
/// not satisfied, then an invalid empty range will be returned.
class UsdUtilsTimeCodeRange
{
public:

    /// \class const_iterator
    ///
    /// A forward iterator into a UsdUtilsTimeCodeRange.
    class const_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = UsdTimeCode;
        using reference = const UsdTimeCode&;
        using pointer = const UsdTimeCode*;
        using difference_type = std::ptrdiff_t;

        /// Returns the UsdTimeCode referenced by this iterator.
        reference operator*() {
            return _currTimeCode;
        }

        /// Returns a pointer to the UsdTimeCode referenced by this iterator.
        pointer operator->() {
            return &_currTimeCode;
        }

        /// Pre-increment operator. Advances this iterator to the next
        /// UsdTimeCode in the range.
        ///
        /// This iterator is returned.
        const_iterator& operator++() {
            if (_timeCodeRange) {
                ++_currStep;
                _currTimeCode =
                    UsdTimeCode(
                        _timeCodeRange->_startTimeCode.GetValue() +
                        _timeCodeRange->_stride * _currStep);
            }
            _InvalidateIfExhausted();
            return *this;
        }

        /// Post-increment operator. Advances this iterator to the next
        /// UsdTimeCode in the range.
        ///
        /// A copy of this iterator prior to the increment is returned.
        const_iterator operator++(int) {
            const_iterator preAdvanceIter = *this;
            ++(*this);
            return preAdvanceIter;
        }

        /// Return true if this iterator is equivalent to \p other.
        bool operator ==(const const_iterator& other) const {
            return _timeCodeRange == other._timeCodeRange &&
                _currStep == other._currStep;
        }

        /// Return true if this iterator is not equivalent to \p other.
        bool operator !=(const const_iterator& other) const {
            return !(*this == other);
        }

    private:
        friend class UsdUtilsTimeCodeRange;

        const_iterator(const UsdUtilsTimeCodeRange* timeCodeRange) :
            _timeCodeRange(timeCodeRange),
            _currStep(0u),
            _maxSteps(0u),
            _currTimeCode()
        {
            if (_timeCodeRange) {
                const double startVal = _timeCodeRange->_startTimeCode.GetValue();
                const double endVal = _timeCodeRange->_endTimeCode.GetValue();
                const double stride = _timeCodeRange->_stride;

                _maxSteps = static_cast<size_t>(
                    GfFloor((endVal - startVal + stride) / stride));
                _currTimeCode = _timeCodeRange->_startTimeCode;
            }

            _InvalidateIfExhausted();
        }

        void _InvalidateIfExhausted() {
            bool finished = false;
            if (!_timeCodeRange) {
                finished = true;
            } else if (_currStep >= _maxSteps) {
                finished = true;
            }

            if (finished) {
                _timeCodeRange = nullptr;
                _currStep = 0u;
                _maxSteps = 0u;
                _currTimeCode = UsdTimeCode();
            }
        }

        const UsdUtilsTimeCodeRange* _timeCodeRange;
        size_t _currStep;
        size_t _maxSteps;
        UsdTimeCode _currTimeCode;
    };

    using iterator = const_iterator;

    /// Create a time code range from \p frameSpec.
    ///
    /// A FrameSpec is a compact string representation of a time code range.
    /// A FrameSpec may contain up to three floating point values for the start
    /// time code, end time code, and stride values of a time code range.
    ///
    /// A FrameSpec containing just a single floating point value represents
    /// a time code range containing only that time code.
    ///
    /// A FrameSpec containing two floating point values separated by the range
    /// separator (':') represents a time code range from the first value as
    /// the start time code to the second values as the end time code.
    ///
    /// A FrameSpec that specifies both a start and end time code value may
    /// also optionally specify a third floating point value as the stride,
    /// separating it from the first two values using the stride separator
    /// ('x').
    ///
    /// The following are examples of valid FrameSpecs:
    ///     123
    ///     101:105
    ///     105:101
    ///     101:109x2
    ///     101:110x2
    ///     101:104x0.5
    ///
    /// An empty string corresponds to an invalid empty time code range.
    ///
    /// A coding error will be issued if the given string is malformed.
    USDUTILS_API
    static UsdUtilsTimeCodeRange CreateFromFrameSpec(
            const std::string& frameSpec);

    /// Construct an invalid empty range.
    ///
    /// The start time code will be initialized to zero, and any iteration of
    /// the range will yield no time codes.
    UsdUtilsTimeCodeRange()
    {
        _Invalidate();
    }

    /// Construct a range containing only the given \p timeCode.
    ///
    /// An iteration of the range will yield only that time code.
    UsdUtilsTimeCodeRange(const UsdTimeCode timeCode) :
        UsdUtilsTimeCodeRange(timeCode, timeCode)
    {
    }

    /// Construct a range containing the time codes from \p startTimeCode to
    /// \p endTimeCode.
    ///
    /// If \p endTimeCode is greater than or equal to \p startTimeCode, then
    /// the stride will be 1.0. Otherwise, the stride will be -1.0.
    UsdUtilsTimeCodeRange(
            const UsdTimeCode startTimeCode,
            const UsdTimeCode endTimeCode) :
        UsdUtilsTimeCodeRange(
            startTimeCode,
            endTimeCode,
            (endTimeCode >= startTimeCode) ? 1.0 : -1.0)
    {
    }

    /// Construct a range containing the time codes from \p startTimeCode to
    /// \p endTimeCode using the stride value \p stride.
    ///
    /// UsdTimeCode::EarliestTime() and UsdTimeCode::Default() cannot be used
    /// as \p startTimeCode or \p endTimeCode. If \p stride is a positive
    /// value, then \p endTimeCode cannot be less than \p startTimeCode. If
    /// \p stride is a negative value, then \p endTimeCode cannot be greater
    /// than \p startTimeCode. Finally, the stride value cannot be zero. If any
    /// of these conditions are not satisfied, then a coding error will be
    /// issued and an invalid empty range will be returned.
    UsdUtilsTimeCodeRange(
            const UsdTimeCode startTimeCode,
            const UsdTimeCode endTimeCode,
            const double stride) :
        _startTimeCode(startTimeCode),
        _endTimeCode(endTimeCode),
        _stride(stride)
    {
        if (_startTimeCode.IsEarliestTime()) {
            TF_CODING_ERROR(
                "startTimeCode cannot be UsdTimeCode::EarliestTime()");
            _Invalidate();
            return;
        }
        if (_startTimeCode.IsDefault()) {
            TF_CODING_ERROR(
                "startTimeCode cannot be UsdTimeCode::Default()");
            _Invalidate();
            return;
        }
        if (_endTimeCode.IsEarliestTime()) {
            TF_CODING_ERROR(
                "endTimeCode cannot be UsdTimeCode::EarliestTime()");
            _Invalidate();
            return;
        }
        if (_endTimeCode.IsDefault()) {
            TF_CODING_ERROR(
                "endTimeCode cannot be UsdTimeCode::Default()");
            _Invalidate();
            return;
        }

        if (_stride > 0.0) {
            if (_endTimeCode < _startTimeCode) {
                TF_CODING_ERROR(
                    "endTimeCode cannot be less than startTimeCode with "
                    "positive stride");
                _Invalidate();
                return;
            }
        } else if (_stride < 0.0) {
            if (_endTimeCode > _startTimeCode) {
                TF_CODING_ERROR(
                    "endTimeCode cannot be greater than startTimeCode with "
                    "negative stride");
                _Invalidate();
                return;
            }
        } else {
            TF_CODING_ERROR("stride cannot be zero");
            _Invalidate();
            return;
        }
    }

    /// Return the start time code of this range.
    UsdTimeCode GetStartTimeCode() const {
        return _startTimeCode;
    }

    /// Return the end time code of this range.
    UsdTimeCode GetEndTimeCode() const {
        return _endTimeCode;
    }

    /// Return the stride value of this range.
    double GetStride() const {
        return _stride;
    }

    /// Return an iterator to the start of this range.
    iterator begin() const {
        return iterator(this);
    }

    /// Return a const_iterator to the start of this range.
    const_iterator cbegin() const {
        return const_iterator(this);
    }

    /// Return the past-the-end iterator for this range.
    iterator end() const {
        return iterator(nullptr);
    }

    /// Return the past-the-end const_iterator for this range.
    const_iterator cend() const {
        return const_iterator(nullptr);
    }

    /// Return true if this range contains no time codes, or false otherwise.
    bool empty() const {
        return begin() == end();
    }

    /// Return true if this range contains one or more time codes, or false
    /// otherwise.
    bool IsValid() const {
        return !empty();
    }

    /// Return true if this range contains one or more time codes, or false
    /// otherwise.
    explicit operator bool() const {
        return IsValid();
    }

    /// Return true if this range is equivalent to \p other.
    bool operator ==(const UsdUtilsTimeCodeRange& other) const {
        return _startTimeCode == other._startTimeCode &&
            _endTimeCode == other._endTimeCode &&
            _stride == other._stride;
    }

    /// Return true if this range is not equivalent to \p other.
    bool operator !=(const UsdUtilsTimeCodeRange& other) const {
        return !(*this == other);
    }

private:

    /// Sets the range such that it yields no time codes.
    void _Invalidate() {
        _startTimeCode = UsdTimeCode(0.0);
        _endTimeCode = UsdTimeCode(-1.0);
        _stride = 1.0;
    }

    UsdTimeCode _startTimeCode;
    UsdTimeCode _endTimeCode;
    double _stride;
};

// Stream I/O operators.

/// Stream insertion operator.
USDUTILS_API
std::ostream& operator<<(
        std::ostream& os,
        const UsdUtilsTimeCodeRange& timeCodeRange);

/// Stream extraction operator.
USDUTILS_API
std::istream& operator>>(
        std::istream& is,
        UsdUtilsTimeCodeRange& timeCodeRange);


PXR_NAMESPACE_CLOSE_SCOPE


#endif
