//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;

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
        Usd_RegisterPrimRangePythonConversions(&convert, &_fromPython);
        Tf_PyRegisterCustomToPythonConverter(
            typeid(UsdPrimRange),
            +[](void const *p) -> PyObject * {
                return convert(*static_cast<UsdPrimRange const *>(p));
            });
    }

    // to-python conversion of UsdPrimRange.
    static PyObject *convert(const UsdPrimRange &primRange) {
        TfPyLock lock;
        return nb::cast(Usd_PyPrimRange(primRange)).release().ptr();
    }

private:
    friend class Usd_PyPrimRangeIterator;
    
    explicit Usd_PyPrimRange(const UsdPrimRange &range)
        : _rng(range)
        , _startPrim(!_rng.empty() ? *_rng.begin() : UsdPrim())
        {}

    static bool _fromPython(PyObject *obj_ptr, UsdPrimRange *out) {
        Usd_PyPrimRange *ptr = nullptr;
        if (nb::try_cast<Usd_PyPrimRange *>(nb::handle(obj_ptr), ptr) && ptr) {
            if (out) {
                *out = ptr->_rng;
            }
            return true;
        }
        return false;
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
            TfPyThrowRuntimeError(
                TfStringPrintf("Iterator points to %s",
                               curPrim.GetDescription().c_str()));
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
            TfPyThrowStopIteration("PrimRange at end");
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
        TfPyThrowRuntimeError(
            TfStringPrintf("Invalid range starting with %s",
                           _startPrim.GetDescription().c_str()));
    }
    return Usd_PyPrimRangeIterator(this);
}

static UsdPrimRange
_TestPrimRangeRoundTrip(const UsdPrimRange &primRange) {
    return primRange;
}

} // anonymous namespace 

void wrapUsdPrimRange(nb::module_ &m)
{
    {
        nb::class_<Usd_PyPrimRange> s(m, "PrimRange");
        s
            .def(nb::init<UsdPrim>(), nb::arg("root"))
            .def(nb::init<UsdPrim, Usd_PrimFlagsPredicate>(),
                 nb::arg("root"), nb::arg("predicate"))
            
            .def_static("PreAndPostVisit",
                 (Usd_PyPrimRange (*)(UsdPrim))
                 &Usd_PyPrimRange::PreAndPostVisit, nb::arg("root"))
            .def_static("PreAndPostVisit",
                 (Usd_PyPrimRange (*)(UsdPrim, Usd_PrimFlagsPredicate))
                 &Usd_PyPrimRange::PreAndPostVisit,
                 nb::arg("root"), nb::arg("predicate"))
             
            .def_static("AllPrims", &Usd_PyPrimRange::AllPrims, nb::arg("root"))

            .def_static("AllPrimsPreAndPostVisit",
                 &Usd_PyPrimRange::AllPrimsPreAndPostVisit, nb::arg("root"))

            .def_static("Stage",
                 (Usd_PyPrimRange (*)(const UsdStagePtr &))
                 &Usd_PyPrimRange::Stage, nb::arg("stage"))
            .def_static("Stage",
                 (Usd_PyPrimRange (*)(
                     const UsdStagePtr &, const Usd_PrimFlagsPredicate &))
                 &Usd_PyPrimRange::Stage, nb::arg("stage"), nb::arg("predicate"))

            .def("IsValid", &Usd_PyPrimRange::IsValid,
                 "true if the iterator is not yet exhausted")

            .def(!nb::self)
            .def(nb::self == nb::self)
            .def(nb::self != nb::self)

            // keep_alive<0, 1> makes it so that the
            // returned iterator will prevent the source range (this) from
            // expiring until the iterator expires itself.  We need that since
            // the iterator stores a pointer to its range.
            .def("__iter__", &Usd_PyPrimRange::__iter__,
                 nb::keep_alive<0, 1>())
            ;

        nb::class_<Usd_PyPrimRangeIterator>(s, "_Iterator")
            .def("__init__", [](nb::args, nb::kwargs) {
                TfPyThrowRuntimeError("This class cannot be instantiated from Python");
            })
            // All we want is to return 'self'.
            .def("__iter__", [](Usd_PyPrimRangeIterator &self) -> Usd_PyPrimRangeIterator & {
                return self;
            }, nb::rv_policy::reference)
            .def("__next__", &Usd_PyPrimRangeIterator::next)
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

    m.def("_TestPrimRangeRoundTrip", _TestPrimRangeRoundTrip);
}
