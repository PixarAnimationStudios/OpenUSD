//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdShade/nodeGraph.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/wrapTypeHelpers.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/tuple.h>

namespace nb = nanobind;

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

static std::string
_Repr(const UsdShadeNodeGraph &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdShade.NodeGraph(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdShadeNodeGraph(nb::module_ &m)
{
    typedef UsdShadeNodeGraph This;

    nb::class_<This, UsdTyped> cls(m, "NodeGraph");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("Define", &This::Define, nb::arg("stage"), nb::arg("path"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        .def("__repr__", ::_Repr)
    ;

    _CustomWrapCode(cls);
}

// ===================================================================== //
// Feel free to add custom code below this line, it will be preserved by 
// the code generator.  The entry point for your custom code should look
// minimally like the following:
//
// WRAP_CUSTOM {
//     _class
//         .def("MyCustomMethod", ...)
//     ;
// }
//
// Of course any other ancillary or support code may be provided.
// 
// Just remember to wrap code in the appropriate delimiters:
// 'namespace {', '}'.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/usd/usdShade/connectableAPI.h"
#include <nanobind/stl/unordered_map.h>

namespace {

static nb::object
_WrapComputeOutputSource(const UsdShadeNodeGraph &self, 
                         const TfToken &outputName)
{
    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    UsdShadeShader source = self.ComputeOutputSource(outputName, &sourceName, 
            &sourceType);
    return nb::make_tuple(source, sourceName, sourceType);
}

WRAP_CUSTOM {
    _class
        .def(nb::init<UsdShadeConnectableAPI>(), nb::arg("connectable"))
        .def("ConnectableAPI", &UsdShadeNodeGraph::ConnectableAPI)

        .def("CreateOutput", 
             &UsdShadeNodeGraph::CreateOutput,
             nb::arg("name"), nb::arg("typeName"))
        .def("GetOutput",
             &UsdShadeNodeGraph::GetOutput,
             nb::arg("name"))
        .def("GetOutputs",
             &UsdShadeNodeGraph::GetOutputs,
             (nb::arg("onlyAuthored") = true),
             nb::rv_policy::copy)
        .def("ComputeOutputSource", _WrapComputeOutputSource, 
             nb::arg("outputName"))

        .def("CreateInput", &UsdShadeNodeGraph::CreateInput,
             nb::arg("name"), nb::arg("type"))
        .def("GetInput", &UsdShadeNodeGraph::GetInput, nb::arg("name"))
        .def("GetInputs", &UsdShadeNodeGraph::GetInputs,
             (nb::arg("onlyAuthored") = true),
             nb::rv_policy::copy)
        .def("GetInterfaceInputs", &UsdShadeNodeGraph::GetInterfaceInputs,
             nb::rv_policy::copy)

        .def("ComputeInterfaceInputConsumersMap",
             &UsdShadeNodeGraph::ComputeInterfaceInputConsumersMap,
             (nb::arg("computeTransitiveConsumers")=false))

    ;
}

} // anonymous namespace
