//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/specializes.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUsdSpecializes()
{
    class_<UsdSpecializes>("Specializes", no_init)
        .def("AddSpecialize", &UsdSpecializes::AddSpecialize,
             (arg("primPath"),
              arg("position")=UsdListPositionBackOfPrependList))
        .def("RemoveSpecialize", &UsdSpecializes::RemoveSpecialize,
             arg("primPath"))
        .def("ClearSpecializes", &UsdSpecializes::ClearSpecializes)
        .def("SetSpecializes", &UsdSpecializes::SetSpecializes)
        .def("GetPrim", 
             (UsdPrim (UsdSpecializes::*)()) &UsdSpecializes::GetPrim)
        .def(!self)
        ;
}
