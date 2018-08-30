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
#ifndef TF_PYRESULTCONVERSIONS_H
#define TF_PYRESULTCONVERSIONS_H

#include "pxr/pxr.h"

#include "pxr/base/tf/pyUtils.h"

#include <boost/python/tuple.hpp>
#include <boost/python/list.hpp>
#include <boost/python/dict.hpp>

#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/remove_reference.hpp>

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

template <typename T> struct Tf_PySequenceToListConverter;
template <typename T> struct Tf_PySequenceToSetConverter;
template <typename T> struct Tf_PyMapToDictionaryConverter;
template <typename T> struct Tf_PySequenceToTupleConverter;
template <typename First, typename Second> struct Tf_PyPairToTupleConverter;

/// \class TfPySequenceToList
///
/// A \c boost::python result converter generator which converts standard
/// library sequences to lists.
///
/// The way to use this is as a return value policy for a function which
/// returns a sequence or a const reference to a sequence.  For example this
/// function:
/// \code
/// vector<double> getDoubles() {
///     vector<double> ret;
///     ret.push_back(1.0);
///     ret.push_back(2.0);
///     ret.push_back(3.0);
///     return ret;
/// }
/// \endcode
///
/// May be wrapped as:
/// \code
/// def("getDoubles", &getDoubles, return_value_policy<TfPySequenceToList>())
/// \endcode
struct TfPySequenceToList {
    template <typename T>
    struct apply {
        typedef Tf_PySequenceToListConverter<T> type;
    };
};

/// \class TfPySequenceToSet
///
/// A \c boost::python result converter generator which converts standard
/// library sequences to sets.
///
/// The way to use this is as a return value policy for a function which
/// returns a sequence or a const reference to a sequence.  For example this
/// function:
/// \code
/// unordered_set<double> getDoubles() {
///     unordered_set<double> ret;
///     ret.insert(1.0);
///     ret.insert(2.0);
///     ret.insert(3.0);
///     return ret;
/// }
/// \endcode
///
/// May be wrapped as:
/// \code
/// def("getDoubles", &getDoubles, return_value_policy<TfPySequenceToSet>())
/// \endcode
struct TfPySequenceToSet {
    template <typename T>
    struct apply {
        typedef Tf_PySequenceToSetConverter<T> type;
    };
};

/// \class TfPyMapToDictionary
///
/// A \c boost::python result converter generator which converts standard
/// library maps to dictionaries.
struct TfPyMapToDictionary {
    template <typename T>
    struct apply {
        typedef Tf_PyMapToDictionaryConverter<T> type;
    };
};

/// \class TfPySequenceToTuple
///
/// A \c boost::python result converter generator which converts standard
/// library sequences to tuples.
/// \see TfPySequenceToList.
struct TfPySequenceToTuple {
    template <typename T>
    struct apply {
        typedef Tf_PySequenceToTupleConverter<T> type;
    };
};

/// A \c boost::python result converter generator which converts standard
/// library pairs to tuples.
struct TfPyPairToTuple {
    template <typename T>
    struct apply {
        typedef Tf_PyPairToTupleConverter<typename T::first_type,
                                          typename T::second_type> type;
    };
};

template <typename T>
struct Tf_PySequenceToListConverter {
    typedef typename boost::remove_reference<T>::type SeqType;
    bool convertible() const {
        return true;
    }
    PyObject *operator()(T seq) const {
        return boost::python::incref(TfPyCopySequenceToList(seq).ptr());
    }
    PyTypeObject *get_pytype() {
        return &PyList_Type;
    }
};

template <typename T>
struct Tf_PySequenceToSetConverter {
    typedef typename std::remove_reference<T>::type SeqType;
    bool convertible() const {
        return true;
    }
    PyObject *operator()(T seq) const {
        return boost::python::incref(TfPyCopySequenceToSet(seq).ptr());
    }
    PyTypeObject *get_pytype() {
        return &PySet_Type;
    }
};

template <typename T>
struct Tf_PyMapToDictionaryConverter {
    typedef typename boost::remove_reference<T>::type SeqType;
    // TODO: convertible() should be made more robust by checking that the
    // value_type of the container is pair<const key_type, data_type> 
    bool convertible() const {
        return true;
    }
    PyObject *operator()(T seq) const {
        return boost::python::incref(TfPyCopyMapToDictionary(seq).ptr());
    }
    PyTypeObject *get_pytype() {
        return &PyDict_Type;
    }
};

template <typename T>
struct Tf_PySequenceToTupleConverter {
    typedef typename boost::remove_reference<T>::type SeqType;
    bool convertible() const {
        return true;
    }
    PyObject *operator()(T seq) const {
        return boost::python::incref(TfPyCopySequenceToTuple(seq).ptr());
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
        boost::python::tuple result =
            boost::python::make_tuple(a.first, a.second);
        return boost::python::incref(result.ptr());
    }
    PyTypeObject *get_pytype() {
        return &PyTuple_Type;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_RESULT_CONVERSIONS_H
