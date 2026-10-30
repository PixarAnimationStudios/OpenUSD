//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyInterpreter.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>

#include <algorithm>
#include <cstdio>
#include <tuple>
#include <unordered_set>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

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
    nb::object seq,
    const std::vector<T> &vec,
    const char *file,
    const int line)
{
    try {
        const int seqSize = static_cast<int>(nb::len(seq));
        const int vecSize = static_cast<int>(vec.size());
        if (seqSize != vecSize) {
            fprintf(
                stderr, "Size mismatch: py size = %d, c++ size = %d (%s:%d)\n",
                seqSize, vecSize, file, line);
            _Exit(1);
        }
        for (int i=0; i<vecSize; ++i) {
            T val = nb::cast<T>(seq[i]);
            if (val != vec[i]) {
                fprintf(
                    stderr, "Items not equal @ pos %d (%s:%d)",
                    i, file, line);
                _Exit(1);
            }
        }
    }
    catch (nb::python_error &e) {
        e.restore();
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
    nb::object set,
    const std::vector<T> &vec,
    const char *file,
    const int line)
{
    try {
        nb::object iter = nb::steal(PyObject_GetIter(set.ptr()));
        TF_AXIOM(iter.is_valid());

        std::vector<T> newVec;

        PyObject *item = nullptr;
        while ((item = PyIter_Next(iter.ptr()))) {
            nb::object itemHandle = nb::steal(item);
            newVec.push_back(nb::cast<T>(itemHandle));
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
    catch (nb::python_error &e) {
        e.restore();
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

    nb::module_ mainMod = nb::module_::import_("__main__");

    nb::class_<Unhashable> unhashableCls(mainMod, "Unhashable");
    unhashableCls
        .def(nb::init<>())
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        ;
    nb::setattr(unhashableCls, "__hash__", nb::none());

    using This = Tf_TestPyResultConversions;
    nb::class_<This> PyResultConversions(mainMod, "TestPyResultConversions");
    PyResultConversions
        .def(nb::init<>())
        // TfPySequenceToList methods
        .def("GetEmptyVecAsList", [](const This &s) { return TfPyCopySequenceToList(s.GetEmptyVec()); })
        .def("GetUniqueVecAsList", [](const This &s) { return TfPyCopySequenceToList(s.GetUniqueVec()); })
        .def("GetDuplicateVecAsList", [](const This &s) { return TfPyCopySequenceToList(s.GetDuplicateVec()); })

        .def("GetEmptySetAsList", [](const This &s) { return TfPyCopySequenceToList(s.GetEmptySet()); })
        .def("GetSetAsList", [](const This &s) { return TfPyCopySequenceToList(s.GetSet()); })
        .def("GetUnhashableVecAsList", [](const This &s) { return TfPyCopySequenceToList(s.GetUnhashableVec()); })

        // TfPySequenceToTuple methods
        .def("GetEmptyVecAsTuple", [](const This &s) { return TfPyCopySequenceToTuple(s.GetEmptyVec()); })
        .def("GetUniqueVecAsTuple", [](const This &s) { return TfPyCopySequenceToTuple(s.GetUniqueVec()); })
        .def("GetDuplicateVecAsTuple", [](const This &s) { return TfPyCopySequenceToTuple(s.GetDuplicateVec()); })

        .def("GetEmptySetAsTuple", [](const This &s) { return TfPyCopySequenceToTuple(s.GetEmptySet()); })
        .def("GetSetAsTuple", [](const This &s) { return TfPyCopySequenceToTuple(s.GetSet()); })
        .def("GetUnhashableVecAsTuple", [](const This &s) { return TfPyCopySequenceToTuple(s.GetUnhashableVec()); })

        // TfPySequenceToSet methods
        .def("GetEmptyVecAsSet", [](const This &s) { return TfPyCopySequenceToSet(s.GetEmptyVec()); })
        .def("GetUniqueVecAsSet", [](const This &s) { return TfPyCopySequenceToSet(s.GetUniqueVec()); })
        .def("GetDuplicateVecAsSet", [](const This &s) { return TfPyCopySequenceToSet(s.GetDuplicateVec()); })

        .def("GetEmptySetAsSet", [](const This &s) { return TfPyCopySequenceToSet(s.GetEmptySet()); })
        .def("GetSetAsSet", [](const This &s) { return TfPyCopySequenceToSet(s.GetSet()); })
        .def("GetUnhashableVecAsSet", [](const This &s) { return TfPyCopySequenceToSet(s.GetUnhashableVec()); })

        // TfPyPairToTuple methods
        .def("GetPair", [](const This &s) {
            auto p = s.GetPair();
            return nb::make_tuple(p.first, p.second);
        })
        ;

    Tf_TestPyResultConversions conv;
    nb::object pyConv = PyResultConversions();
    TF_AXIOM(pyConv.is_valid());

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
    catch (nb::python_error &) {
        PyErr_Clear();
    }

    // TfPyPairToTuple tests
    {
        try {
            auto res = pyConv.attr("GetPair")();
            int first = nb::cast<int>(res[0]);
            double second = nb::cast<double>(res[1]);
            TF_AXIOM(first == conv.GetPair().first);
            TF_AXIOM(second == conv.GetPair().second);
        }
        catch (nb::python_error &e) {
            e.restore();
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
