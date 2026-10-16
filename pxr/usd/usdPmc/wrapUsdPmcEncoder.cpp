//
// Copyright 2026 Apple
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "usdPmcEncoder.h"

#include "pxr/pxr.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

static bool
_EncodeStage(UsdPmcMeshEncoder& self,
             const std::string& inFile,
             const std::string& outFile,
             const VtDictionary& options)
{
    return self.EncodeStage(inFile, outFile, options);
}

} // anonymous namespace

void wrapUsdPmcMeshEncoder(nb::module_ &m)
{
    nb::class_<UsdPmcMeshEncoder>(m, "UsdPmcMeshEncoder")
        .def(nb::init<>())
        .def("EncodeStage", &_EncodeStage,
             nb::arg("inFile"), nb::arg("outFile"),
             nb::arg("options") = VtDictionary())
        ;
}
