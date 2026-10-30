//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/output.h"
#include "pxr/usd/usdShade/connectableAPI.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/vector.h>

#include <vector>

namespace nb = nanobind;

using std::vector;
PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static bool
_Set(const UsdShadeInput &self, nb::handle val, const UsdTimeCode &time) 
{
    return self.Set(UsdPythonToSdfType(val, self.GetTypeName()), time);
}

static TfPyObjWrapper
_Get(const UsdShadeInput &self, UsdTimeCode time) {
    VtValue val;
    self.Get(&val, time);
    return UsdVtValueToPython(val);
}

static nb::object
_GetConnectedSources(const UsdShadeInput &self)
{
    SdfPathVector invalidSourcePaths;
    UsdShadeInput::SourceInfoVector sources =
        self.GetConnectedSources(&invalidSourcePaths);
    return nb::make_tuple(
        std::vector<UsdShadeConnectionSourceInfo>(sources.begin(), sources.end()),
        invalidSourcePaths);
}

static nb::object
_GetConnectedSource(const UsdShadeInput &self)
{
    UsdShadeConnectableAPI source;
    TfToken                sourceName;
    UsdShadeAttributeType  sourceType;
    
    if (self.GetConnectedSource(&source, &sourceName, &sourceType)){
        return nb::make_tuple(source, sourceName, sourceType);
    } else {
        return nb::none();
    }
}

static SdfPathVector
_GetRawConnectedSourcePaths(const UsdShadeInput &self) 
{
    SdfPathVector sourcePaths;
    self.GetRawConnectedSourcePaths(&sourcePaths);
    return sourcePaths;
}

static nb::object
_GetValueProducingAttribute(const UsdShadeInput &self)
{
    UsdShadeAttributeType attrType;
    UsdAttribute attr = self.GetValueProducingAttribute(&attrType);
    return nb::make_tuple(attr, attrType);
}

} // anonymous namespace 

void wrapUsdShadeInput(nanobind::module_ &m)
{
    typedef UsdShadeInput Input;

    bool (Input::*ConnectToSource_1)(
        UsdShadeConnectableAPI const&,
        TfToken const &,
        UsdShadeAttributeType const,
        SdfValueTypeName) const = &Input::ConnectToSource;

    bool (Input::*ConnectToSource_2)(
        SdfPath const &) const = &Input::ConnectToSource;

    bool (Input::*ConnectToSource_3)(
        UsdShadeInput const &) const = &Input::ConnectToSource;

    bool (Input::*ConnectToSource_4)(
        UsdShadeOutput const &) const = &Input::ConnectToSource;

    bool (Input::*ConnectToSource_5)(
        UsdShadeConnectionSourceInfo const &,
        Input::ConnectionModification const mod) const = &Input::ConnectToSource;

    bool (Input::*CanConnect_1)(
        UsdAttribute const &) const = &Input::CanConnect;

    nb::class_<Input>(m, "Input")
        .def(nb::init<>())
        .def(nb::init<UsdAttribute>(), nb::arg("attr"))
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def("__hash__", [](const Input &self) { return Input::Hash()(self); })
        .def("__bool__", [](const Input &self) { return bool(self); })

        .def("GetFullName", &Input::GetFullName,
             nb::rv_policy::copy)
        .def("GetBaseName", &Input::GetBaseName)
        .def("GetPrim", &Input::GetPrim)
        .def("GetTypeName", &Input::GetTypeName)
        .def("Get", _Get, nb::arg("time")=UsdTimeCode::Default())
        .def("Set", _Set, nb::arg("value").none(), nb::arg("time")=UsdTimeCode::Default())
        .def("SetRenderType", &Input::SetRenderType,
             nb::arg("renderType"))
        .def("GetRenderType", &Input::GetRenderType)
        .def("HasRenderType", &Input::HasRenderType)

        .def("GetSdrMetadata", &Input::GetSdrMetadata)
        .def("GetSdrMetadataByKey", &Input::GetSdrMetadataByKey,
             nb::arg("key"))

        .def("SetSdrMetadata", &Input::SetSdrMetadata,
             nb::arg("sdrMetadata"))
        .def("SetSdrMetadataByKey", &Input::SetSdrMetadataByKey,
             nb::arg("key"), nb::arg("value"))

        .def("HasSdrMetadata", &Input::HasSdrMetadata)
        .def("HasSdrMetadataByKey", &Input::HasSdrMetadataByKey,
             nb::arg("key"))

        .def("ClearSdrMetadata", &Input::ClearSdrMetadata)
        .def("ClearSdrMetadataByKey", 
             &Input::ClearSdrMetadataByKey, nb::arg("key"))

        .def("SetDocumentation", &Input::SetDocumentation)
        .def("GetDocumentation", &Input::GetDocumentation)
        .def("SetDisplayGroup", &Input::SetDisplayGroup)
        .def("GetDisplayGroup", &Input::GetDisplayGroup)

        .def("SetConnectability", &Input::SetConnectability)
        .def("GetConnectability", &Input::GetConnectability)
        .def("ClearConnectability", &Input::ClearConnectability)

        .def("GetValueProducingAttributes",
            &Input::GetValueProducingAttributes,
            nb::arg("shaderOutputsOnly")=false)
        .def("GetValueProducingAttribute", _GetValueProducingAttribute)

        .def("GetAttr", &Input::GetAttr,
             nb::rv_policy::copy)

        .def("CanConnect", CanConnect_1,
            nb::arg("source"))

        .def("ConnectToSource", ConnectToSource_5,
            nb::arg("source"),
            nb::arg("mod")=UsdShadeConnectionModification::Replace)
        .def("ConnectToSource", ConnectToSource_1,
            nb::arg("source"), nb::arg("sourceName"), 
            nb::arg("sourceType")=UsdShadeAttributeType::Output,
            nb::arg("typeName")=SdfValueTypeName())
        .def("ConnectToSource", ConnectToSource_2,
            nb::arg("sourcePath"))
        .def("ConnectToSource", ConnectToSource_3,
            nb::arg("input"))
        .def("ConnectToSource", ConnectToSource_4,
            nb::arg("output"))

        .def("SetConnectedSources", &Input::SetConnectedSources)

        .def("GetConnectedSources", _GetConnectedSources)
        .def("GetConnectedSource", _GetConnectedSource)
        .def("GetRawConnectedSourcePaths", _GetRawConnectedSourcePaths,
            nb::rv_policy::copy)
        .def("HasConnectedSource", &Input::HasConnectedSource)
        .def("IsSourceConnectionFromBaseMaterial",
             &Input::IsSourceConnectionFromBaseMaterial)
        .def("DisconnectSource", &Input::DisconnectSource,
             nb::arg("sourceAttr")=UsdAttribute())
        .def("ClearSources", &Input::ClearSources)
        .def("ClearSource", &Input::ClearSource)

        .def_static("IsInput", &Input::IsInput)
        .def_static("IsInterfaceInputName", &Input::IsInterfaceInputName)
        ;

    nb::implicitly_convertible<Input, UsdAttribute>();
    nb::implicitly_convertible<Input, UsdProperty>();
    nb::implicitly_convertible<Input, UsdObject>();

}

