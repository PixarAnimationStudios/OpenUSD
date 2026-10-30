//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
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
_Set(const UsdShadeOutput &self, nb::handle val, const UsdTimeCode &time) 
{
    return self.Set(UsdPythonToSdfType(val, self.GetTypeName()), time);
}

static nb::object
_GetConnectedSources(const UsdShadeOutput &self)
{
    SdfPathVector invalidSourcePaths;
    UsdShadeOutput::SourceInfoVector sources =
        self.GetConnectedSources(&invalidSourcePaths);
    return nb::make_tuple(
        std::vector<UsdShadeConnectionSourceInfo>(sources.begin(), sources.end()),
        invalidSourcePaths);
}

static nb::object
_GetConnectedSource(const UsdShadeOutput &self)
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
_GetRawConnectedSourcePaths(const UsdShadeOutput &self) 
{
    SdfPathVector sourcePaths;
    self.GetRawConnectedSourcePaths(&sourcePaths);
    return sourcePaths;
}

} // anonymous namespace 

void wrapUsdShadeOutput(nanobind::module_ &m)
{
    typedef UsdShadeOutput Output;

    bool (Output::*ConnectToSource_1)(
        UsdShadeConnectableAPI const&,
        TfToken const &,
        UsdShadeAttributeType const,
        SdfValueTypeName) const = &Output::ConnectToSource;

    bool (Output::*ConnectToSource_2)(
        SdfPath const &) const = &Output::ConnectToSource;

    bool (Output::*ConnectToSource_3)(
        UsdShadeInput const &) const = &Output::ConnectToSource;

    bool (Output::*ConnectToSource_4)(
        UsdShadeOutput const &) const = &Output::ConnectToSource;

    bool (Output::*ConnectToSource_5)(
        UsdShadeConnectionSourceInfo const &,
        Output::ConnectionModification const mod) const = &Output::ConnectToSource;

    bool (Output::*CanConnect_1)(
        UsdAttribute const &) const = &Output::CanConnect;

    nb::class_<Output>(m, "Output")
        .def(nb::init<>())
        .def(nb::init<UsdAttribute>(), nb::arg("attr"))
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def("__hash__", [](const Output &self) { return hash_value(self.GetAttr()); })
        .def("__bool__", [](const Output &self) { return bool(self); })

        .def("GetFullName", &Output::GetFullName,
             nb::rv_policy::copy)
        .def("GetBaseName", &Output::GetBaseName)
        .def("GetPrim", &Output::GetPrim)
        .def("GetTypeName", &Output::GetTypeName)
        .def("Set", _Set, nb::arg("value").none(), nb::arg("time")=UsdTimeCode::Default())
        .def("SetRenderType", &Output::SetRenderType,
             nb::arg("renderType"))
        .def("GetRenderType", &Output::GetRenderType)
        .def("HasRenderType", &Output::HasRenderType)

        .def("GetSdrMetadata", &Output::GetSdrMetadata)
        .def("GetSdrMetadataByKey", &Output::GetSdrMetadataByKey,
             nb::arg("key"))

        .def("SetSdrMetadata", &Output::SetSdrMetadata,
             nb::arg("sdrMetadata"))
        .def("SetSdrMetadataByKey", &Output::SetSdrMetadataByKey,
             nb::arg("key"), nb::arg("value"))

        .def("HasSdrMetadata", &Output::HasSdrMetadata)
        .def("HasSdrMetadataByKey", &Output::HasSdrMetadataByKey,
             nb::arg("key"))

        .def("ClearSdrMetadata", &Output::ClearSdrMetadata)
        .def("ClearSdrMetadataByKey", 
             &Output::ClearSdrMetadataByKey, nb::arg("key"))

        .def("GetAttr", &Output::GetAttr,
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
            nb::arg("sourceInput"))
        .def("ConnectToSource", ConnectToSource_4,
            nb::arg("sourceOutput"))

        .def("SetConnectedSources", &Output::SetConnectedSources)

        .def("GetConnectedSources", _GetConnectedSources)
        .def("GetConnectedSource", _GetConnectedSource)
        .def("GetRawConnectedSourcePaths", _GetRawConnectedSourcePaths,
            nb::rv_policy::copy)
        .def("HasConnectedSource", &Output::HasConnectedSource)
        .def("IsSourceConnectionFromBaseMaterial", 
             &Output::IsSourceConnectionFromBaseMaterial)
        .def("DisconnectSource", &Output::DisconnectSource,
             nb::arg("sourceAttr")=UsdAttribute())
        .def("ClearSources", &Output::ClearSources)
        .def("ClearSource", &Output::ClearSource)

        .def("GetValueProducingAttributes",
             &Output::GetValueProducingAttributes,
             nb::arg("shaderOutputsOnly")=false)

        .def_static("IsOutput", &Output::IsOutput)
        ;

    nb::implicitly_convertible<Output, UsdAttribute>();
    nb::implicitly_convertible<Output, UsdProperty>();
    nb::implicitly_convertible<Output, UsdObject>();

}

