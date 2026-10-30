/*
    src/nb_datetime.cpp: conversion between datetime objects and their integer fields

    Copyright (c) 2023 Hudson River Trading LLC <opensource@hudson-trading.com>
    Copyright (c) 2026 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#include "nb_internals.h"

#if !defined(Py_LIMITED_API) && !defined(PYPY_VERSION)
#  include <datetime.h>
#endif

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

#if !defined(Py_LIMITED_API) && !defined(PYPY_VERSION)

static bool import_datetime() noexcept {
    if (PyDateTimeAPI)
        return true;
    PyDateTime_IMPORT;
    return PyDateTimeAPI != nullptr;
}

int datetime_unpack(nb_internals *, PyObject *o, uint32_t accept,
                    int32_t *parts) noexcept {
    if (!import_datetime())
        return -1;

    if ((accept & (uint32_t) datetime_kind::datetime) && PyDateTime_Check(o)) {
        parts[0] = PyDateTime_GET_YEAR(o);
        parts[1] = PyDateTime_GET_MONTH(o);
        parts[2] = PyDateTime_GET_DAY(o);
        parts[3] = PyDateTime_DATE_GET_HOUR(o);
        parts[4] = PyDateTime_DATE_GET_MINUTE(o);
        parts[5] = PyDateTime_DATE_GET_SECOND(o);
        parts[6] = PyDateTime_DATE_GET_MICROSECOND(o);
        parts[7] = PyDateTime_DATE_GET_FOLD(o);
        return (int) datetime_kind::datetime;
    }

    if ((accept & (uint32_t) datetime_kind::date) && PyDate_Check(o)) {
        parts[0] = PyDateTime_GET_YEAR(o);
        parts[1] = PyDateTime_GET_MONTH(o);
        parts[2] = PyDateTime_GET_DAY(o);
        return (int) datetime_kind::date;
    }

    if ((accept & (uint32_t) datetime_kind::time) && PyTime_Check(o)) {
        parts[0] = PyDateTime_TIME_GET_HOUR(o);
        parts[1] = PyDateTime_TIME_GET_MINUTE(o);
        parts[2] = PyDateTime_TIME_GET_SECOND(o);
        parts[3] = PyDateTime_TIME_GET_MICROSECOND(o);
        parts[4] = PyDateTime_TIME_GET_FOLD(o);
        return (int) datetime_kind::time;
    }

    if ((accept & (uint32_t) datetime_kind::timedelta) && PyDelta_Check(o)) {
        parts[0] = PyDateTime_DELTA_GET_DAYS(o);
        parts[1] = PyDateTime_DELTA_GET_SECONDS(o);
        parts[2] = PyDateTime_DELTA_GET_MICROSECONDS(o);
        return (int) datetime_kind::timedelta;
    }

    return (int) datetime_kind::none;
}

PyObject *datetime_pack(nb_internals *, uint32_t kind,
                        const int32_t *parts) noexcept {
    if (!import_datetime())
        return nullptr;

    switch ((datetime_kind) kind) {
        case datetime_kind::datetime:
            return PyDateTime_FromDateAndTimeAndFold(
                parts[0], parts[1], parts[2], parts[3], parts[4], parts[5],
                parts[6], parts[7]);

        case datetime_kind::date:
            return PyDate_FromDate(parts[0], parts[1], parts[2]);

        case datetime_kind::time:
            return PyTime_FromTimeAndFold(parts[0], parts[1], parts[2],
                                          parts[3], parts[4]);

        case datetime_kind::timedelta:
            return PyDelta_FromDSU(parts[0], parts[1], parts[2]);

        default:
            PyErr_Format(PyExc_SystemError,
                         "nanobind::detail::datetime_pack(): unsupported "
                         "kind %u!", kind);
            return nullptr;
    }
}

#else // defined(Py_LIMITED_API) || defined(PYPY_VERSION)

/// Types of the 'datetime' module and their fields, both in 'datetime_kind'
/// bit order. Rows shorter than the largest layout are null-terminated.
static import_cache datetime_types[4] = {
    { "datetime", "datetime" }, { "datetime", "date" },
    { "datetime", "time" }, { "datetime", "timedelta" }
};

static const char *datetime_fields[4][8] = {
    { "year", "month", "day", "hour", "minute", "second", "microsecond",
      "fold" },
    { "year", "month", "day", nullptr },
    { "hour", "minute", "second", "microsecond", "fold", nullptr },
    { "days", "seconds", "microseconds", nullptr }
};

/// Resolve one of the types above. The result is a borrowed reference owned
/// by the internals lifeline, which resets the cache when it is torn down.
static PyObject *datetime_type(nb_internals *p, int index) noexcept {
    import_cache &c = datetime_types[index];
    PyObject *v = c.load();
    if (NB_UNLIKELY(!v))
        v = import_cached(p, &c);
    return v;
}

/// Set *dest to the integer value of getattr(o, name). Returns true
/// on success, false and sets the Python error indicator on failure.
/// The attribute value must be a Python integer object; other types
/// of numbers are not supported.
static bool set_from_int_attr(int32_t *dest, PyObject *o,
                              const char *name) noexcept {
    PyObject *value = PyObject_GetAttrString(o, name);
    if (!value)
        return false;
    long lval = PyLong_AsLong(value);
    if (lval == -1 && PyErr_Occurred()) {
        Py_DECREF(value);
        return false;
    }
    if (lval < (long) INT32_MIN || lval > (long) INT32_MAX) {
        PyErr_Format(PyExc_OverflowError,
                     "%R attribute '%s' (%R) does not fit in an int",
                     o, name, value);
        Py_DECREF(value);
        return false;
    }
    Py_DECREF(value);
    *dest = (int32_t) lval;
    return true;
}

int datetime_unpack(nb_internals *p, PyObject *o, uint32_t accept,
                    int32_t *parts) noexcept {
    PyTypeObject *tp = Py_TYPE(o);

    for (int i = 0; i < 4; ++i) {
        if (!(accept & ((uint32_t) 1 << i)))
            continue;

        PyObject *t = datetime_type(p, i);
        if (!t)
            return -1;
        if (!PyType_IsSubtype(tp, (PyTypeObject *) t))
            continue;

        const char **fields = datetime_fields[i];
        for (int j = 0; j < 8 && fields[j]; ++j) {
            if (!set_from_int_attr(parts + j, o, fields[j]))
                return -1;
        }

        return 1 << i;
    }

    return (int) datetime_kind::none;
}

/// Replace the 'fold' attribute of 'o' when it is nonzero (the Python-level
/// datetime/time constructors only accept it as a keyword-only argument).
/// Consumes and replaces the reference to 'o'.
static PyObject *set_fold(PyObject *o, int32_t fold) noexcept {
    if (!o || fold == 0)
        return o;
    PyObject *method = PyObject_GetAttrString(o, "replace"),
             *args = PyTuple_New(0),
             *kwargs = Py_BuildValue("{s:i}", "fold", (int) fold),
             *result = nullptr;
    if (method && args && kwargs)
        result = PyObject_Call(method, args, kwargs);
    Py_XDECREF(method);
    Py_XDECREF(args);
    Py_XDECREF(kwargs);
    Py_DECREF(o);
    return result;
}

PyObject *datetime_pack(nb_internals *p, uint32_t kind,
                        const int32_t *parts) noexcept {
    int index;
    switch ((datetime_kind) kind) {
        case datetime_kind::datetime:  index = 0; break;
        case datetime_kind::date:      index = 1; break;
        case datetime_kind::time:      index = 2; break;
        case datetime_kind::timedelta: index = 3; break;
        default:
            PyErr_Format(PyExc_SystemError,
                         "nanobind::detail::datetime_pack(): unsupported "
                         "kind %u!", kind);
            return nullptr;
    }

    PyObject *t = datetime_type(p, index);
    if (!t)
        return nullptr;

    switch ((datetime_kind) kind) {
        case datetime_kind::datetime:
            return set_fold(
                PyObject_CallFunction(t, "iiiiiii",
                                      parts[0], parts[1], parts[2], parts[3],
                                      parts[4], parts[5], parts[6]),
                parts[7]);

        case datetime_kind::time:
            return set_fold(
                PyObject_CallFunction(t, "iiii", parts[0], parts[1],
                                      parts[2], parts[3]),
                parts[4]);

        default: // date and timedelta take three integer fields
            return PyObject_CallFunction(t, "iii", parts[0], parts[1],
                                         parts[2]);
    }
}

#endif

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
