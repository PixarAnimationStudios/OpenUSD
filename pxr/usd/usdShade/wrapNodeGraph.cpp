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
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/external/boost/python.hpp"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

void wrapUsdShadeNodeGraph()
{
    typedef UsdShadeNodeGraph This;

    class_<This, bases<UsdTyped> >
        cls("NodeGraph");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("Define", &This::Define, (arg("stage"), arg("path")))
        .staticmethod("Define")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)


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

namespace {

static object
_WrapComputeOutputSource(const UsdShadeNodeGraph &self, 
                         const TfToken &outputName)
{
    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    UsdShadeShader source = self.ComputeOutputSource(outputName, &sourceName, 
            &sourceType);
    return pxr_boost::python::make_tuple (source, sourceName, sourceType);
}

WRAP_CUSTOM {
    _class
        .def(init<UsdShadeConnectableAPI>(arg("connectable")))
        .def("ConnectableAPI", &UsdShadeNodeGraph::ConnectableAPI)

        .def("CreateOutput", 
             &UsdShadeNodeGraph::CreateOutput,
             (arg("name"), arg("typeName")))
        .def("GetOutput",
             &UsdShadeNodeGraph::GetOutput,
             (arg("name")))
        .def("GetOutputs",
             &UsdShadeNodeGraph::GetOutputs,
             (arg("onlyAuthored") = true),
             return_value_policy<TfPySequenceToList>())
        .def("ComputeOutputSource", _WrapComputeOutputSource, 
             (arg("outputName")))

        .def("CreateInput", &UsdShadeNodeGraph::CreateInput,
             (arg("name"), arg("type")))
        .def("GetInput", &UsdShadeNodeGraph::GetInput, arg("name"))
        .def("GetInputs", &UsdShadeNodeGraph::GetInputs,
             (arg("onlyAuthored") = true),
             return_value_policy<TfPySequenceToList>())
        .def("GetInterfaceInputs", &UsdShadeNodeGraph::GetInterfaceInputs,
             return_value_policy<TfPySequenceToList>())

        .def("ComputeInterfaceInputConsumersMap",
             &UsdShadeNodeGraph::ComputeInterfaceInputConsumersMap,
             return_value_policy<TfPyMapToDictionary>(),
             (arg("computeTransitiveConsumers")=false))

    ;
}

} // anonymous namespace
