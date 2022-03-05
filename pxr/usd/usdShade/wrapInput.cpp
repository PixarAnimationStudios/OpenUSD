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


PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static bool
_Set(const UsdShadeInput &self, boost::python::object val, const UsdTimeCode &time) 
{
    return self.Set(UsdPythonToSdfType(val, self.GetTypeName()), time);
}

static TfPyObjWrapper
_Get(const UsdShadeInput &self, UsdTimeCode time) {
    VtValue val;
    self.Get(&val, time);
    return UsdVtValueToPython(val);
}

static boost::python::object
_GetConnectedSources(const UsdShadeInput &self)
{
    SdfPathVector invalidSourcePaths;
    UsdShadeInput::SourceInfoVector sources =
        self.GetConnectedSources(&invalidSourcePaths);
    return boost::python::make_tuple(
        std::vector<UsdShadeConnectionSourceInfo>(sources.begin(), sources.end()),
        invalidSourcePaths);
}

static boost::python::object
_GetConnectedSource(const UsdShadeInput &self)
{
    UsdShadeConnectableAPI source;
    TfToken                sourceName;
    UsdShadeAttributeType  sourceType;
    
    if (self.GetConnectedSource(&source, &sourceName, &sourceType)){
        return boost::python::make_tuple(source, sourceName, sourceType);
    } else {
        return boost::python::object();
    }
}

static SdfPathVector
_GetRawConnectedSourcePaths(const UsdShadeInput &self) 
{
    SdfPathVector sourcePaths;
    self.GetRawConnectedSourcePaths(&sourcePaths);
    return sourcePaths;
}

static boost::python::object
_GetValueProducingAttribute(const UsdShadeInput &self)
{
    UsdShadeAttributeType attrType;
    UsdAttribute attr = self.GetValueProducingAttribute(&attrType);
    return boost::python::make_tuple(attr, attrType);
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

    boost::python::class_<Input>("Input")
        .def(boost::python::init<UsdAttribute>(boost::python::arg("attr")))
        .def(boost::python::self==boost::python::self)
        .def(boost::python::self!=boost::python::self)
        .def(!boost::python::self)

        .def("GetFullName", &Input::GetFullName,
                boost::python::return_value_policy<boost::python::return_by_value>())
        .def("GetBaseName", &Input::GetBaseName)
        .def("GetPrim", &Input::GetPrim)
        .def("GetTypeName", &Input::GetTypeName)
        .def("Get", _Get, (boost::python::arg("time")=UsdTimeCode::Default()))
        .def("Set", _Set, (boost::python::arg("value"), boost::python::arg("time")=UsdTimeCode::Default()))
        .def("SetRenderType", &Input::SetRenderType,
             (boost::python::arg("renderType")))
        .def("GetRenderType", &Input::GetRenderType)
        .def("HasRenderType", &Input::HasRenderType)

        .def("GetSdrMetadata", &Input::GetSdrMetadata)
        .def("GetSdrMetadataByKey", &Input::GetSdrMetadataByKey,
             (boost::python::arg("key")))

        .def("SetSdrMetadata", &Input::SetSdrMetadata,
             (boost::python::arg("sdrMetadata")))
        .def("SetSdrMetadataByKey", &Input::SetSdrMetadataByKey,
             (boost::python::arg("key"), boost::python::arg("value")))

        .def("HasSdrMetadata", &Input::HasSdrMetadata)
        .def("HasSdrMetadataByKey", &Input::HasSdrMetadataByKey,
             (boost::python::arg("key")))

        .def("ClearSdrMetadata", &Input::ClearSdrMetadata)
        .def("ClearSdrMetadataByKey", 
             &Input::ClearSdrMetadataByKey, (boost::python::arg("key")))

        .def("SetDocumentation", &Input::SetDocumentation)
        .def("GetDocumentation", &Input::GetDocumentation)
        .def("SetDisplayGroup", &Input::SetDisplayGroup)
        .def("GetDisplayGroup", &Input::GetDisplayGroup)

        .def("SetConnectability", &Input::SetConnectability)
        .def("GetConnectability", &Input::GetConnectability)
        .def("ClearConnectability", &Input::ClearConnectability)

        .def("GetValueProducingAttributes",
            &Input::GetValueProducingAttributes,
            (boost::python::arg("shaderOutputsOnly")=false))
        .def("GetValueProducingAttribute", _GetValueProducingAttribute)

        .def("GetAttr", &Input::GetAttr,
             boost::python::return_value_policy<boost::python::return_by_value>())

        .def("CanConnect", CanConnect_1,
            (boost::python::arg("source")))

        .def("ConnectToSource", ConnectToSource_5,
            (boost::python::arg("source"),
             boost::python::arg("mod")=UsdShadeConnectionModification::Replace))
        .def("ConnectToSource", ConnectToSource_1,
            (boost::python::arg("source"), boost::python::arg("sourceName"), 
             boost::python::arg("sourceType")=UsdShadeAttributeType::Output,
             boost::python::arg("typeName")=SdfValueTypeName()))
        .def("ConnectToSource", ConnectToSource_2,
            (boost::python::arg("sourcePath")))
        .def("ConnectToSource", ConnectToSource_3,
            (boost::python::arg("input")))
        .def("ConnectToSource", ConnectToSource_4,
            (boost::python::arg("output")))

        .def("SetConnectedSources", &Input::SetConnectedSources)

        .def("GetConnectedSources", _GetConnectedSources)
        .def("GetConnectedSource", _GetConnectedSource)
        .def("GetRawConnectedSourcePaths", _GetRawConnectedSourcePaths,
            boost::python::return_value_policy<TfPySequenceToList>())
        .def("HasConnectedSource", &Input::HasConnectedSource)
        .def("IsSourceConnectionFromBaseMaterial",
             &Input::IsSourceConnectionFromBaseMaterial)
        .def("DisconnectSource", &Input::DisconnectSource,
             (boost::python::arg("sourceAttr")=UsdAttribute()))
        .def("ClearSources", &Input::ClearSources)
        .def("ClearSource", &Input::ClearSource)

        .def("IsInput", &Input::IsInput)
        .staticmethod("IsInput")
        .def("IsInterfaceInputName", &Input::IsInterfaceInputName)
        .staticmethod("IsInterfaceInputName")
        ;

    boost::python::implicitly_convertible<Input, UsdAttribute>();

    boost::python::to_python_converter<
        std::vector<Input>,
        TfPySequenceToPython<std::vector<Input> > >();
}

