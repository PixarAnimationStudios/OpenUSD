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
_Set(const UsdShadeOutput &self, object val, const UsdTimeCode &time) 
{
    return self.Set(UsdPythonToSdfType(val, self.GetTypeName()), time);
}

static object
_GetConnectedSources(const UsdShadeOutput &self)
{
    SdfPathVector invalidSourcePaths;
    UsdShadeOutput::SourceInfoVector sources =
        self.GetConnectedSources(&invalidSourcePaths);
    return pxr_boost::python::make_tuple(
        std::vector<UsdShadeConnectionSourceInfo>(sources.begin(), sources.end()),
        invalidSourcePaths);
}

static object
_GetConnectedSource(const UsdShadeOutput &self)
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

    class_<Output>("Output")
        .def(init<UsdAttribute>(arg("attr")))
        .def(self==self)
        .def(self!=self)
        .def(!self)

        .def("GetFullName", &Output::GetFullName,
                return_value_policy<return_by_value>())
        .def("GetBaseName", &Output::GetBaseName)
        .def("GetPrim", &Output::GetPrim)
        .def("GetTypeName", &Output::GetTypeName)
        .def("Set", _Set, (arg("value"), arg("time")=UsdTimeCode::Default()))
        .def("SetRenderType", &Output::SetRenderType,
             (arg("renderType")))
        .def("GetRenderType", &Output::GetRenderType)
        .def("HasRenderType", &Output::HasRenderType)

        .def("GetSdrMetadata", &Output::GetSdrMetadata)
        .def("GetSdrMetadataByKey", &Output::GetSdrMetadataByKey,
             (arg("key")))

        .def("SetSdrMetadata", &Output::SetSdrMetadata,
             (arg("sdrMetadata")))
        .def("SetSdrMetadataByKey", &Output::SetSdrMetadataByKey,
             (arg("key"), arg("value")))

        .def("HasSdrMetadata", &Output::HasSdrMetadata)
        .def("HasSdrMetadataByKey", &Output::HasSdrMetadataByKey,
             (arg("key")))

        .def("ClearSdrMetadata", &Output::ClearSdrMetadata)
        .def("ClearSdrMetadataByKey", 
             &Output::ClearSdrMetadataByKey, (arg("key")))

        .def("GetAttr", &Output::GetAttr,
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
            (arg("sourceInput")))
        .def("ConnectToSource", ConnectToSource_4,
            (arg("sourceOutput")))

        .def("SetConnectedSources", &Output::SetConnectedSources)

        .def("GetConnectedSources", _GetConnectedSources)
        .def("GetConnectedSource", _GetConnectedSource)
        .def("GetRawConnectedSourcePaths", _GetRawConnectedSourcePaths,
            return_value_policy<TfPySequenceToList>())
        .def("HasConnectedSource", &Output::HasConnectedSource)
        .def("IsSourceConnectionFromBaseMaterial", 
             &Output::IsSourceConnectionFromBaseMaterial)
        .def("DisconnectSource", &Output::DisconnectSource,
             (arg("sourceAttr")=UsdAttribute()))
        .def("ClearSources", &Output::ClearSources)
        .def("ClearSource", &Output::ClearSource)

        .def("GetValueProducingAttributes",
             &Output::GetValueProducingAttributes,
             (arg("shaderOutputsOnly")=false))

        .def("IsOutput", &Output::IsOutput)
        .staticmethod("IsOutput")
        ;

    implicitly_convertible<Output, UsdAttribute>();

    to_python_converter<
        std::vector<Output>,
        TfPySequenceToPython<std::vector<Output> > >();
}


