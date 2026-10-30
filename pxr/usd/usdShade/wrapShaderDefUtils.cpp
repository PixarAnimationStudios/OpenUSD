//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/vector.h>

#include "pxr/usd/usdShade/shaderDefUtils.h"
#include "pxr/usd/usdShade/shader.h"

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdShadeShaderDefUtils(nanobind::module_ &m)
{
    typedef UsdShadeShaderDefUtils This;

    nb::class_<This>(m, "ShaderDefUtils")
        .def_static("GetDiscoveryResults", 
             &UsdShadeShaderDefUtils::GetDiscoveryResults,
             nb::arg("shaderDef"), nb::arg("sourceUri"),
             nb::rv_policy::copy)
        .def_static("GetProperties", 
             &UsdShadeShaderDefUtils::GetProperties,
             nb::arg("shaderDef"))
        .def_static("GetPrimvarNamesMetadataString", 
             &UsdShadeShaderDefUtils::GetPrimvarNamesMetadataString,
             nb::arg("metadata"), nb::arg("shaderDef"))
    ;

}
