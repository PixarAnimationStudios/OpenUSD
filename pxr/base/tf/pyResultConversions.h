//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_RESULT_CONVERSIONS_H
#define PXR_BASE_TF_PY_RESULT_CONVERSIONS_H

#include "pxr/pxr.h"

#include "pxr/base/tf/pyUtils.h"

#include <nanobind/nanobind.h>

#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

template <typename T> struct Tf_PySequenceToListConverter;
template <typename T> struct Tf_PySequenceToSetConverter;
template <typename T> struct Tf_PyMapToDictionaryConverter;
template <typename T> struct Tf_PySequenceToTupleConverter;
template <typename First, typename Second> struct Tf_PyPairToTupleConverter;

struct TfPySequenceToList {
    template <typename T>
    struct apply {
        typedef Tf_PySequenceToListConverter<T> type;
    };
};

struct TfPySequenceToSet {
    template <typename T>
    struct apply {
        typedef Tf_PySequenceToSetConverter<T> type;
    };
};

struct TfPyMapToDictionary {
    template <typename T>
    struct apply {
        typedef Tf_PyMapToDictionaryConverter<T> type;
    };
};

struct TfPySequenceToTuple {
    template <typename T>
    struct apply {
        typedef Tf_PySequenceToTupleConverter<T> type;
    };
};

struct TfPyPairToTuple {
    template <typename T>
    struct apply {
        typedef Tf_PyPairToTupleConverter<typename T::first_type,
                                          typename T::second_type> type;
    };
};

template <typename T>
struct Tf_PySequenceToListConverter {
    typedef std::remove_reference_t<T> SeqType;
    bool convertible() const {
        return true;
    }
    PyObject *operator()(T seq) const {
        return TfPyCopySequenceToList(seq).release().ptr();
    }
    PyTypeObject *get_pytype() {
        return &PyList_Type;
    }
};

template <typename T>
struct Tf_PySequenceToSetConverter {
    typedef std::remove_reference_t<T> SeqType;
    bool convertible() const {
        return true;
    }
    PyObject *operator()(T seq) const {
        return TfPyCopySequenceToSet(seq).release().ptr();
    }
    PyTypeObject *get_pytype() {
        return &PySet_Type;
    }
};

template <typename T>
struct Tf_PyMapToDictionaryConverter {
    typedef std::remove_reference_t<T> SeqType;
    bool convertible() const {
        return true;
    }
    PyObject *operator()(T seq) const {
        return TfPyCopyMapToDictionary(seq).release().ptr();
    }
    PyTypeObject *get_pytype() {
        return &PyDict_Type;
    }
};

template <typename T>
struct Tf_PySequenceToTupleConverter {
    typedef std::remove_reference_t<T> SeqType;
    bool convertible() const {
        return true;
    }
    PyObject *operator()(T seq) const {
        return TfPyCopySequenceToTuple(seq).release().ptr();
    }
    PyTypeObject *get_pytype() {
        return &PyTuple_Type;
    }
};

template <typename First, typename Second>
struct Tf_PyPairToTupleConverter {
    typedef std::pair<First, Second> PairType;
    bool convertible() const {
        return true;
    }
    PyObject *operator()(PairType const& a) const {
        return nanobind::make_tuple(a.first, a.second).release().ptr();
    }
    PyTypeObject *get_pytype() {
        return &PyTuple_Type;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_RESULT_CONVERSIONS_H
