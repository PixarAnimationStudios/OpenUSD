//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/multiInterval.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static string
_Repr(GfMultiInterval const &self)
{
    string r = TF_PY_REPR_PREFIX + "MultiInterval(";
    if (!self.IsEmpty()) {
        r += "[";
        int count = 0;
        TF_FOR_ALL(i, self) {
            if (count)
                r += ", ";
            r += TfPyRepr(*i);
            count++;
        }
        r += "]";
    }
    r += ")";
    return r;
}

} // anonymous namespace 

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/make_iterator.h>
#include <nanobind/operators.h>
#include <nanobind/stl/vector.h>

void wrapMultiInterval(nanobind::module_ &m)
{    
    typedef GfMultiInterval This;

    nanobind::class_<This>(m, "MultiInterval")
        .def(nanobind::init<>())
        .def(nanobind::init<const GfInterval &>())
        .def(nanobind::init<const GfMultiInterval &>())
        .def(nanobind::init<const std::vector<GfInterval> &>())
        .def(TfTypePythonClass())

        .def_prop_ro("size", &This::GetSize)
        .def_prop_ro("isEmpty", &This::IsEmpty)
        .def_prop_ro("bounds", &This::GetBounds)

        .def("Contains",
            (bool (This::*)(const GfInterval &) const) &This::Contains,
            "Returns true if x is inside the multi-interval.")
        .def("Contains",
            (bool (This::*)(const GfMultiInterval &) const) &This::Contains,
            "Returns true if x is inside the multi-interval.")
        .def("Contains",
            (bool (This::*)(double) const) &This::Contains,
            "Returns true if x is inside the multi-interval.")

        .def("Clear", &This::Clear)
        .def("GetComplement", &This::GetComplement)

        .def("Add",
            (void (This::*)(const GfInterval &)) &This::Add)
        .def("Add",
            (void (This::*)(const GfMultiInterval &)) &This::Add)

        .def("ArithmeticAdd",
            (void (This::*)(const GfInterval &)) &This::ArithmeticAdd)

        .def("Remove",
            (void (This::*)(const GfInterval &)) &This::Remove)
        .def("Remove",
            (void (This::*)(const GfMultiInterval &)) &This::Remove)

        .def("Intersect",
            (void (This::*)(const GfInterval &)) &This::Intersect)
        .def("Intersect",
            (void (This::*)(const GfMultiInterval &)) &This::Intersect)

        .def("IsEmpty", &This::IsEmpty)
        .def("GetSize", &This::GetSize)
        .def("GetBounds", &This::GetBounds)

        .def("GetNextNonContainingInterval", [](const This &self, double x) -> nanobind::object {
            const auto it = self.GetNextNonContainingInterval(x);
            return it == self.end() ? nanobind::none() : nanobind::cast(*it);
        })
        .def("GetPriorNonContainingInterval", [](const This &self, double x) -> nanobind::object {
            const auto it = self.GetPriorNonContainingInterval(x);
            return it == self.end() ? nanobind::none() : nanobind::cast(*it);
        })
        .def("GetContainingInterval", [](const This &self, double x) -> nanobind::object {
            const auto it = self.GetContainingInterval(x);
            return it == self.end() ? nanobind::none() : nanobind::cast(*it);
        })

        .def_static("GetFullInterval", &This::GetFullInterval)

        // totally_ordered
        .def(nanobind::self == nanobind::self)
        .def(nanobind::self != nanobind::self)
        .def(nanobind::self < nanobind::self)
        .def(nanobind::self <= nanobind::self)
        .def(nanobind::self > nanobind::self)
        .def(nanobind::self >= nanobind::self)

        .def("__str__", [](const This &self) { return TfStringify(self); })
        .def("__repr__", _Repr)
        .def("__hash__", &This::Hash)
        .def("__iter__", [](const This &self) {
            return nanobind::make_iterator(
                nanobind::type<This>(), "Iterator", self.begin(), self.end());
        }, nanobind::keep_alive<0, 1>())
        ;
}
