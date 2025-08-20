//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdr/shaderMetadataHelpers.h"
#include "pxr/usd/usd/pyConversions.h"

#include "pxr/external/boost/python.hpp"
PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

// Python has no equivalent notion of namespaces in C++, so
// use an empty class as a namespace standin for Python.
class _MetadataHelpers{};

static tuple
_ParseSdfValue(const std::string& valueStr,
               const SdrShaderPropertyConstPtr& property) {
    std::string err;
    VtValue value = ShaderMetadataHelpers::ParseSdfValue(valueStr, property,
                                                         &err);
    return pxr_boost::python::make_tuple(value, err);
}

} // anonymous namespace

void wrapShaderMetadataHelpers()
{
    class_<_MetadataHelpers>("MetadataHelpers")
        .def("ParseSdfValue", _ParseSdfValue);
}