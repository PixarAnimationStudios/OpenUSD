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

#include <boost/python/iterator.hpp>
#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <string>

using namespace boost::python;

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

static object
_GetNextNonContainingInterval(
    GfMultiInterval const &self,
    double x) 
{
    const GfMultiInterval::const_iterator it = 
        self.GetNextNonContainingInterval(x);
    return it == self.end() ? /* None */ object() : object(*it);
}

static object
_GetPriorNonContainingInterval(
    GfMultiInterval const &self,
    double x) 
{
    const GfMultiInterval::const_iterator it = 
        self.GetPriorNonContainingInterval(x);
    return it == self.end() ? /* None */ object() : object(*it);
}

static object
_GetContainingInterval(
    GfMultiInterval const &self,
    double x) 
{
    const GfMultiInterval::const_iterator it = self.GetContainingInterval(x);
    return it == self.end() ? /* None */ object() : object(*it);
}

} // anonymous namespace 

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

        .def("GetNextNonContainingInterval", _GetNextNonContainingInterval)
        .def("GetPriorNonContainingInterval", _GetPriorNonContainingInterval)
        .def("GetContainingInterval", _GetContainingInterval)

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
