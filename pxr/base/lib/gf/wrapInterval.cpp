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
#include "pxr/base/gf/interval.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python/class.hpp>
#include <boost/python/init.hpp>
#include <boost/python/operators.hpp>
#include <string>

using namespace boost::python;

using std::string;

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

void wrapInterval()
{    
    typedef GfInterval This;

    class_<This>( "Interval", "Basic mathematical interval class", init<>() )
        .def(init<double>(
                 "Create a closed interval representing the single point [val,val]."))
        .def(init<double, double>(
                 "Create a closed interval representing the range [v1,v2]."))
        .def(init<double, double, bool, bool>("Create the interval."))

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
