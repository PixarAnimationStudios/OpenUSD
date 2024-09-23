//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/scope.hpp"
#include "pxr/external/boost/python/tuple.hpp"

#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/usd/usdShade/shaderDefUtils.h"
#include "pxr/usd/usdShade/shader.h"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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
