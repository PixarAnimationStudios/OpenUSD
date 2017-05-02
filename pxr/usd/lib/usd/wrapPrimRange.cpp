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
#include "pxr/pxr.h"
#include "pxr/usd/usd/primRange.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/object.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_arg.hpp>
#include <boost/python/to_python_converter.hpp>
#include <boost/python/converter/from_python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

class Usd_PyPrimRangeIterator;

class Usd_PyPrimRange
{
public:

    Usd_PyPrimRange(UsdPrim root)
        : _rng(root)
        , _startPrim(!_rng.empty() ? *_rng.begin() : UsdPrim())
        {}

    Usd_PyPrimRange(UsdPrim root, Usd_PrimFlagsPredicate predicate)
        : _rng(root, predicate)
        , _startPrim(!_rng.empty() ? *_rng.begin() : UsdPrim())
        {}

    static Usd_PyPrimRange
    PreAndPostVisit(UsdPrim root) {
        return Usd_PyPrimRange(UsdPrimRange::PreAndPostVisit(root));
    }

    static Usd_PyPrimRange
    PreAndPostVisit(UsdPrim root, Usd_PrimFlagsPredicate predicate) {
        return Usd_PyPrimRange(
            UsdPrimRange::PreAndPostVisit(root, predicate));
    }

    static Usd_PyPrimRange
    AllPrims(UsdPrim root) {
        return Usd_PyPrimRange(UsdPrimRange::AllPrims(root));
    }

    static Usd_PyPrimRange
    AllPrimsPreAndPostVisit(UsdPrim root) {
        return Usd_PyPrimRange(
            UsdPrimRange::AllPrimsPreAndPostVisit(root));
    }

    static Usd_PyPrimRange
    Stage(const UsdStagePtr &stage) {
        return Usd_PyPrimRange(UsdPrimRange::Stage(stage));
    }

    static Usd_PyPrimRange
    Stage(const UsdStagePtr &stage, const Usd_PrimFlagsPredicate &predicate) {
        return Usd_PyPrimRange(
            UsdPrimRange::Stage(stage, predicate));
    }

    bool IsValid() const { return _startPrim && !_rng.empty(); }

    operator bool() const { return IsValid(); }

    bool operator==(Usd_PyPrimRange const &other) const {
        return _startPrim == other._startPrim && _rng == other._rng;
    }
    bool operator!=(Usd_PyPrimRange const &other) const {
        return !(*this == other);
    }

    Usd_PyPrimRangeIterator __iter__() const;

    static void RegisterConversions() {
        // to-python
        to_python_converter<UsdPrimRange, Usd_PyPrimRange>();
        // from-python
        converter::registry::push_back(
            &_convertible, &_construct,
            boost::python::type_id<UsdPrimRange>());
    }

    // to-python conversion of UsdPrimRange.
    static PyObject *convert(const UsdPrimRange &primRange) {
        TfPyLock lock;
        // (extra parens to avoid 'most vexing parse')
        boost::python::object obj((Usd_PyPrimRange(primRange)));
        PyObject *ret = obj.ptr();
        Py_INCREF(ret);
        return ret;
    }

private:
    friend class Usd_PyPrimRangeIterator;
    
    explicit Usd_PyPrimRange(const UsdPrimRange &range)
        : _rng(range)
        , _startPrim(!_rng.empty() ? *_rng.begin() : UsdPrim())
        {}

    static void *_convertible(PyObject *obj_ptr) {
        extract<Usd_PyPrimRange> extractor(obj_ptr);
        return extractor.check() ? obj_ptr : nullptr;
    }

    static void _construct(PyObject *obj_ptr,
                           converter::rvalue_from_python_stage1_data *data) {
        void *storage = ((converter::rvalue_from_python_storage<
                          Usd_PyPrimRange>*)data)->storage.bytes;
        Usd_PyPrimRange pyIter = extract<Usd_PyPrimRange>(obj_ptr);
        new (storage) UsdPrimRange(pyIter._rng);
        data->convertible = storage;
    }

    UsdPrimRange _rng;
    UsdPrim _startPrim;
};

class Usd_PyPrimRangeIterator {
public:
    explicit Usd_PyPrimRangeIterator(Usd_PyPrimRange const *range)
        : range(range)
        , iter(range->_rng.begin())
        , curPrim(iter != range->_rng.end() ? *iter : UsdPrim()) {}
    
    bool IsPostVisit() const { return iter.IsPostVisit(); }
    void PruneChildren() { iter.PruneChildren(); }
    bool IsValid() const { return curPrim && iter != range->_rng.end(); }
    UsdPrim GetCurrentPrim() const { return curPrim; }
    
