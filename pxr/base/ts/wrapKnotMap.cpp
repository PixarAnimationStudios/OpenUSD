//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/knotMap.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;


static int _WrapLen(const TsKnotMap &knotMap)
{
    return knotMap.size();
}

// Iteration is implemented on a copy of the knots.
//
static PyObject* _WrapIter(const TsKnotMap &knotMap)
{
    // Make a native dict that is a copy of the (time, knot) pairs.
    dict d;
    for (const TsKnot &knot : knotMap)
        d[knot.GetTime()] = knot;

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

static object
_WrapFindClosest(const TsKnotMap &knotMap, const TsTime time)
{
    const auto it = knotMap.FindClosest(time);
    if (it != knotMap.end())
    {
        return object(*it);
    }
    else
    {
        return object();
    }
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


void wrapKnotMap()
{
    using This = TsKnotMap;

    class_<This>("KnotMap", no_init)

        .def(init<>())
        .def(init<const TsKnotMap &>())

        .def(self == self)
        .def(self != self)

        // Dict-like interface.  All iteration is on copies of the knot data.
        .def("__len__", &_WrapLen)
        .def("__iter__", &_WrapIter)
        .def("__getitem__", &_WrapGetItem)
        .def("__contains__", &_WrapContains)
        .def("keys", &_WrapKeys,
            return_value_policy<TfPySequenceToList>())
        .def("values", &_WrapValues,
            return_value_policy<TfPySequenceToList>())
        .def("__setitem__", &_WrapSetItem)
        .def("__delitem__", &_WrapDelItem)
        .def("clear", &This::clear)

        .def("FindClosest", &_WrapFindClosest)
        .def("GetValueType", &This::GetValueType)
        .def("GetTimeSpan", &This::GetTimeSpan)
        .def("HasCurveSegments", &This::HasCurveSegments)

        ;
}
