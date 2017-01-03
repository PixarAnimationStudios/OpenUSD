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
#include "pxr/base/gf/multiInterval.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python/iterator.hpp>
#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <string>

using namespace boost::python;

using std::string;

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

void wrapMultiInterval()
{    
    typedef GfMultiInterval This;

    class_<This>( "MultiInterval", init<>() )
        .def(init<const GfInterval &>())
        .def(init<const GfMultiInterval &>())
        .def(init<const std::vector<GfInterval> &>())
        .def(TfTypePythonClass())

        .add_property("size", &This::GetSize)
        .add_property("isEmpty", &This::IsEmpty)
        .add_property("bounds", &This::GetBounds)

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

        .def("GetFullInterval", &This::GetFullInterval)
        .staticmethod("GetFullInterval")

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
        .def("__iter__", iterator<This>())
        ;
}
