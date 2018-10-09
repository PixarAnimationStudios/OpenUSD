//
// Copyright 2018 Pixar
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

#include "pxr/pxr.h"
#include "pxr/base/tf/pyInterpreter.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/import.hpp>
#include <boost/python/operators.hpp>

#include <cstdio>
#include <tuple>
#include <unordered_set>
#include <vector>

using namespace boost::python;
PXR_NAMESPACE_USING_DIRECTIVE

namespace
{
    struct Unhashable
    {
    };

    bool operator==(const Unhashable &, const Unhashable &) {
        return true;
    }
    bool operator!=(const Unhashable &, const Unhashable &) {
        return false;
    }
}

class Tf_TestPyResultConversions
{
public:
    Tf_TestPyResultConversions()
        : _sourceVec{ 1, 1, 2, 2, 2, 3, 4, 4, 5, 5, 5, 5 }
    {}

    std::vector<int> GetEmptyVec() const {
        return std::vector<int>();
    }

    std::vector<int> GetUniqueVec() const {
        std::vector<int> v = _sourceVec;
        std::sort(v.begin(), v.end());
        v.erase(
            std::unique(v.begin(), v.end()),
            v.end());
        return v;
    }

    const std::vector<int> &GetDuplicateVec() const {
        return _sourceVec;
    }

    std::unordered_set<int> GetEmptySet() const {
        return std::unordered_set<int>();
    }

    std::unordered_set<int> GetSet() const {
        return std::unordered_set<int>(_sourceVec.begin(), _sourceVec.end());
    }

    // Returns a vector of unhashable items.  This should succeed for
    // TfPySequenceTo{List,Tuple} but generate a runtime exception for
    // TfPySequenceToSet.
    std::vector<Unhashable> GetUnhashableVec() const {
        return std::vector<Unhashable>(13);
    };

    std::pair<int, double> GetPair() const {
        return { 1, 2.0 };
    }

private:
    const std::vector<int> &_GetSourceVec() const {
        return _sourceVec;
    }

private:
    std::vector<int> _sourceVec;
};

// Check that a Python list or tuple contains the same items, in the same
// order, as a C++ vector.
template <typename T>
static void
AssertPySeqVecEqual(
    boost::python::object seq,
    const std::vector<T> &vec,
    const char *file,
    const int line)
{
    try {
        const int seqSize = len(seq);
        const int vecSize = vec.size();
        if (seqSize != vecSize) {
            fprintf(
                stderr, "Size mismatch: py size = %d, c++ size = %d (%s:%d)\n",
                seqSize, vecSize, file, line);
            _Exit(1);
        }
        for (int i=0; i<vecSize; ++i) {
            T val = extract<T>(seq[i]);
            if (val != vec[i]) {
                fprintf(
                    stderr, "Items not equal @ pos %d (%s:%d)",
                    i, file, line);
                _Exit(1);
            }
        }
    }
    catch (boost::python::error_already_set &) {
        fprintf(stderr, "Unexpected Python exception (%s:%d)\n", file, line);
        PyErr_Print();
        _Exit(1);
    }
}

#define ASSERT_PY_SEQ_VEC_EQUAL(seq, vec) \
    AssertPySeqVecEqual(seq, vec, __FILE__, __LINE__)

// Check that a Python set contains the same items as a C++ vector.  Items may
// appear in any order.
template <typename T>
static void
AssertPySetVecEqual(
    boost::python::object set,
    const std::vector<T> &vec,
    const char *file,
    const int line)
{
    try {
        handle<> iter(PyObject_GetIter(set.ptr()));
        TF_AXIOM(iter);

        std::vector<T> newVec;

        PyObject *item = nullptr;
        while ((item = PyIter_Next(iter.get()))) {
            handle<> itemHandle(item);
            newVec.push_back(extract<T>(itemHandle.get()));
        }

        if (vec.size() != newVec.size()) {
            fprintf(
                stderr, "Size mismatch: py size = %zu, c++ size = %zu (%s:%d)",
                newVec.size(), vec.size(), file, line);
            _Exit(1);
        }

        // Check that the set and vector contain the same elements (but not in
        // any particular order.)
        if (!std::is_permutation(vec.begin(), vec.end(), newVec.begin())) {
            fprintf(stderr, "Contents not equal (%s:%d)\n", file, line);
            _Exit(1);
        }
    }
    catch (boost::python::error_already_set &) {
        fprintf(stderr, "Unexpected Python exception (%s:%d)\n", file, line);
        PyErr_Print();
        _Exit(1);
    }
}

#define ASSERT_PY_SET_VEC_EQUAL(seq, vec) \
    AssertPySetVecEqual(seq, vec, __FILE__, __LINE__)

