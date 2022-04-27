//
// Unlicensed 2022 Pixar
//

#include "pxr/pxr.h"
#include "pxr/usd/usdExec/execOutput.h"
#include "pxr/usd/usdExec/execConnectableAPI.h"

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
_Set(const UsdExecOutput &self, object val, const UsdTimeCode &time) 
{
    return self.Set(UsdPythonToSdfType(val, self.GetTypeName()), time);
}

static object
_GetConnectedSources(const UsdExecOutput &self)
{
    SdfPathVector invalidSourcePaths;
    UsdExecOutput::SourceInfoVector sources =
        self.GetConnectedSources(&invalidSourcePaths);
    return boost::python::make_tuple(
        std::vector<UsdExecConnectionSourceInfo>(sources.begin(), sources.end()),
        invalidSourcePaths);
}

static object
_GetConnectedSource(const UsdExecOutput &self)
{
    UsdExecConnectableAPI source;
    TfToken                sourceName;
    UsdExecAttributeType  sourceType;
    
    if (self.GetConnectedSource(&source, &sourceName, &sourceType)){
        return boost::python::make_tuple(source, sourceName, sourceType);
    } else {
        return object();
    }
}

static SdfPathVector
_GetRawConnectedSourcePaths(const UsdExecOutput &self) 
{
    SdfPathVector sourcePaths;
    self.GetRawConnectedSourcePaths(&sourcePaths);
    return sourcePaths;
}

} // anonymous namespace 


void wrapUsdExecOutput()
{
    typedef UsdExecOutput Output;

    bool (Output::*ConnectToSource_1)(
        UsdExecConnectableAPI const&,
        TfToken const &,
        UsdExecAttributeType const,
        SdfValueTypeName) const = &Output::ConnectToSource;

    bool (Output::*ConnectToSource_2)(
        SdfPath const &) const = &Output::ConnectToSource;

    bool (Output::*ConnectToSource_3)(
        UsdExecInput const &) const = &Output::ConnectToSource;

    bool (Output::*ConnectToSource_4)(
        UsdExecOutput const &) const = &Output::ConnectToSource;

    bool (Output::*ConnectToSource_5)(
        UsdExecConnectionSourceInfo const &,
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

        .def("GetExecMetadata", &Output::GetExecMetadata)
        .def("GetExecMetadataByKey", &Output::GetExecMetadataByKey,
             (arg("key")))

        .def("SetExecMetadata", &Output::SetExecMetadata,
             (arg("execMetadata")))
        .def("SetExecMetadataByKey", &Output::SetExecMetadataByKey,
             (arg("key"), arg("value")))

        .def("HasExecMetadata", &Output::HasExecMetadata)
        .def("HasExecMetadataByKey", &Output::HasExecMetadataByKey,
             (arg("key")))

        .def("ClearExecMetadata", &Output::ClearExecMetadata)
        .def("ClearExecMetadataByKey", 
             &Output::ClearExecMetadataByKey, (arg("key")))

        .def("GetAttr", &Output::GetAttr)

        .def("CanConnect", CanConnect_1,
            (arg("source")))

        .def("ConnectToSource", ConnectToSource_5,
            (arg("source"),
             arg("mod")=UsdExecConnectionModification::Replace))
        .def("ConnectToSource", ConnectToSource_1,
            (arg("source"), arg("sourceName"), 
             arg("sourceType")=UsdExecAttributeType::Output,
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
        .def("DisconnectSource", &Output::DisconnectSource,
             (arg("sourceAttr")=UsdAttribute()))
        .def("ClearSources", &Output::ClearSources)
        .def("ClearSource", &Output::ClearSource)

        .def("GetValueProducingAttributes",
             &Output::GetValueProducingAttributes,
             (arg("outputsOnly")=false))

        .def("IsOutput", &Output::IsOutput)
        .staticmethod("IsOutput")
        ;

    implicitly_convertible<Output, UsdAttribute>();

    to_python_converter<
        std::vector<Output>,
        TfPySequenceToPython<std::vector<Output> > >();
}


