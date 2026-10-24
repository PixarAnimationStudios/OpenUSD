//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usdShade/udimUtils.h"

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdShadeUdimUtils(nanobind::module_ &m)
{
    typedef UsdShadeUdimUtils This;

    nb::class_<This>(m, "UdimUtils")
        .def_static("IsUdimIdentifier", 
            &UsdShadeUdimUtils::IsUdimIdentifier,
            nb::arg("identifier"))

        .def_static("ResolveUdimTilePaths", 
            &UsdShadeUdimUtils::ResolveUdimTilePaths,
            nb::arg("udimPath"), nb::arg("layer"))

        .def_static("ReplaceUdimPattern",
            &UsdShadeUdimUtils::ReplaceUdimPattern,
            nb::arg("identifierWithPattern"), nb::arg("replacement"))

        .def_static("ResolveUdimPath",
            &UsdShadeUdimUtils::ResolveUdimPath,
            nb::arg("udimPath"), nb::arg("layer"))
        ;

}