int
main(int argc, char **argv)
{
    TfPyInitialize();

    TfPyLock lock;

    class_<Unhashable>("Unhashable", init<>())
        .def(self == self)
        .def(self != self)

        // Disable hash function for this type.
        .setattr("__hash__", object())
        ;

    using This = Tf_TestPyResultConversions;
    class_<This> PyResultConversions("TestPyResultConversions", init<>());
    PyResultConversions
        // TfPySequenceToList methods
        .def("GetEmptyVecAsList", &This::GetEmptyVec,
             return_value_policy<TfPySequenceToList>())
        .def("GetUniqueVecAsList", &This::GetUniqueVec,
             return_value_policy<TfPySequenceToList>())
        .def("GetDuplicateVecAsList", &This::GetDuplicateVec,
             return_value_policy<TfPySequenceToList>())

        .def("GetEmptySetAsList", &This::GetEmptySet,
             return_value_policy<TfPySequenceToList>())
        .def("GetSetAsList", &This::GetSet,
             return_value_policy<TfPySequenceToList>())
        .def("GetUnhashableVecAsList", &This::GetUnhashableVec,
             return_value_policy<TfPySequenceToList>())

        // TfPySequenceToTuple methods
        .def("GetEmptyVecAsTuple", &This::GetEmptyVec,
             return_value_policy<TfPySequenceToTuple>())
        .def("GetUniqueVecAsTuple", &This::GetUniqueVec,
             return_value_policy<TfPySequenceToTuple>())
        .def("GetDuplicateVecAsTuple", &This::GetDuplicateVec,
             return_value_policy<TfPySequenceToTuple>())

        .def("GetEmptySetAsTuple", &This::GetEmptySet,
             return_value_policy<TfPySequenceToTuple>())
        .def("GetSetAsTuple", &This::GetSet,
             return_value_policy<TfPySequenceToTuple>())
        .def("GetUnhashableVecAsTuple", &This::GetUnhashableVec,
             return_value_policy<TfPySequenceToTuple>())

        // TfPySequenceToSet methods
        .def("GetEmptyVecAsSet", &This::GetEmptyVec,
             return_value_policy<TfPySequenceToSet>())
        .def("GetUniqueVecAsSet", &This::GetUniqueVec,
             return_value_policy<TfPySequenceToSet>())
        .def("GetDuplicateVecAsSet", &This::GetDuplicateVec,
             return_value_policy<TfPySequenceToSet>())

        .def("GetEmptySetAsSet", &This::GetEmptySet,
             return_value_policy<TfPySequenceToSet>())
        .def("GetSetAsSet", &This::GetSet,
             return_value_policy<TfPySequenceToSet>())
        .def("GetUnhashableVecAsSet", &This::GetUnhashableVec,
             return_value_policy<TfPySequenceToSet>())

        // TfPyPairToTuple methods
        .def("GetPair", &This::GetPair,
             return_value_policy<TfPyPairToTuple>())
        ;

    Tf_TestPyResultConversions conv;
    boost::python::object pyConv = PyResultConversions();
    TF_AXIOM(pyConv);

    // TfPySequenceToList tests
    ASSERT_PY_SEQ_VEC_EQUAL(
        pyConv.attr("GetEmptyVecAsList")(),
        std::vector<int>());

    ASSERT_PY_SEQ_VEC_EQUAL(
        pyConv.attr("GetUniqueVecAsList")(),
        conv.GetUniqueVec());

    ASSERT_PY_SEQ_VEC_EQUAL(
        pyConv.attr("GetDuplicateVecAsList")(),
        conv.GetDuplicateVec());

    ASSERT_PY_SET_VEC_EQUAL(
        pyConv.attr("GetEmptySetAsList")(),
        std::vector<int>());

    ASSERT_PY_SET_VEC_EQUAL(
        pyConv.attr("GetSetAsList")(),
        conv.GetUniqueVec());

    ASSERT_PY_SEQ_VEC_EQUAL(
        pyConv.attr("GetUnhashableVecAsList")(),
        conv.GetUnhashableVec());

    // TfPySequenceToTuple tests
    ASSERT_PY_SEQ_VEC_EQUAL(
        pyConv.attr("GetEmptyVecAsTuple")(),
        std::vector<int>());

    ASSERT_PY_SEQ_VEC_EQUAL(
        pyConv.attr("GetUniqueVecAsTuple")(),
        conv.GetUniqueVec());

    ASSERT_PY_SEQ_VEC_EQUAL(
        pyConv.attr("GetDuplicateVecAsTuple")(),
        conv.GetDuplicateVec());

    ASSERT_PY_SET_VEC_EQUAL(
        pyConv.attr("GetEmptySetAsTuple")(),
        std::vector<int>());

    ASSERT_PY_SET_VEC_EQUAL(
        pyConv.attr("GetSetAsTuple")(),
        conv.GetUniqueVec());

    ASSERT_PY_SEQ_VEC_EQUAL(
        pyConv.attr("GetUnhashableVecAsTuple")(),
        conv.GetUnhashableVec());

    // TfPySequenceToSet tests
    ASSERT_PY_SET_VEC_EQUAL(
        pyConv.attr("GetEmptyVecAsSet")(),
        std::vector<int>());

    ASSERT_PY_SET_VEC_EQUAL(
        pyConv.attr("GetUniqueVecAsSet")(),
        conv.GetUniqueVec());

    // Note that this is not a copy-paste mistake.  The duplicate vector, in
    // set form, has the same contents as the unique vector.
    ASSERT_PY_SET_VEC_EQUAL(
        pyConv.attr("GetDuplicateVecAsSet")(),
        conv.GetUniqueVec());

    ASSERT_PY_SET_VEC_EQUAL(
        pyConv.attr("GetEmptySetAsSet")(),
        std::vector<int>());

    ASSERT_PY_SET_VEC_EQUAL(
        pyConv.attr("GetSetAsSet")(),
        conv.GetUniqueVec());

    try {
        pyConv.attr("GetUnhashableVecAsSet")();
        TF_FATAL_ERROR("Conversion of unhashable type to Python set failed "
                       "to throw the expected exception");
    }
    catch (boost::python::error_already_set &) {
        PyErr_Clear();
    }

    // TfPyPairToTuple tests
    {
        try {
            auto res = pyConv.attr("GetPair")();
            int first = extract<int>(res[0]);
            double second = extract<double>(res[1]);
            TF_AXIOM(first == conv.GetPair().first);
            TF_AXIOM(second == conv.GetPair().second);
        }
        catch (boost::python::error_already_set &) {
            fprintf(
                stderr,
                "Unexpected Python exception when extracting pair (%s:%d)\n",
                __FILE__, __LINE__);
            PyErr_Print();
            return 1;
        }
    }

    return 0;
}
