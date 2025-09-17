//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/references.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUsdReferences()
{
    class_<UsdReferences>("References", no_init)
        .def("AddReference",
             (bool (UsdReferences::*)(const SdfReference &, UsdListPosition))
             &UsdReferences::AddReference,
             (arg("ref"),
              arg("position")=UsdListPositionBackOfPrependList))
        .def("AddReference",
             (bool (UsdReferences::*)(const string &, const SdfPath &,
                                      const SdfLayerOffset &, UsdListPosition))
             &UsdReferences::AddReference,
             (arg("assetPath"), arg("primPath"),
              arg("layerOffset")=SdfLayerOffset(),
              arg("position")=UsdListPositionBackOfPrependList))
        .def("AddReference",(bool (UsdReferences::*)(const string &,
                                                     const SdfLayerOffset &,
                                                     UsdListPosition))
             &UsdReferences::AddReference,
             (arg("assetPath"),
              arg("layerOffset")=SdfLayerOffset(),
              arg("position")=UsdListPositionBackOfPrependList))
        .def("AddInternalReference",
             &UsdReferences::AddInternalReference, 
             (arg("primPath"),
              arg("layerOffset")=SdfLayerOffset(),
              arg("position")=UsdListPositionBackOfPrependList))

        .def("RemoveReference", &UsdReferences::RemoveReference, arg("ref"))
        .def("ClearReferences", &UsdReferences::ClearReferences)
        .def("SetReferences", &UsdReferences::SetReferences)
        .def("GetPrim", (UsdPrim (UsdReferences::*)()) &UsdReferences::GetPrim)
        .def(!self)
        ;
}
