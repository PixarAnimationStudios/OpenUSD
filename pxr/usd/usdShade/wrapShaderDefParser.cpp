//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderNodeDiscoveryResult.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/usdShade/shaderDefParser.h"

#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

// Expose the unique_ptr returned from `ParseShaderNode()` as a raw ptr. The Python side
// will be responsible for managing this object.
static SdrShaderNodePtr
_ParseShaderNode(UsdShadeShaderDefParserPlugin& self,
       const SdrShaderNodeDiscoveryResult& discoveryResult)
{
    return dynamic_cast<SdrShaderNodePtr>(
            self.ParseShaderNode(discoveryResult).release());
}

/// \deprecated
/// Deprecated in favor of _ParseShaderNode
static NdrNodePtr
_Parse(UsdShadeShaderDefParserPlugin& self,
       const NdrNodeDiscoveryResult& discoveryResult)
{
    return dynamic_cast<NdrNodePtr>(
            self.Parse(discoveryResult).release());
}

// Note that this parser is only wrapped for testing purposes. In real-world
// scenarios, it should not be used directly.
void wrapUsdShadeShaderDefParser()
{
    typedef UsdShadeShaderDefParserPlugin This;

    return_value_policy<copy_const_reference> copyRefPolicy;

    class_<This, noncopyable>("ShaderDefParserPlugin")
        .def("Parse", &_Parse, return_value_policy<manage_new_object>())
        .def("ParseShaderNode", &_ParseShaderNode,
             return_value_policy<manage_new_object>())
        .def("GetDiscoveryTypes", &This::GetDiscoveryTypes, copyRefPolicy)
        .def("GetSourceType", &This::GetSourceType, copyRefPolicy)
        ;
}