    UsdPrim next() {
        // If the current prim is invalid, we can't use iter and must raise an
        // exception.
        _RaiseIfAtEnd();
        if (!curPrim) {
            PyErr_SetString(
                PyExc_RuntimeError,
                TfStringPrintf("Iterator points to %s",
                               curPrim.GetDescription().c_str()).c_str());
            throw_error_already_set();
        }
        if (didFirst) {
            ++iter;
            _RaiseIfAtEnd();
        }
        didFirst = true;
        curPrim = *iter;
        TF_VERIFY(curPrim);
        return curPrim;
    }

    void _RaiseIfAtEnd() const {
        if (iter == range->_rng.end()) {
            PyErr_SetString(PyExc_StopIteration, "PrimRange at end");
            throw_error_already_set();
        }
    }

    Usd_PyPrimRange const *range;
    UsdPrimRange::iterator iter;
    UsdPrim curPrim;
    bool didFirst = false;
};

Usd_PyPrimRangeIterator
Usd_PyPrimRange::__iter__() const
{
    if (!_rng.empty() && !_startPrim) {
        PyErr_SetString(
            PyExc_RuntimeError,
            TfStringPrintf("Invalid range starting with %s",
                           _startPrim.GetDescription().c_str()).c_str());
    }
    return Usd_PyPrimRangeIterator(this);
}

static UsdPrimRange
_TestPrimRangeRoundTrip(const UsdPrimRange &primRange) {
    return primRange;
}

} // anonymous namespace 

void wrapUsdPrimRange()
{
    {
        scope s =
            class_<Usd_PyPrimRange>("PrimRange", no_init)
            .def(init<UsdPrim>(arg("root")))
            .def(init<UsdPrim, Usd_PrimFlagsPredicate>(
                     (arg("root"), arg("predicate"))))
            
            .def("PreAndPostVisit",
                 (Usd_PyPrimRange (*)(UsdPrim))
                 &Usd_PyPrimRange::PreAndPostVisit, arg("root"))
            .def("PreAndPostVisit",
                 (Usd_PyPrimRange (*)(UsdPrim, Usd_PrimFlagsPredicate))
                 &Usd_PyPrimRange::PreAndPostVisit,
                 (arg("root"), arg("predicate")))
            .staticmethod("PreAndPostVisit")
             
            .def("AllPrims", &Usd_PyPrimRange::AllPrims, arg("root"))
            .staticmethod("AllPrims")

            .def("AllPrimsPreAndPostVisit",
                 &Usd_PyPrimRange::AllPrimsPreAndPostVisit, arg("root"))
            .staticmethod("AllPrimsPreAndPostVisit")

            .def("Stage",
                 (Usd_PyPrimRange (*)(const UsdStagePtr &))
                 &Usd_PyPrimRange::Stage, arg("stage"))
            .def("Stage",
                 (Usd_PyPrimRange (*)(
                     const UsdStagePtr &, const Usd_PrimFlagsPredicate &))
                 &Usd_PyPrimRange::Stage, (arg("stage"), arg("predicate")))
            .staticmethod("Stage")

            .def("IsValid", &Usd_PyPrimRange::IsValid,
                 "true if the iterator is not yet exhausted")

            .def(!self)
            .def(self == self)
            .def(self != self)

            // with_custodian_and_ward_postcall<0, 1> makes it so that the
            // returned iterator will prevent the source range (this) from
            // expiring until the iterator expires itself.  We need that since
            // the iterator stores a pointer to its range.
            .def("__iter__", &Usd_PyPrimRange::__iter__,
                 with_custodian_and_ward_postcall<0, 1>())
            ;

        class_<Usd_PyPrimRangeIterator>("_Iterator", no_init)
            // This is a lambda that does nothing cast to a function pointer.
            // All we want is to return 'self'.
            .def("__iter__", static_cast<void (*)(Usd_PyPrimRangeIterator)>
                     ([](Usd_PyPrimRangeIterator){}), return_self<>())
            .def("next", &Usd_PyPrimRangeIterator::next)
            .def("IsPostVisit", &Usd_PyPrimRangeIterator::IsPostVisit)
            .def("PruneChildren", &Usd_PyPrimRangeIterator::PruneChildren)
            .def("IsValid", &Usd_PyPrimRangeIterator::IsValid,
                 "true if the iterator is not yet exhausted")
            .def("GetCurrentPrim", &Usd_PyPrimRangeIterator::GetCurrentPrim,
                 "Since an iterator cannot be dereferenced in python, "
                 "GetCurrentPrim()\n performs the same function: yielding "
                 "the currently visited prim.")
            ;
    }

    Usd_PyPrimRange::RegisterConversions();

    def("_TestPrimRangeRoundTrip", _TestPrimRangeRoundTrip);
}
