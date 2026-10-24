//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdr/shaderNodeDiscoveryResult.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {


static
std::string
_Repr(const SdrShaderNodeDiscoveryResult& x)
{
    std::vector<std::string> args = {
        TfPyRepr(x.identifier),
        TfPyRepr(x.version),
        TfPyRepr(x.name),
        TfPyRepr(x.function),
        TfPyRepr(x.discoveryType),
        TfPyRepr(x.shadingSystem),
        TfPyRepr(x.uri),
        TfPyRepr(x.resolvedUri)
    };

    #define ADD_KW_ARG(kwArgs, propName) \
        kwArgs.push_back(TfStringPrintf(#propName "=%s", \
                                        TfPyRepr(x.propName).c_str()));

    if (!x.sourceCode.empty()) { ADD_KW_ARG(args, sourceCode); };
    if (!x.metadata.empty()) { ADD_KW_ARG(args, metadata); };
    if (!x.blindData.empty()) { ADD_KW_ARG(args, blindData); };
    if (!x.subIdentifier.IsEmpty()) { ADD_KW_ARG(args, subIdentifier); };

    #undef ADD_KW_ARG

    return TF_PY_REPR_PREFIX +
        TfStringPrintf("NodeDiscoveryResult(%s)", 
                       TfStringJoin(args, ", ").c_str());
}

}

void wrapShaderNodeDiscoveryResult(nanobind::module_ &m)
{
    typedef SdrShaderNodeDiscoveryResult This;
    nb::class_<This>(m, "NodeDiscoveryResult")
        .def(nb::init<SdrIdentifier, SdrVersion, std::string, TfToken, TfToken, 
                      TfToken, std::string, std::string, std::string,
                      SdrTokenMap, std::string, TfToken>(),
             nb::arg("identifier"),
             nb::arg("version"),
             nb::arg("name"),
             nb::arg("function"),
             nb::arg("discoveryType"),
             nb::arg("shadingSystem"),
             nb::arg("uri"),
             nb::arg("resolvedUri"),
             nb::arg("sourceCode") = std::string(), 
             nb::arg("metadata") = SdrTokenMap(),
             nb::arg("blindData") = std::string(),
             nb::arg("subIdentifier") = TfToken())
        .def_ro("identifier", &This::identifier)
        .def_ro("version", &This::version)
        .def_ro("name", &This::name)
        .def_ro("family", &This::function)
        .def_ro("function", &This::function)
        .def_ro("discoveryType", &This::discoveryType)
        .def_ro("sourceType", &This::shadingSystem)
        .def_ro("shadingSystem", &This::shadingSystem)
        .def_ro("uri", &This::uri)
        .def_ro("resolvedUri", &This::resolvedUri)
        .def_ro("sourceCode", &This::sourceCode)
        .def_ro("metadata", &This::metadata)
        .def_ro("blindData", &This::blindData)
        .def_ro("subIdentifier", &This::subIdentifier)
        .def("__repr__", _Repr)
        ;
}
