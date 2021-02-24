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


static std::string
_Repr(const UsdShadeConnectableAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdShade.ConnectableAPI(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdShadeConnectableAPI()
{
    typedef UsdShadeConnectableAPI This;

    class_<This, bases<UsdAPISchemaBase> >
        cls("ConnectableAPI");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

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

#include "pxr/usd/usdShade/utils.h"

namespace {

#include <boost/python/tuple.hpp>

static object
_GetConnectedSource(const UsdAttribute &shadingAttr)
{
    UsdShadeConnectableAPI source;
    TfToken                sourceName;
    UsdShadeAttributeType  sourceType;
    
    if (UsdShadeConnectableAPI::GetConnectedSource(shadingAttr, 
            &source, &sourceName, &sourceType)){
        return boost::python::make_tuple(source, sourceName, sourceType);
    } else {
        return object();
    }
}

static object
_GetConnectedSources(const UsdAttribute &shadingAttr)
{
    SdfPathVector invalidSourcePaths;
    UsdShadeSourceInfoVector sources =
        UsdShadeConnectableAPI::GetConnectedSources(shadingAttr,
                                                    &invalidSourcePaths);
    return boost::python::make_tuple(
        std::vector<UsdShadeConnectionSourceInfo>(
            sources.begin(), sources.end()),
        invalidSourcePaths);
}

static SdfPathVector
_GetRawConnectedSourcePaths(const UsdAttribute &shadingAttr) 
{
    SdfPathVector sourcePaths;
    UsdShadeConnectableAPI::GetRawConnectedSourcePaths(shadingAttr, 
            &sourcePaths);
    return sourcePaths;
}

static std::string
_GetSourceName(const UsdShadeConnectionSourceInfo& sourceInfo)
{
    return sourceInfo.sourceName.GetString();
}

static void
_SetSourceName(UsdShadeConnectionSourceInfo& sourceInfo,
               const std::string& sourceName)
{
    sourceInfo.sourceName = TfToken(sourceName);
}

static std::string
_ConnectionSourceInfoRepr(UsdShadeConnectionSourceInfo& sourceInfo) {
    std::stringstream ss;
    ss << "UsdShade.UsdShadeConnectionSourceInfo("
       << TfPyRepr(sourceInfo.source.GetPrim()) << ", "
       << sourceInfo.sourceName.GetString() << ", "
       << UsdShadeUtils::GetPrefixForAttributeType(sourceInfo.sourceType) << ", "
       << sourceInfo.typeName.GetAsToken().GetString() << ")";
    return ss.str();
}

WRAP_CUSTOM {

    using ConnectionSourceInfo = UsdShadeConnectionSourceInfo;

    bool (*ConnectToSource_1)(
        UsdAttribute const &,
        UsdShadeConnectableAPI const&,
        TfToken const &,
        UsdShadeAttributeType const,
        SdfValueTypeName) = 
            &UsdShadeConnectableAPI::ConnectToSource;

    bool (*ConnectToSource_2)(
        UsdAttribute const &,
        SdfPath const &) = &UsdShadeConnectableAPI::ConnectToSource;

    bool (*ConnectToSource_3)(
        UsdAttribute const &,
        UsdShadeInput const &) = &UsdShadeConnectableAPI::ConnectToSource;

    bool (*ConnectToSource_4)(
        UsdAttribute const &,
        UsdShadeOutput const &) = &UsdShadeConnectableAPI::ConnectToSource;

    bool (*ConnectToSource_5)(
        UsdAttribute const &,
        ConnectionSourceInfo const &,
        UsdShadeConnectionModification const) = 
            &UsdShadeConnectableAPI::ConnectToSource;

    bool (*CanConnect_Input)(
        UsdShadeInput const &,
        UsdAttribute const &) = &UsdShadeConnectableAPI::CanConnect;
       
    bool (*CanConnect_Output)(
        UsdShadeOutput const &,
        UsdAttribute const &) = &UsdShadeConnectableAPI::CanConnect;

    bool (*IsSourceConnectionFromBaseMaterial)(UsdAttribute const &) = 
        &UsdShadeConnectableAPI::IsSourceConnectionFromBaseMaterial;

    bool (*HasConnectedSource)(UsdAttribute const &) = 
        &UsdShadeConnectableAPI::HasConnectedSource;

    bool (*DisconnectSource)(UsdAttribute const &, UsdAttribute const &) = 
        &UsdShadeConnectableAPI::DisconnectSource;

    bool (*ClearSources)(UsdAttribute const &) =
        &UsdShadeConnectableAPI::ClearSources;

    bool (*ClearSource)(UsdAttribute const &) =
        &UsdShadeConnectableAPI::ClearSource;

    bool (*HasConnectableAPI)(TfType const &) =
        &UsdShadeConnectableAPI::HasConnectableAPI;

    _class
        .def("IsContainer", &UsdShadeConnectableAPI::IsContainer)

        .def("CanConnect", CanConnect_Input,
            (arg("input"), arg("source")))
        .def("CanConnect", CanConnect_Output,
            (arg("output"), arg("source")=UsdAttribute()))
        .staticmethod("CanConnect")

        .def("ConnectToSource", ConnectToSource_1, 
            (arg("shadingAttr"), 
             arg("source"), arg("sourceName"), 
             arg("sourceType")=UsdShadeAttributeType::Output,
             arg("typeName")=SdfValueTypeName()))
        .def("ConnectToSource", ConnectToSource_2,
            (arg("shadingAttr"), arg("sourcePath")))
        .def("ConnectToSource", ConnectToSource_3,
            (arg("shadingAttr"), arg("input")))
        .def("ConnectToSource", ConnectToSource_4,
            (arg("shadingAttr"), arg("output")))
        .def("ConnectToSource", ConnectToSource_5,
            (arg("shadingAttr"),
             arg("source"),
             arg("mod")=UsdShadeConnectionModification::Replace))
        .staticmethod("ConnectToSource")

        .def("SetConnectedSources",
                &UsdShadeConnectableAPI::SetConnectedSources)
        .staticmethod("SetConnectedSources")

        .def("GetConnectedSource", _GetConnectedSource,
            (arg("shadingAttr")))
            .staticmethod("GetConnectedSource")

        .def("GetConnectedSources", _GetConnectedSources,
            (arg("shadingAttr")))
            .staticmethod("GetConnectedSources")

        .def("GetRawConnectedSourcePaths", _GetRawConnectedSourcePaths, 
            (arg("shadingAttr")),
            return_value_policy<TfPySequenceToList>())
            .staticmethod("GetRawConnectedSourcePaths")

        .def("HasConnectedSource", HasConnectedSource,
            (arg("shadingAttr")))
            .staticmethod("HasConnectedSource")

        .def("IsSourceConnectionFromBaseMaterial",
                IsSourceConnectionFromBaseMaterial,
            (arg("shadingAttr")))
            .staticmethod("IsSourceConnectionFromBaseMaterial")

        .def("DisconnectSource", DisconnectSource,
            (arg("shadingAttr"), arg("sourceAttr")=UsdAttribute()))
            .staticmethod("DisconnectSource")

        .def("ClearSources", ClearSources,
            (arg("shadingAttr")))
            .staticmethod("ClearSources")

        .def("ClearSource", ClearSource,
            (arg("shadingAttr")))
            .staticmethod("ClearSource")

        .def("HasConnectableAPI", HasConnectableAPI,
            (arg("schemaType")))
            .staticmethod("HasConnectableAPI")

        .def("CreateOutput", &UsdShadeConnectableAPI::CreateOutput,
             (arg("name"), arg("type")))
        .def("GetOutput", &UsdShadeConnectableAPI::GetOutput, arg("name"))
        .def("GetOutputs", &UsdShadeConnectableAPI::GetOutputs,
             (arg("onlyAuthored") = true),
             return_value_policy<TfPySequenceToList>())

        .def("CreateInput", &UsdShadeConnectableAPI::CreateInput,
             (arg("name"), arg("type")))
        .def("GetInput", &UsdShadeConnectableAPI::GetInput, arg("name"))
        .def("GetInputs", &UsdShadeConnectableAPI::GetInputs,
             (arg("onlyAuthored") = true),
             return_value_policy<TfPySequenceToList>())

    ;

    class_<ConnectionSourceInfo>("ConnectionSourceInfo")
        .def(init<UsdShadeConnectableAPI const &,
                  TfToken const &,
                  UsdShadeAttributeType,
                  SdfValueTypeName>((arg("source"),
                                     arg("sourceName"),
                                     arg("sourceType"),
                                     arg("typeName")=SdfValueTypeName())))
        .def(init<UsdShadeInput const &>(arg("input")))
        .def(init<UsdShadeOutput const &>(arg("output")))
        .def(init<UsdStagePtr const&, SdfPath const&>())

        .def_readwrite("source", &ConnectionSourceInfo::source)
        .add_property("sourceName", &_GetSourceName, &_SetSourceName)
        .def_readwrite("sourceType", &ConnectionSourceInfo::sourceType)
        .def_readwrite("typeName", &ConnectionSourceInfo::typeName)

        .def("__repr__", _ConnectionSourceInfoRepr)
        .def("IsValid", &ConnectionSourceInfo::IsValid)
        .def("__nonzero__", &ConnectionSourceInfo::IsValid)
        .def("__eq__", &ConnectionSourceInfo::operator==)
        .def("__ne__", &ConnectionSourceInfo::operator!=)
    ;

    to_python_converter<
        std::vector<ConnectionSourceInfo>,
        TfPySequenceToPython<std::vector<ConnectionSourceInfo>>>();
    TfPyContainerConversions::from_python_sequence<
        std::vector<ConnectionSourceInfo>,
        TfPyContainerConversions::variable_capacity_policy>();

}

} // anonymous namespace
