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
#include "pxr/pxr.h"
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
_Set(const UsdShadeOutput &self, boost::python::object val, const UsdTimeCode &time) 
{
    return self.Set(UsdPythonToSdfType(val, self.GetTypeName()), time);
}

static boost::python::object
_GetConnectedSources(const UsdShadeOutput &self)
{
    SdfPathVector invalidSourcePaths;
    UsdShadeOutput::SourceInfoVector sources =
        self.GetConnectedSources(&invalidSourcePaths);
    return boost::python::make_tuple(
        std::vector<UsdShadeConnectionSourceInfo>(sources.begin(), sources.end()),
        invalidSourcePaths);
}

static boost::python::object
_GetConnectedSource(const UsdShadeOutput &self)
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
_GetRawConnectedSourcePaths(const UsdShadeOutput &self) 
{
    SdfPathVector sourcePaths;
    self.GetRawConnectedSourcePaths(&sourcePaths);
    return sourcePaths;
}

} // anonymous namespace 


void wrapUsdShadeOutput()
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

    boost::python::class_<Output>("Output")
        .def(boost::python::init<UsdAttribute>(boost::python::arg("attr")))
        .def(boost::python::self==boost::python::self)
        .def(boost::python::self!=boost::python::self)
        .def(!boost::python::self)

        .def("GetFullName", &Output::GetFullName,
                boost::python::return_value_policy<boost::python::return_by_value>())
        .def("GetBaseName", &Output::GetBaseName)
        .def("GetPrim", &Output::GetPrim)
        .def("GetTypeName", &Output::GetTypeName)
        .def("Set", _Set, (boost::python::arg("value"), boost::python::arg("time")=UsdTimeCode::Default()))
        .def("SetRenderType", &Output::SetRenderType,
             (boost::python::arg("renderType")))
        .def("GetRenderType", &Output::GetRenderType)
        .def("HasRenderType", &Output::HasRenderType)

        .def("GetSdrMetadata", &Output::GetSdrMetadata)
        .def("GetSdrMetadataByKey", &Output::GetSdrMetadataByKey,
             (boost::python::arg("key")))

        .def("SetSdrMetadata", &Output::SetSdrMetadata,
             (boost::python::arg("sdrMetadata")))
        .def("SetSdrMetadataByKey", &Output::SetSdrMetadataByKey,
             (boost::python::arg("key"), boost::python::arg("value")))

        .def("HasSdrMetadata", &Output::HasSdrMetadata)
        .def("HasSdrMetadataByKey", &Output::HasSdrMetadataByKey,
             (boost::python::arg("key")))

        .def("ClearSdrMetadata", &Output::ClearSdrMetadata)
        .def("ClearSdrMetadataByKey", 
             &Output::ClearSdrMetadataByKey, (boost::python::arg("key")))

        .def("GetAttr", &Output::GetAttr)

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
            (boost::python::arg("sourceInput")))
        .def("ConnectToSource", ConnectToSource_4,
            (boost::python::arg("sourceOutput")))

        .def("SetConnectedSources", &Output::SetConnectedSources)

        .def("GetConnectedSources", _GetConnectedSources)
        .def("GetConnectedSource", _GetConnectedSource)
        .def("GetRawConnectedSourcePaths", _GetRawConnectedSourcePaths,
            boost::python::return_value_policy<TfPySequenceToList>())
        .def("HasConnectedSource", &Output::HasConnectedSource)
        .def("IsSourceConnectionFromBaseMaterial", 
             &Output::IsSourceConnectionFromBaseMaterial)
        .def("DisconnectSource", &Output::DisconnectSource,
             (boost::python::arg("sourceAttr")=UsdAttribute()))
        .def("ClearSources", &Output::ClearSources)
        .def("ClearSource", &Output::ClearSource)

        .def("GetValueProducingAttributes",
             &Output::GetValueProducingAttributes,
             (boost::python::arg("shaderOutputsOnly")=false))

        .def("IsOutput", &Output::IsOutput)
        .staticmethod("IsOutput")
        ;

    boost::python::implicitly_convertible<Output, UsdAttribute>();

    boost::python::to_python_converter<
        std::vector<Output>,
        TfPySequenceToPython<std::vector<Output> > >();
}


