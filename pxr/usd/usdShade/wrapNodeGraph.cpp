//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usd/usdShade/nodeGraph.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>


PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdUsdShadeWrapNodeGraph {

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

    boost::python::class_<This, boost::python::bases<UsdTyped> >
        cls("NodeGraph");

    cls
        .def(boost::python::init<UsdPrim>(boost::python::arg("prim")))
        .def(boost::python::init<UsdSchemaBase const&>(boost::python::arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Get")

        .def("Define", &This::Define, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Define")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             boost::python::arg("includeInherited")=true,
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!boost::python::self)


        .def("__repr__", pxrUsdUsdShadeWrapNodeGraph::_Repr)
    ;

    pxrUsdUsdShadeWrapNodeGraph::_CustomWrapCode(cls);
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

namespace pxrUsdUsdShadeWrapNodeGraph {

static boost::python::object
_WrapComputeOutputSource(const UsdShadeNodeGraph &self, 
                         const TfToken &outputName)
{
    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    UsdShadeShader source = self.ComputeOutputSource(outputName, &sourceName, 
            &sourceType);
    return boost::python::make_tuple (source, sourceName, sourceType);
}

WRAP_CUSTOM {
    _class
        .def(boost::python::init<UsdShadeConnectableAPI>(boost::python::arg("connectable")))
        .def("ConnectableAPI", &UsdShadeNodeGraph::ConnectableAPI)

        .def("CreateOutput", 
             &UsdShadeNodeGraph::CreateOutput,
             (boost::python::arg("name"), boost::python::arg("typeName")))
        .def("GetOutput",
             &UsdShadeNodeGraph::GetOutput,
             (boost::python::arg("name")))
        .def("GetOutputs",
             &UsdShadeNodeGraph::GetOutputs,
             (boost::python::arg("onlyAuthored") = true),
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("ComputeOutputSource", _WrapComputeOutputSource, 
             (boost::python::arg("outputName")))

        .def("CreateInput", &UsdShadeNodeGraph::CreateInput,
             (boost::python::arg("name"), boost::python::arg("type")))
        .def("GetInput", &UsdShadeNodeGraph::GetInput, boost::python::arg("name"))
        .def("GetInputs", &UsdShadeNodeGraph::GetInputs,
             (boost::python::arg("onlyAuthored") = true),
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetInterfaceInputs", &UsdShadeNodeGraph::GetInterfaceInputs,
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("ComputeInterfaceInputConsumersMap",
             &UsdShadeNodeGraph::ComputeInterfaceInputConsumersMap,
             boost::python::return_value_policy<TfPyMapToDictionary>(),
             (boost::python::arg("computeTransitiveConsumers")=false))

    ;
}

} // anonymous namespace
