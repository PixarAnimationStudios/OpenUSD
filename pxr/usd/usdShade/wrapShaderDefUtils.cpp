//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/tuple.hpp>

#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/usd/usdShade/shaderDefUtils.h"
#include "pxr/usd/usdShade/shader.h"

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdShadeShaderDefUtils()
{
    scope thisScope = class_<UsdShadeShaderDefUtils>("ShaderDefUtils", no_init)
        .def("GetNodeDiscoveryResults", 
             &UsdShadeShaderDefUtils::GetNodeDiscoveryResults,
             (arg("shaderDef"), arg("sourceUri")),
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetNodeDiscoveryResults")
        .def("GetShaderProperties", 
             &UsdShadeShaderDefUtils::GetShaderProperties,
             arg("shaderDef"),
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetShaderProperties")
        .def("GetPrimvarNamesMetadataString", 
             &UsdShadeShaderDefUtils::GetPrimvarNamesMetadataString,
             (arg("metadata"), arg("shaderDef")))
        .staticmethod("GetPrimvarNamesMetadataString")
    ;
}
