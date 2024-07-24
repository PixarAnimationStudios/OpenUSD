//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/object.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/tuple.hpp>

#include "pxr/usd/usdShade/udimUtils.h"


using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdShadeUdimUtils()
{
    class_<UsdShadeUdimUtils>("UdimUtils", no_init)
        .def("IsUdimIdentifier", 
            UsdShadeUdimUtils::IsUdimIdentifier,
            (arg("identifier")))
        .staticmethod("IsUdimIdentifier")

        .def("ResolveUdimTilePaths", 
            UsdShadeUdimUtils::ResolveUdimTilePaths,
            (arg("udimPath"), arg("layer")))
        .staticmethod("ResolveUdimTilePaths")

        .def("ReplaceUdimPattern",
            UsdShadeUdimUtils::ReplaceUdimPattern,
            (arg("identifierWithPattern"), arg("replacement")))
        .staticmethod("ReplaceUdimPattern")

        .def("ResolveUdimPath",
            UsdShadeUdimUtils::ResolveUdimPath,
            (arg("udimPath"), arg("layer")))
        .staticmethod("ResolveUdimPath")
        ;

}
