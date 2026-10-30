//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdShade/connectableAPI.h"
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
_Repr(const UsdShadeConnectableAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdShade.ConnectableAPI(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdShadeConnectableAPI(nb::module_ &m)
{
    typedef UsdShadeConnectableAPI This;

    nb::class_<This, UsdAPISchemaBase> cls(m, "ConnectableAPI");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

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

#include "pxr/usd/usdShade/utils.h"

namespace {

static nb::object
_GetConnectedSource(const UsdAttribute &shadingAttr)
{
    UsdShadeConnectableAPI source;
    TfToken                sourceName;
    UsdShadeAttributeType  sourceType;
    
    if (UsdShadeConnectableAPI::GetConnectedSource(shadingAttr, 
            &source, &sourceName, &sourceType)){
        return nb::make_tuple(source, sourceName, sourceType);
    } else {
        return nb::none();
    }
}

static nb::object
_GetConnectedSources(const UsdAttribute &shadingAttr)
{
    SdfPathVector invalidSourcePaths;
    UsdShadeSourceInfoVector sources =
        UsdShadeConnectableAPI::GetConnectedSources(shadingAttr,
                                                    &invalidSourcePaths);
    return nb::make_tuple(
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

        .def_static("CanConnect", CanConnect_Input,
            nb::arg("input"), nb::arg("source"))
        .def_static("CanConnect", CanConnect_Output,
            nb::arg("output"), nb::arg("source")=UsdAttribute())

        .def_static("ConnectToSource", ConnectToSource_1, 
            nb::arg("shadingAttr"), 
             nb::arg("source"), nb::arg("sourceName"), 
             nb::arg("sourceType")=UsdShadeAttributeType::Output,
             nb::arg("typeName")=SdfValueTypeName())
        .def_static("ConnectToSource", ConnectToSource_2,
            nb::arg("shadingAttr"), nb::arg("sourcePath"))
        .def_static("ConnectToSource", ConnectToSource_3,
            nb::arg("shadingAttr"), nb::arg("input"))
        .def_static("ConnectToSource", ConnectToSource_4,
            nb::arg("shadingAttr"), nb::arg("output"))
        .def_static("ConnectToSource", ConnectToSource_5,
            nb::arg("shadingAttr"),
             nb::arg("source"),
             nb::arg("mod")=UsdShadeConnectionModification::Replace)

        .def_static("SetConnectedSources",
                &UsdShadeConnectableAPI::SetConnectedSources)

        .def_static("GetConnectedSource", _GetConnectedSource,
            nb::arg("shadingAttr"))

        .def_static("GetConnectedSources", _GetConnectedSources,
            nb::arg("shadingAttr"))

        .def_static("GetRawConnectedSourcePaths", _GetRawConnectedSourcePaths, 
            nb::arg("shadingAttr"),
             nb::rv_policy::copy)

        .def_static("HasConnectedSource", HasConnectedSource,
            nb::arg("shadingAttr"))

        .def_static("IsSourceConnectionFromBaseMaterial",
                IsSourceConnectionFromBaseMaterial,
            nb::arg("shadingAttr"))

        .def_static("DisconnectSource", DisconnectSource,
            nb::arg("shadingAttr"), nb::arg("sourceAttr")=UsdAttribute())

        .def_static("ClearSources", ClearSources,
            nb::arg("shadingAttr"))

        .def_static("ClearSource", ClearSource,
            nb::arg("shadingAttr"))

        .def_static("HasConnectableAPI", HasConnectableAPI,
            nb::arg("schemaType"))

        .def("CreateOutput", &UsdShadeConnectableAPI::CreateOutput,
             nb::arg("name"), nb::arg("type"))
        .def("GetOutput", &UsdShadeConnectableAPI::GetOutput, nb::arg("name"))
        .def("GetOutputs", &UsdShadeConnectableAPI::GetOutputs,
             (nb::arg("onlyAuthored") = true),
             nb::rv_policy::copy)

        .def("CreateInput", &UsdShadeConnectableAPI::CreateInput,
             nb::arg("name"), nb::arg("type"))
        .def("GetInput", &UsdShadeConnectableAPI::GetInput, nb::arg("name"))
        .def("GetInputs", &UsdShadeConnectableAPI::GetInputs,
             (nb::arg("onlyAuthored") = true),
             nb::rv_policy::copy)

    ;

    nb::module_ m = TfPyGetCurrentNanobindModule();
    nb::class_<ConnectionSourceInfo>(m, "ConnectionSourceInfo")
        .def(nb::init<>())
        .def(nb::init<UsdShadeConnectableAPI const &,
                      TfToken const &,
                      UsdShadeAttributeType,
                      SdfValueTypeName>(),
             nb::arg("source"),
             nb::arg("sourceName"),
             nb::arg("sourceType"),
             nb::arg("typeName")=SdfValueTypeName())
        .def(nb::init<UsdShadeInput const &>(), nb::arg("input"))
        .def(nb::init<UsdShadeOutput const &>(), nb::arg("output"))
        .def(nb::init<UsdStagePtr const&, SdfPath const&>())

        .def_rw("source", &ConnectionSourceInfo::source)
        .def_prop_rw("sourceName", &_GetSourceName, &_SetSourceName)
        .def_rw("sourceType", &ConnectionSourceInfo::sourceType)
        .def_rw("typeName", &ConnectionSourceInfo::typeName)

        .def("__repr__", _ConnectionSourceInfoRepr)
        .def("IsValid", &ConnectionSourceInfo::IsValid)
        .def("__bool__", &ConnectionSourceInfo::IsValid)
        .def("__hash__", [](const ConnectionSourceInfo &self) { return TfHash()(self); })
        .def("__eq__", &ConnectionSourceInfo::operator==)
        .def("__ne__", &ConnectionSourceInfo::operator!=)
    ;
}

} // anonymous namespace
