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

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/init.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

void wrapInterval()
{    
    typedef GfInterval This;

    class_<This>( "Interval", "Basic mathematical interval class", init<>() )
        .def(init<double>(
                 "Create a closed interval representing the single point [val,val]."))
        .def(init<double, double>(
                 "Create a closed interval representing the range [v1,v2]."))
        .def(init<double, double, bool, bool>("Create the interval."))
        .def(init<This>())

        .def( TfTypePythonClass() )

        .add_property("min", &This::GetMin, "The minimum value." )
        .add_property("max", &This::GetMax, "The maximum value." )
        .add_property("minClosed", &This::IsMinClosed)
        .add_property("maxClosed", &This::IsMaxClosed)
        .add_property("minOpen", &This::IsMinOpen)
        .add_property("maxOpen", &This::IsMaxOpen)
        .add_property("minFinite", &This::IsMinFinite)
        .add_property("maxFinite", &This::IsMaxFinite)
        .add_property("finite", &This::IsFinite)

        .add_property("isEmpty", &This::IsEmpty,
             "True if the interval is empty." )

        .add_property("size", &This::GetSize,
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

        .def("GetFullInterval", &This::GetFullInterval)
        .staticmethod("GetFullInterval")

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
        .def(self + self)
        .def(self += self)
        .def(self * self)
        .def(self *= self)
        .def(self - self)
        .def(self -= self)
        .def( -self )

        // andable
        .def(self &= self)
        .def(self & self)

        // orable
        .def(self |= self)
        .def(self | self)

        // totally_ordered
        .def(self == self)
        .def(self != self)
        .def(self < self)
        .def(self <= self)
        .def(self > self)
        .def(self >= self)

        .def(str(self))
        .def("__repr__", _Repr)
        .def("__hash__", &This::Hash)
        ;

    TfPyContainerConversions::from_python_sequence< std::vector<GfInterval>,
        TfPyContainerConversions::variable_capacity_policy >();
}
