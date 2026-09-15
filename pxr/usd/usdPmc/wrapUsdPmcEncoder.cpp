//
// Copyright 2025 Apple
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "usdPmcEncoder.hpp"

#include "pxr/pxr.h"
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/init.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

void wrapUsdPmcMeshEncoder()
{
    class_<UsdPmcMeshEncoder>("UsdPmcMeshEncoder", init<>())
        .def("EncodeStage", &_EncodeStage,
             (arg("inFile"), arg("outFile"), arg("options") = VtDictionary()))
        ;
}
