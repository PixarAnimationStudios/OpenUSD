//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderNodeDiscoveryResult.h"
#include "pxr/usd/usdShade/shaderDefParser.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

// Expose the unique_ptr returned from `ParseShaderNode()` as a raw ptr. The Python side
// will be responsible for managing this object.
static SdrShaderNodePtr
_ParseShaderNode(UsdShadeShaderDefParserPlugin& self,
       const SdrShaderNodeDiscoveryResult& discoveryResult)
{
    return dynamic_cast<SdrShaderNodePtr>(
            self.ParseShaderNode(discoveryResult).release());
}

// Note that this parser is only wrapped for testing purposes. In real-world
// scenarios, it should not be used directly.
void wrapUsdShadeShaderDefParser(nanobind::module_ &m)
{
    typedef UsdShadeShaderDefParserPlugin This;

    nb::class_<This>(m, "ShaderDefParserPlugin")
        .def(nb::init<>())
        .def("ParseShaderNode", &_ParseShaderNode,
             nb::rv_policy::take_ownership)
        .def("GetDiscoveryTypes", &This::GetDiscoveryTypes, nb::rv_policy::copy)
        .def("GetShadingSystem", &This::GetShadingSystem, nb::rv_policy::copy)
        ;

}
