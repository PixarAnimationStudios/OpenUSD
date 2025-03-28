//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/object.hpp"
#include "pxr/external/boost/python/scope.hpp"
#include "pxr/external/boost/python/tuple.hpp"

#include "pxr/usd/usdShade/udimUtils.h"


PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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
