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
#include "pxr/usd/usd/treeIterator.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/object.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_arg.hpp>
#include <boost/python/to_python_converter.hpp>
#include <boost/python/converter/from_python.hpp>

using namespace boost::python;

class Usd_PyTreeIterator
{
public:

    Usd_PyTreeIterator(UsdPrim root)
        : _iter(root)
        , _curPrim(_iter ? *_iter : UsdPrim())
        , _didFirst(false)
        {}

    Usd_PyTreeIterator(UsdPrim root, Usd_PrimFlagsPredicate predicate)
        : _iter(root, predicate)
        , _curPrim(_iter ? *_iter : UsdPrim())
        , _didFirst(false)
        {}

    static Usd_PyTreeIterator
    PreAndPostVisit(UsdPrim root) {
        return Usd_PyTreeIterator(UsdTreeIterator::PreAndPostVisit(root));
    }

    static Usd_PyTreeIterator
    PreAndPostVisit(UsdPrim root, Usd_PrimFlagsPredicate predicate) {
        return Usd_PyTreeIterator(
            UsdTreeIterator::PreAndPostVisit(root, predicate));
    }

    static Usd_PyTreeIterator
    AllPrims(UsdPrim root) {
        return Usd_PyTreeIterator(UsdTreeIterator::AllPrims(root));
    }

    static Usd_PyTreeIterator
    AllPrimsPreAndPostVisit(UsdPrim root) {
        return Usd_PyTreeIterator(
            UsdTreeIterator::AllPrimsPreAndPostVisit(root));
    }

    static Usd_PyTreeIterator
    Stage(const UsdStagePtr &stage) {
        return Usd_PyTreeIterator(UsdTreeIterator::Stage(stage));
    }

    static Usd_PyTreeIterator
    Stage(const UsdStagePtr &stage, const Usd_PrimFlagsPredicate &predicate) {
        return Usd_PyTreeIterator(
            UsdTreeIterator::Stage(stage, predicate));
    }

    bool IsPostVisit() const { return _iter.IsPostVisit(); }
    void PruneChildren() { _iter.PruneChildren(); }
    bool IsValid() const { return _curPrim and _iter; }
    UsdPrim GetCurrentPrim() const { return _curPrim; }

    operator bool() const { return IsValid(); }
    bool operator==(Usd_PyTreeIterator other) const {
        return _curPrim == other._curPrim and _iter == other._iter;
    }
    bool operator!=(Usd_PyTreeIterator other) const {
        return not (*this == other);
    }

    // Intentionally does nothing -- simply bound with a return_self policy.
    void __iter__() const {};
    
    UsdPrim next() {
        // If the current prim is invalid, we can't use _iter and must raise an
        // exception.
        _RaiseIfAtEnd();
        if (not _curPrim) {
            PyErr_SetString(
                PyExc_RuntimeError,
                TfStringPrintf("Iterator points to %s",
                               _curPrim.GetDescription().c_str()).c_str());
            throw_error_already_set();
        }
        if (_didFirst) {
            ++_iter;
            _RaiseIfAtEnd();
        }
        _didFirst = true;
        _curPrim = *_iter;
        TF_VERIFY(_curPrim);
        return _curPrim;
    }

    static void RegisterConversions() {
        // to-python
        to_python_converter<UsdTreeIterator, Usd_PyTreeIterator>();
        // from-python
        converter::registry::push_back(
            &_convertible, &_construct,
            boost::python::type_id<UsdTreeIterator>());
    }

    // to-python conversion of UsdTreeIterator.
    static PyObject *convert(const UsdTreeIterator &treeIter) {
        TfPyLock lock;
        // (extra parens to avoid 'most vexing parse')
        boost::python::object obj((Usd_PyTreeIterator(treeIter)));
        PyObject *ret = obj.ptr();
        Py_INCREF(ret);
        return ret;
    }

private:
    explicit Usd_PyTreeIterator(const UsdTreeIterator &treeIter)
        : _iter(treeIter)
        , _curPrim(treeIter ? *treeIter : UsdPrim())
        , _didFirst(false)
        {}

    void _RaiseIfAtEnd() const {
        if (not _iter) {
            PyErr_SetString(PyExc_StopIteration, "TreeIterator at end");
            throw_error_already_set();
        }
    }

    static void *_convertible(PyObject *obj_ptr) {
        extract<Usd_PyTreeIterator> extractor(obj_ptr);
        return extractor.check() ? obj_ptr : NULL;
    }

    static void _construct(PyObject *obj_ptr,
                           converter::rvalue_from_python_stage1_data *data) {
        void *storage = ((converter::rvalue_from_python_storage<
                              Usd_PyTreeIterator>*)data)->storage.bytes;
        Usd_PyTreeIterator pyIter = extract<Usd_PyTreeIterator>(obj_ptr);
        new (storage) UsdTreeIterator(pyIter._iter);
        data->convertible = storage;
    }

    UsdTreeIterator _iter;
    UsdPrim _curPrim;
    bool _didFirst;
};

static UsdTreeIterator
_TestTreeIterRoundTrip(const UsdTreeIterator &treeIter) {
    return treeIter;
}

void wrapUsdTreeIterator()
{
    class_<Usd_PyTreeIterator>("TreeIterator", no_init)
        .def(init<UsdPrim>(arg("root")))
        .def(init<UsdPrim, Usd_PrimFlagsPredicate>(
                 (arg("root"), arg("predicate"))))

        .def("PreAndPostVisit",
             (Usd_PyTreeIterator (*)(UsdPrim))
             &Usd_PyTreeIterator::PreAndPostVisit, arg("root"))
        .def("PreAndPostVisit",
             (Usd_PyTreeIterator (*)(UsdPrim, Usd_PrimFlagsPredicate))
             &Usd_PyTreeIterator::PreAndPostVisit,
             (arg("root"), arg("predicate")))
        .staticmethod("PreAndPostVisit")
             
        .def("AllPrims", &Usd_PyTreeIterator::AllPrims, arg("root"))
        .staticmethod("AllPrims")

        .def("AllPrimsPreAndPostVisit",
             &Usd_PyTreeIterator::AllPrimsPreAndPostVisit, arg("root"))
        .staticmethod("AllPrimsPreAndPostVisit")

        .def("Stage",
             (Usd_PyTreeIterator (*)(const UsdStagePtr &))
             &Usd_PyTreeIterator::Stage, arg("stage"))
        .def("Stage",
             (Usd_PyTreeIterator (*)(
                 const UsdStagePtr &, const Usd_PrimFlagsPredicate &))
             &Usd_PyTreeIterator::Stage, (arg("stage"), arg("predicate")))
        .staticmethod("Stage")

        .def("IsPostVisit", &Usd_PyTreeIterator::IsPostVisit)
        .def("PruneChildren", &Usd_PyTreeIterator::PruneChildren)
        .def("IsValid", &Usd_PyTreeIterator::IsValid,
            "true if the iterator is not yet exhausted")
        .def("GetCurrentPrim", &Usd_PyTreeIterator::GetCurrentPrim,
            "Since an iterator cannot be dereferenced in python, "
            "GetCurrentPrim()\n performs the same function: yielding "
            "the currently visited prim.")

        .def(!self)
        .def(self == self)
        .def(self != self)

        .def("__iter__", &Usd_PyTreeIterator::__iter__, return_self<>())
        .def("next", &Usd_PyTreeIterator::next)

        ;

    Usd_PyTreeIterator::RegisterConversions();

    def("_TestTreeIterRoundTrip", _TestTreeIterRoundTrip);
}

