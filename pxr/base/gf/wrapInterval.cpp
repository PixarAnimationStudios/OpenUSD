//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/interval.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static string
_Repr(GfInterval const &self)
{
    string r = TF_PY_REPR_PREFIX + "Interval(";
    if (!self.IsEmpty()) {
        r += TfPyRepr(self.GetMin()) + ", " + TfPyRepr(self.GetMax());
        if (!self.IsMinClosed() || !self.IsMaxClosed()) {
            r += ", " + TfPyRepr(self.IsMinClosed())
               + ", " + TfPyRepr(self.IsMaxClosed());
        }
    }
    r += ")";
    return r;
}

} // anonymous namespace 

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>

void wrapInterval(nanobind::module_ &m)
{    
    typedef GfInterval This;

    nanobind::class_<This>(m, "Interval", "Basic mathematical interval class")
        .def(nanobind::init<>())
        .def(nanobind::init<double>(),
             "Create a closed interval representing the single point [val,val].")
        .def(nanobind::init<double, double>(),
             "Create a closed interval representing the range [v1,v2].")
        .def(nanobind::init<double, double, bool, bool>(),
             "Create the interval.")
        .def(nanobind::init<This>())

        .def(TfTypePythonClass())

        .def_prop_ro("min", &This::GetMin, "The minimum value.")
        .def_prop_ro("max", &This::GetMax, "The maximum value.")
        .def_prop_ro("minClosed", &This::IsMinClosed)
        .def_prop_ro("maxClosed", &This::IsMaxClosed)
        .def_prop_ro("minOpen", &This::IsMinOpen)
        .def_prop_ro("maxOpen", &This::IsMaxOpen)
        .def_prop_ro("minFinite", &This::IsMinFinite)
        .def_prop_ro("maxFinite", &This::IsMaxFinite)
        .def_prop_ro("finite", &This::IsFinite)
        .def_prop_ro("isEmpty", &This::IsEmpty,
             "True if the interval is empty.")
        .def_prop_ro("size", &This::GetSize,
            "The width of the interval.")

        .def("Contains",
            (bool (This::*)(const GfInterval &) const) &This::Contains,
            "Returns true if x is inside the interval.")
        .def("Contains",
            (bool (This::*)(double) const) &This::Contains,
            "Returns true if x is inside the interval.")

        // For 2x compatibility
        .def("In",
            (bool (This::*)(double) const) &This::Contains,
            "Returns true if x is inside the interval.")

        .def_static("GetFullInterval", &This::GetFullInterval)

        .def("Intersects", &This::Intersects)

        .def("IsEmpty", &This::IsEmpty,
             "True if the interval is empty.")

        .def("IsFinite", &This::IsFinite)
        .def("IsMaxFinite", &This::IsMaxFinite)
        .def("IsMinFinite", &This::IsMinFinite)

        .def("IsMaxClosed", &This::IsMaxClosed)
        .def("IsMaxOpen", &This::IsMaxOpen)

        .def("IsMinClosed", &This::IsMinClosed)
        .def("IsMinOpen", &This::IsMinOpen)

        .def("GetMax", &This::GetMax, "Get the maximum value.")
        .def("GetMin", &This::GetMin, "Get the minimum value.")

        .def("GetSize", &This::GetSize,
             "The width of the interval")

        .def("SetMax", (void (This::*)(double))&This::SetMax,
             "Set the maximum value.")
        .def("SetMax", (void (This::*)(double, bool))&This::SetMax,
             "Set the maximum value and boundary condition.")

        .def("SetMin", (void (This::*)(double))&This::SetMin,
             "Set the minimum value.")
        .def("SetMin", (void (This::*)(double, bool))&This::SetMin,
             "Set the minimum value and boundary condition.")

        // ring_operators
        .def(nanobind::self + nanobind::self)
        .def(nanobind::self += nanobind::self)
        .def(nanobind::self * nanobind::self)
        .def(nanobind::self *= nanobind::self)
        .def(nanobind::self - nanobind::self)
        .def(nanobind::self -= nanobind::self)
        .def(-nanobind::self)

        // andable
        .def(nanobind::self &= nanobind::self)
        .def(nanobind::self & nanobind::self)

        // orable
        .def(nanobind::self |= nanobind::self)
        .def(nanobind::self | nanobind::self)

        // totally_ordered
        .def(nanobind::self == nanobind::self)
        .def(nanobind::self != nanobind::self)
        .def(nanobind::self < nanobind::self)
        .def("__le__", [](const This &self, const This &rhs) { return !(rhs < self); }, nanobind::is_operator())
        .def("__gt__", [](const This &self, const This &rhs) { return rhs < self; }, nanobind::is_operator())
        .def("__ge__", [](const This &self, const This &rhs) { return !(self < rhs); }, nanobind::is_operator())

        .def("__str__", [](const This &self) { return TfStringify(self); })
        .def("__repr__", _Repr)
        .def("__hash__", &This::Hash)
        ;

    TfPyContainerConversions::from_python_sequence< std::vector<GfInterval>,
        TfPyContainerConversions::variable_capacity_policy >();
}
