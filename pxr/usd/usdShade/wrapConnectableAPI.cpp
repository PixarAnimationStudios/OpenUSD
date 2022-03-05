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

    boost::python::class_<This, boost::python::bases<UsdAPISchemaBase> >
        cls("ConnectableAPI");

    cls
        .def(boost::python::init<UsdPrim>(boost::python::arg("prim")))
        .def(boost::python::init<UsdSchemaBase const&>(boost::python::arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Get")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             boost::python::arg("includeInherited")=true,
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!boost::python::self)


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

static boost::python::object
_GetConnectedSource(const UsdAttribute &shadingAttr)
{
    UsdShadeConnectableAPI source;
    TfToken                sourceName;
    UsdShadeAttributeType  sourceType;
    
    if (UsdShadeConnectableAPI::GetConnectedSource(shadingAttr, 
            &source, &sourceName, &sourceType)){
        return boost::python::make_tuple(source, sourceName, sourceType);
    } else {
        return boost::python::object();
    }
}

static boost::python::object
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
        .def("RequiresEncapsulation", 
                &UsdShadeConnectableAPI::RequiresEncapsulation)

        .def("CanConnect", CanConnect_Input,
            (boost::python::arg("input"), boost::python::arg("source")))
        .def("CanConnect", CanConnect_Output,
            (boost::python::arg("output"), boost::python::arg("source")=UsdAttribute()))
        .staticmethod("CanConnect")

        .def("ConnectToSource", ConnectToSource_1, 
            (boost::python::arg("shadingAttr"), 
             boost::python::arg("source"), boost::python::arg("sourceName"), 
             boost::python::arg("sourceType")=UsdShadeAttributeType::Output,
             boost::python::arg("typeName")=SdfValueTypeName()))
        .def("ConnectToSource", ConnectToSource_2,
            (boost::python::arg("shadingAttr"), boost::python::arg("sourcePath")))
        .def("ConnectToSource", ConnectToSource_3,
            (boost::python::arg("shadingAttr"), boost::python::arg("input")))
        .def("ConnectToSource", ConnectToSource_4,
            (boost::python::arg("shadingAttr"), boost::python::arg("output")))
        .def("ConnectToSource", ConnectToSource_5,
            (boost::python::arg("shadingAttr"),
             boost::python::arg("source"),
             boost::python::arg("mod")=UsdShadeConnectionModification::Replace))
        .staticmethod("ConnectToSource")

        .def("SetConnectedSources",
                &UsdShadeConnectableAPI::SetConnectedSources)
        .staticmethod("SetConnectedSources")

        .def("GetConnectedSource", _GetConnectedSource,
            (boost::python::arg("shadingAttr")))
            .staticmethod("GetConnectedSource")

        .def("GetConnectedSources", _GetConnectedSources,
            (boost::python::arg("shadingAttr")))
            .staticmethod("GetConnectedSources")

        .def("GetRawConnectedSourcePaths", _GetRawConnectedSourcePaths, 
            (boost::python::arg("shadingAttr")),
            boost::python::return_value_policy<TfPySequenceToList>())
            .staticmethod("GetRawConnectedSourcePaths")

        .def("HasConnectedSource", HasConnectedSource,
            (boost::python::arg("shadingAttr")))
            .staticmethod("HasConnectedSource")

        .def("IsSourceConnectionFromBaseMaterial",
                IsSourceConnectionFromBaseMaterial,
            (boost::python::arg("shadingAttr")))
            .staticmethod("IsSourceConnectionFromBaseMaterial")

        .def("DisconnectSource", DisconnectSource,
            (boost::python::arg("shadingAttr"), boost::python::arg("sourceAttr")=UsdAttribute()))
            .staticmethod("DisconnectSource")

        .def("ClearSources", ClearSources,
            (boost::python::arg("shadingAttr")))
            .staticmethod("ClearSources")

        .def("ClearSource", ClearSource,
            (boost::python::arg("shadingAttr")))
            .staticmethod("ClearSource")

        .def("HasConnectableAPI", HasConnectableAPI,
            (boost::python::arg("schemaType")))
            .staticmethod("HasConnectableAPI")

        .def("CreateOutput", &UsdShadeConnectableAPI::CreateOutput,
             (boost::python::arg("name"), boost::python::arg("type")))
        .def("GetOutput", &UsdShadeConnectableAPI::GetOutput, boost::python::arg("name"))
        .def("GetOutputs", &UsdShadeConnectableAPI::GetOutputs,
             (boost::python::arg("onlyAuthored") = true),
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("CreateInput", &UsdShadeConnectableAPI::CreateInput,
             (boost::python::arg("name"), boost::python::arg("type")))
        .def("GetInput", &UsdShadeConnectableAPI::GetInput, boost::python::arg("name"))
        .def("GetInputs", &UsdShadeConnectableAPI::GetInputs,
             (boost::python::arg("onlyAuthored") = true),
             boost::python::return_value_policy<TfPySequenceToList>())

    ;

    boost::python::class_<ConnectionSourceInfo>("ConnectionSourceInfo")
        .def(boost::python::init<UsdShadeConnectableAPI const &,
                  TfToken const &,
                  UsdShadeAttributeType,
                  SdfValueTypeName>((boost::python::arg("source"),
                                     boost::python::arg("sourceName"),
                                     boost::python::arg("sourceType"),
                                     boost::python::arg("typeName")=SdfValueTypeName())))
        .def(boost::python::init<UsdShadeInput const &>(boost::python::arg("input")))
        .def(boost::python::init<UsdShadeOutput const &>(boost::python::arg("output")))
        .def(boost::python::init<UsdStagePtr const&, SdfPath const&>())

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

    boost::python::to_python_converter<
        std::vector<ConnectionSourceInfo>,
        TfPySequenceToPython<std::vector<ConnectionSourceInfo>>>();
    TfPyContainerConversions::from_python_sequence<
        std::vector<ConnectionSourceInfo>,
        TfPyContainerConversions::variable_capacity_policy>();

}

} // anonymous namespace
