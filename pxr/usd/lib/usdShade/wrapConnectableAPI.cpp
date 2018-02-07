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
#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;


} // anonymous namespace

void wrapUsdShadeConnectableAPI()
{
    typedef UsdShadeConnectableAPI This;

    class_<This, bases<UsdSchemaBase> >
        cls("ConnectableAPI");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("Apply", &This::Apply, (arg("stage"), arg("path")))
        .staticmethod("Apply")

        .def("IsConcrete",
            static_cast<bool (*)(void)>( [](){ return This::IsConcrete; }))
        .staticmethod("IsConcrete")

        .def("IsTyped",
            static_cast<bool (*)(void)>( [](){ return This::IsTyped; } ))
        .staticmethod("IsTyped")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)


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

namespace {

#include <boost/python/tuple.hpp>

static object
_GetConnectedSource(const UsdProperty &shadingProp)
{
    UsdShadeConnectableAPI source;
    TfToken                sourceName;
    UsdShadeAttributeType  sourceType;
    
    if (UsdShadeConnectableAPI::GetConnectedSource(shadingProp, 
            &source, &sourceName, &sourceType)){
        return boost::python::make_tuple(source, sourceName, sourceType);
    } else {
        return object();
    }
}

static SdfPathVector
_GetRawConnectedSourcePaths(const UsdProperty &shadingProp) 
{
    SdfPathVector sourcePaths;
    UsdShadeConnectableAPI::GetRawConnectedSourcePaths(shadingProp, 
            &sourcePaths);
    return sourcePaths;
}

WRAP_CUSTOM {

    bool (*ConnectToSource_1)(
        UsdProperty const &,
        UsdShadeConnectableAPI const&,
        TfToken const &,
        UsdShadeAttributeType const,
        SdfValueTypeName) = 
            &UsdShadeConnectableAPI::ConnectToSource;

    bool (*ConnectToSource_2)(
        UsdProperty const &,
        SdfPath const &) = &UsdShadeConnectableAPI::ConnectToSource;

    bool (*ConnectToSource_3)(
        UsdProperty const &,
        UsdShadeInput const &) = &UsdShadeConnectableAPI::ConnectToSource;

    bool (*ConnectToSource_4)(
        UsdProperty const &,
        UsdShadeOutput const &) = &UsdShadeConnectableAPI::ConnectToSource;

    bool (*CanConnect_Input)(
        UsdShadeInput const &,
        UsdAttribute const &) = &UsdShadeConnectableAPI::CanConnect;
       
    bool (*CanConnect_Output)(
        UsdShadeOutput const &,
        UsdAttribute const &) = &UsdShadeConnectableAPI::CanConnect;

    bool (*IsSourceConnectionFromBaseMaterial)(UsdProperty const &) = 
        &UsdShadeConnectableAPI::IsSourceConnectionFromBaseMaterial;

    bool (*HasConnectedSource)(UsdProperty const &) = 
        &UsdShadeConnectableAPI::HasConnectedSource;

    bool (*DisconnectSource)(UsdProperty const &) = 
        &UsdShadeConnectableAPI::DisconnectSource;

    bool (*ClearSource)(UsdProperty const &) = 
        &UsdShadeConnectableAPI::ClearSource;

    _class
        .def(init<UsdShadeShader const &>(arg("shader")))
        .def(init<UsdShadeNodeGraph const&>(arg("nodeGraph")))

        .def("IsShader", &UsdShadeConnectableAPI::IsShader)
        .def("IsNodeGraph", &UsdShadeConnectableAPI::IsNodeGraph)

        .def("CanConnect", CanConnect_Input,
            (arg("input"), arg("source")))
        .def("CanConnect", CanConnect_Output,
            (arg("output"), arg("source")=UsdAttribute()))
        .staticmethod("CanConnect")

        .def("ConnectToSource", ConnectToSource_1, 
            (arg("shadingProp"), 
             arg("source"), arg("sourceName"), 
             arg("sourceType")=UsdShadeAttributeType::Output,
             arg("typeName")=SdfValueTypeName()))
        .def("ConnectToSource", ConnectToSource_2,
            (arg("shadingProp"), arg("sourcePath")))
        .def("ConnectToSource", ConnectToSource_3,
            (arg("shadingProp"), arg("input")))
        .def("ConnectToSource", ConnectToSource_4,
            (arg("shadingProp"), arg("output")))
        .staticmethod("ConnectToSource")

        .def("GetConnectedSource", _GetConnectedSource,
            (arg("shadingProp")))
            .staticmethod("GetConnectedSource")

        .def("GetRawConnectedSourcePaths", _GetRawConnectedSourcePaths, 
            (arg("shadingProp")),
            return_value_policy<TfPySequenceToList>())
            .staticmethod("GetRawConnectedSourcePaths")

        .def("HasConnectedSource", HasConnectedSource,
            (arg("shadingProp")))
            .staticmethod("HasConnectedSource")

        .def("IsSourceConnectionFromBaseMaterial", IsSourceConnectionFromBaseMaterial,
            (arg("shadingProp")))
            .staticmethod("IsSourceConnectionFromBaseMaterial")
        
        .def("DisconnectSource", DisconnectSource,
            (arg("shadingProp")))
            .staticmethod("DisconnectSource")
        
        .def("ClearSource", ClearSource,
            (arg("shadingProp")))
            .staticmethod("ClearSource")
        
        .def("CreateOutput", &UsdShadeConnectableAPI::CreateOutput,
             (arg("name"), arg("type")))
        .def("GetOutput", &UsdShadeConnectableAPI::GetOutput, arg("name"))
        .def("GetOutputs", &UsdShadeConnectableAPI::GetOutputs,
             return_value_policy<TfPySequenceToList>())

        .def("CreateInput", &UsdShadeConnectableAPI::CreateInput,
             (arg("name"), arg("type")))
        .def("GetInput", &UsdShadeConnectableAPI::GetInput, arg("name"))
        .def("GetInputs", &UsdShadeConnectableAPI::GetInputs,
             return_value_policy<TfPySequenceToList>())

    ;

    implicitly_convertible<UsdShadeConnectableAPI, UsdShadeNodeGraph>();
    implicitly_convertible<UsdShadeConnectableAPI, UsdShadeShader>();
}

} // anonymous namespace
