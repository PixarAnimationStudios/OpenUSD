//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/knotMap.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

static int _WrapLen(const TsKnotMap &knotMap)
{
    return knotMap.size();
}

// Iteration is implemented on a copy of the knots.
//
static PyObject* _WrapIter(const TsKnotMap &knotMap)
{
    // Make a native dict that is a copy of the (time, knot) pairs.
    nb::dict d;
    for (const TsKnot &knot : knotMap)
        d[nb::cast(knot.GetTime())] = nb::cast(knot);

    // Return a native Python iterator for the dict.  This will INCREF the dict,
    // and will be the only refcount on it, so that it is freed when iteration
    // is complete.
    return PyObject_GetIter(d.ptr());
}

static TsKnot _WrapGetItem(const TsKnotMap &knotMap, const TsTime &time)
{
    const TsKnotMap::const_iterator it = knotMap.find(time);
    if (it == knotMap.end())
    {
        TfPyThrowIndexError(TfStringPrintf("No knot at time %g", time));
    }

    return *it;
}

static bool _WrapContains(const TsKnotMap &knotMap, const TsTime &time)
{
    return knotMap.find(time) != knotMap.end();
}

// Iteration is implemented on a copy of the knots.
//
static std::vector<TsTime>
_WrapKeys(const TsKnotMap &knotMap)
{
    std::vector<TsTime> result;
    result.reserve(knotMap.size());

    for (const TsKnot &knot : knotMap)
        result.push_back(knot.GetTime());

    return result;
}

// Iteration is implemented on a copy of the knots.
//
static std::vector<TsKnot>
_WrapValues(const TsKnotMap &knotMap)
{
    return std::vector<TsKnot>(knotMap.begin(), knotMap.end());
}

// The C++ KnotMap interface is a hybrid of vector and map.  Knot objects can be
// looked up by time, but they also contain their own time.  Our Python
// interface is dict-like, so our mutator interface is map[time] = knot.  This
// gives us two potentially conflicting sources of time coordinate, from the key
// and the value.  We resolve this by insisting that both be the same.  It would
// also be possible to have a method like map.Set(knot), but that would not be
// dict-like.
//
// Also note that map[time] = knot is unconditional: any prior knot at the
// specified time is replaced.  This matches the behavior of TsSpline::SetKnot,
// but not the behavior of TsKnotMap::insert, which is map-like, and does
// nothing when there is an existing knot at the same time.
//
static void _WrapSetItem(
    TsKnotMap &knotMap,
    const TsTime time,
    const TsKnot &knot)
{
    if (knot.GetTime() != time)
    {
        TF_CODING_ERROR(
            "Ts.KnotMap.__setitem__: key does not match value.GetTime");
        return;
    }

    knotMap.erase(time);
    knotMap.insert(knot);
}

static void _WrapDelItem(
    TsKnotMap &knotMap,
    const TsTime time)
{
    knotMap.erase(time);
}

void wrapKnotMap(nanobind::module_& m)
{
    using This = TsKnotMap;

    nb::class_<This>(m, "KnotMap")

        .def(nb::init<>())
        .def(nb::init<const TsKnotMap &>())

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)

        // Dict-like interface.  All iteration is on copies of the knot data.
        .def("__len__", &_WrapLen)
        .def("__iter__", [](const TsKnotMap& knotMap) {
            return nb::steal<nb::object>(_WrapIter(knotMap));
        })
        .def("__getitem__", &_WrapGetItem)
        .def("__contains__", &_WrapContains)
        .def("keys", &_WrapKeys)
        .def("values", &_WrapValues)
        .def("__setitem__", &_WrapSetItem)
        .def("__delitem__", &_WrapDelItem)
        .def("clear", &This::clear)

        .def("FindClosest", [](const TsKnotMap& knotMap, TsTime time) -> nb::object {
            const auto it = knotMap.FindClosest(time);
            if (it != knotMap.end()) {
                return nb::cast(*it);
            }
            return nb::none();
        })
        .def("GetValueType", &This::GetValueType)
        .def("GetTimeSpan", &This::GetTimeSpan)
        .def("HasCurveSegments", &This::HasCurveSegments)

        ;
}
