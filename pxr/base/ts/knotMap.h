//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_KNOT_MAP_H
#define PXR_BASE_TS_KNOT_MAP_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/knot.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/gf/interval.h"

#include <vector>
#include <initializer_list>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

struct Ts_SplineData;


/// The knots in a spline.  Stored as a vector, but unique and sorted like a
/// map.  A knot's time is stored within the knot itself, but is also sometimes
/// used as a key.  Some methods are vector-like, some are map-like, and some
/// are set-like.
///
/// Separate from TsSpline in order to provide identical operations on different
/// collections of knots.  The most straightforward collection is the authored
/// knots, provided by GetKnots, but collections included baked loop knots can
/// also be obtained.
///
class TsKnotMap
{
public:
    using KnotVec = std::vector<TsKnot>;

    using iterator = KnotVec::iterator;
    using const_iterator = KnotVec::const_iterator;
    using reverse_iterator = KnotVec::reverse_iterator;
    using const_reverse_iterator = KnotVec::const_reverse_iterator;

public:
    /// \name Construction and value semantics
    /// @{

    TS_API
    TsKnotMap();

    TS_API
    TsKnotMap(std::initializer_list<TsKnot> knots);

    TS_API
    bool operator==(const TsKnotMap &other) const;

    TS_API
    bool operator!=(const TsKnotMap &other) const;

    /// @}
    /// \name Iteration
    ///
    /// These methods are <code>std::vector</code>-like.
    ///
    /// @{

    TS_API
    iterator begin();

    TS_API
    const_iterator begin() const;

    TS_API
    const_iterator cbegin() const;

    TS_API
    iterator end();

    TS_API
    const_iterator end() const;

    TS_API
    const_iterator cend() const;

    TS_API
    reverse_iterator rbegin();

    TS_API
    const_reverse_iterator rbegin() const;

    TS_API
    const_reverse_iterator crbegin() const;

    TS_API
    reverse_iterator rend();

    TS_API
    const_reverse_iterator rend() const;

    TS_API
    const_reverse_iterator crend() const;

    /// @}
    /// \name Size
    ///
    /// These methods are <code>std::vector</code>-like.
    ///
    /// @{

    TS_API
    size_t size() const;

    TS_API
    bool empty() const;

    TS_API
    void reserve(size_t size);

    /// @}
    /// \name Modification
    ///
    /// These methods are <code>std::set</code>-like.
    ///
    /// @{

    TS_API
    void clear();

    TS_API
    void swap(TsKnotMap &other);

    /// Inserts a knot.  If there is already a knot at the same time, nothing is
    /// changed.  Returns an iterator to the newly inserted knot, or the
    /// existing one at the same time.  The second member of the returned pair
    /// indicates whether an insertion took place.
    TS_API
    std::pair<iterator, bool> insert(const TsKnot &knot);

    /// Removes the knot at the specified time, if it exists.  Returns the
    /// number of knots erased (0 or 1).
    TS_API
    size_t erase(TsTime time);

    /// Removes a knot.  Returns the iterator after it.
    TS_API
    iterator erase(iterator i);

    /// Removes a range of knots.  Returns the iterator after the last removed.
    TS_API
    iterator erase(iterator first, iterator last);

    /// @}
    /// \name Searching
    ///
    /// These methods are <code>std::map</code>-like.
    ///
    /// @{

    /// Exact matches only; returns end() if not found.
    TS_API
    iterator find(TsTime time);

    /// Const version of find().
    TS_API
    const_iterator find(TsTime time) const;

    /// If there is a knot at the specified time, returns that.  Otherwise, if
    /// there is a knot after the specified time, returns the first such knot.
    /// Otherwise returns end().
    TS_API
    iterator lower_bound(TsTime time);

    /// Const version of lower_bound().
    TS_API
    const_iterator lower_bound(TsTime time) const;

    /// @}
    /// \name Non-STL Methods
    /// @{

    /// Returns the knot whose time most closely (or exactly) matches the
    /// specified time.  In case of ties, returns the later knot.  If there are
    /// no knots, returns end().
    TS_API
    iterator FindClosest(TsTime time);

    /// Const version of FindClosest().
    TS_API
    const_iterator FindClosest(TsTime time) const;

    /// Returns the value type of the knots, or Unknown if empty.
    TS_API
    TfType GetValueType() const;

    /// Returns the time interval containing the first and last knot.  Returns
    /// an empty interval if there are no knots.
    TS_API
    GfInterval GetTimeSpan() const;

    /// Returns whether there are any segments with curve interpolation.
    TS_API
    bool HasCurveSegments() const;

    /// @}

private:
    friend class TsSpline;

    // Constructor for copying knot data from SplineData into KnotMap.
    TS_API
    TsKnotMap(const Ts_SplineData *data);

private:
    // Knot objects.
    KnotVec _knots;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
