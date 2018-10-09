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
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/output.h"
#include "pxr/usd/usdShade/connectableAPI.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/tuple.hpp>

#include <vector>

using std::vector;
using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

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
_GetConnectedSource(const UsdShadeInput &self)
{
    UsdShadeConnectableAPI source;
    TfToken                sourceName;
    UsdShadeAttributeType  sourceType;
    
    if (self.GetConnectedSource(&source, &sourceName, &sourceType)){
        return boost::python::make_tuple(source, sourceName, sourceType);
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

    bool (Input::*CanConnect_1)(
        UsdAttribute const &) const = &Input::CanConnect;

    class_<Input>("Input")
        .def(init<UsdAttribute>(arg("attr")))
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

        .def("GetAttr", &Input::GetAttr,
             return_value_policy<return_by_value>())

        .def("CanConnect", CanConnect_1,
            (arg("source")))

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

        .def("GetConnectedSource", _GetConnectedSource)
        .def("GetRawConnectedSourcePaths", _GetRawConnectedSourcePaths,
            return_value_policy<TfPySequenceToList>())
        .def("HasConnectedSource", &Input::HasConnectedSource)
        .def("IsSourceConnectionFromBaseMaterial",
             &Input::IsSourceConnectionFromBaseMaterial)
        .def("DisconnectSource", &Input::DisconnectSource)
        .def("ClearSource", &Input::ClearSource)

        .def("IsInput", &Input::IsInput)
        .staticmethod("IsInput")
        .def("IsInterfaceInputName", &Input::IsInterfaceInputName)
        .staticmethod("IsInterfaceInputName")
        ;

    implicitly_convertible<Input, UsdAttribute>();
    implicitly_convertible<Input, UsdProperty>();

    to_python_converter<
        std::vector<Input>,
        TfPySequenceToPython<std::vector<Input> > >();
}

