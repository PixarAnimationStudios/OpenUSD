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

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/implicit.hpp"
#include "pxr/external/boost/python/tuple.hpp"

#include <vector>

using std::vector;
PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static bool
_Set(const UsdShadeInput &self, object val, const UsdTimeCode &time) 
{
    return self.Set(UsdPythonToSdfType(val, self.GetTypeName()), time);
}

static TfPyObjWrapper
_Get(const UsdShadeInput &self, UsdTimeCode time) {
    VtValue val;
    self.Get(&val, time);
    return UsdVtValueToPython(val);
}

static object
_GetConnectedSources(const UsdShadeInput &self)
{
    SdfPathVector invalidSourcePaths;
    UsdShadeInput::SourceInfoVector sources =
        self.GetConnectedSources(&invalidSourcePaths);
    return pxr_boost::python::make_tuple(
        std::vector<UsdShadeConnectionSourceInfo>(sources.begin(), sources.end()),
        invalidSourcePaths);
}

static object
_GetConnectedSource(const UsdShadeInput &self)
{
    UsdShadeConnectableAPI source;
    TfToken                sourceName;
    UsdShadeAttributeType  sourceType;
    
    if (self.GetConnectedSource(&source, &sourceName, &sourceType)){
        return pxr_boost::python::make_tuple(source, sourceName, sourceType);
    } else {
        return object();
    }
}

static SdfPathVector
_GetRawConnectedSourcePaths(const UsdShadeInput &self) 
{
    SdfPathVector sourcePaths;
    self.GetRawConnectedSourcePaths(&sourcePaths);
    return sourcePaths;
}

static object
_GetValueProducingAttribute(const UsdShadeInput &self)
{
    UsdShadeAttributeType attrType;
    UsdAttribute attr = self.GetValueProducingAttribute(&attrType);
    return pxr_boost::python::make_tuple(attr, attrType);
}

} // anonymous namespace 

void wrapUsdShadeInput()
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

    class_<Input>("Input")
        .def(init<UsdAttribute>(arg("attr")))
        .def(self==self)
        .def(self!=self)
        .def(!self)

        .def("GetFullName", &Input::GetFullName,
                return_value_policy<return_by_value>())
        .def("GetBaseName", &Input::GetBaseName)
        .def("GetPrim", &Input::GetPrim)
        .def("GetTypeName", &Input::GetTypeName)
        .def("Get", _Get, (arg("time")=UsdTimeCode::Default()))
        .def("Set", _Set, (arg("value"), arg("time")=UsdTimeCode::Default()))
        .def("SetRenderType", &Input::SetRenderType,
             (arg("renderType")))
        .def("GetRenderType", &Input::GetRenderType)
        .def("HasRenderType", &Input::HasRenderType)

        .def("GetSdrMetadata", &Input::GetSdrMetadata)
        .def("GetSdrMetadataByKey", &Input::GetSdrMetadataByKey,
             (arg("key")))

        .def("SetSdrMetadata", &Input::SetSdrMetadata,
             (arg("sdrMetadata")))
        .def("SetSdrMetadataByKey", &Input::SetSdrMetadataByKey,
             (arg("key"), arg("value")))

        .def("HasSdrMetadata", &Input::HasSdrMetadata)
        .def("HasSdrMetadataByKey", &Input::HasSdrMetadataByKey,
             (arg("key")))

        .def("ClearSdrMetadata", &Input::ClearSdrMetadata)
        .def("ClearSdrMetadataByKey", 
             &Input::ClearSdrMetadataByKey, (arg("key")))

        .def("SetDocumentation", &Input::SetDocumentation)
        .def("GetDocumentation", &Input::GetDocumentation)
        .def("SetDisplayGroup", &Input::SetDisplayGroup)
        .def("GetDisplayGroup", &Input::GetDisplayGroup)

        .def("SetConnectability", &Input::SetConnectability)
        .def("GetConnectability", &Input::GetConnectability)
        .def("ClearConnectability", &Input::ClearConnectability)

        .def("GetValueProducingAttributes",
            &Input::GetValueProducingAttributes,
            (arg("shaderOutputsOnly")=false))
        .def("GetValueProducingAttribute", _GetValueProducingAttribute)

        .def("GetAttr", &Input::GetAttr,
             return_value_policy<return_by_value>())

        .def("CanConnect", CanConnect_1,
            (arg("source")))

        .def("ConnectToSource", ConnectToSource_5,
            (arg("source"),
             arg("mod")=UsdShadeConnectionModification::Replace))
        .def("ConnectToSource", ConnectToSource_1,
            (arg("source"), arg("sourceName"), 
             arg("sourceType")=UsdShadeAttributeType::Output,
             arg("typeName")=SdfValueTypeName()))
        .def("ConnectToSource", ConnectToSource_2,
            (arg("sourcePath")))
        .def("ConnectToSource", ConnectToSource_3,
            (arg("input")))
        .def("ConnectToSource", ConnectToSource_4,
            (arg("output")))

        .def("SetConnectedSources", &Input::SetConnectedSources)

        .def("GetConnectedSources", _GetConnectedSources)
        .def("GetConnectedSource", _GetConnectedSource)
        .def("GetRawConnectedSourcePaths", _GetRawConnectedSourcePaths,
            return_value_policy<TfPySequenceToList>())
        .def("HasConnectedSource", &Input::HasConnectedSource)
        .def("IsSourceConnectionFromBaseMaterial",
             &Input::IsSourceConnectionFromBaseMaterial)
        .def("DisconnectSource", &Input::DisconnectSource,
             (arg("sourceAttr")=UsdAttribute()))
        .def("ClearSources", &Input::ClearSources)
        .def("ClearSource", &Input::ClearSource)

        .def("IsInput", &Input::IsInput)
        .staticmethod("IsInput")
        .def("IsInterfaceInputName", &Input::IsInterfaceInputName)
        .staticmethod("IsInterfaceInputName")
        ;

    implicitly_convertible<Input, UsdAttribute>();

    to_python_converter<
        std::vector<Input>,
        TfPySequenceToPython<std::vector<Input> > >();
}

