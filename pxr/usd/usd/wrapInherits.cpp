//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/inherits.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>

using std::string;

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdInherits()
{
    class_<UsdInherits>("Inherits", no_init)
        .def("AddInherit", &UsdInherits::AddInherit,
             (arg("primPath"),
              arg("position")=UsdListPositionBackOfPrependList))
        .def("RemoveInherit", &UsdInherits::RemoveInherit, arg("primPath"))
        .def("ClearInherits", &UsdInherits::ClearInherits)
        .def("SetInherits", &UsdInherits::SetInherits)
        .def("GetAllDirectInherits", &UsdInherits::GetAllDirectInherits,
             return_value_policy<TfPySequenceToList>())
        .def("GetPrim", (UsdPrim (UsdInherits::*)()) &UsdInherits::GetPrim)
        .def(!self)
        ;
}
