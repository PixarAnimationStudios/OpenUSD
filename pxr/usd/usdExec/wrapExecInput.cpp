//
// Unlicensed 2022 benmalartre
//
#include "pxr/usd/usdExec/execInput.h"
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
_Set(const UsdExecInput &self, object val, const UsdTimeCode &time) 
{
    return self.Set(UsdPythonToSdfType(val, self.GetTypeName()), time);
}

static TfPyObjWrapper
_Get(const UsdExecInput &self, UsdTimeCode time) {
    VtValue val;
    self.Get(&val, time);
    return UsdVtValueToPython(val);
}

static object
_GetConnectedSources(const UsdExecInput &self)
{
    SdfPathVector invalidSourcePaths;
    UsdExecInput::SourceInfoVector sources =
        self.GetConnectedSources(&invalidSourcePaths);
    return boost::python::make_tuple(
        std::vector<UsdExecConnectionSourceInfo>(sources.begin(), sources.end()),
        invalidSourcePaths);
}

static object
_GetConnectedSource(const UsdExecInput &self)
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
_GetRawConnectedSourcePaths(const UsdExecInput &self) 
{
    SdfPathVector sourcePaths;
    self.GetRawConnectedSourcePaths(&sourcePaths);
    return sourcePaths;
}

static object
_GetValueProducingAttribute(const UsdExecInput &self)
{
    UsdExecAttributeType attrType;
    UsdAttribute attr = self.GetValueProducingAttribute(&attrType);
    return boost::python::make_tuple(attr, attrType);
}

} // anonymous namespace 

void wrapUsdExecInput()
{
    typedef UsdExecInput Input;

    bool (Input::*ConnectToSource_1)(
        UsdExecConnectableAPI const&,
        TfToken const &,
        UsdExecAttributeType const,
        SdfValueTypeName) const = &Input::ConnectToSource;

    bool (Input::*ConnectToSource_2)(
        SdfPath const &) const = &Input::ConnectToSource;

    bool (Input::*ConnectToSource_3)(
        UsdExecInput const &) const = &Input::ConnectToSource;

    bool (Input::*ConnectToSource_4)(
        UsdExecOutput const &) const = &Input::ConnectToSource;

    bool (Input::*ConnectToSource_5)(
        UsdExecConnectionSourceInfo const &,
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

        .def("GetExecMetadata", &Input::GetExecMetadata)
        .def("GetExecMetadataByKey", &Input::GetExecMetadataByKey,
             (arg("key")))

        .def("SetExecMetadata", &Input::SetExecMetadata,
             (arg("execMetadata")))
        .def("SetExecMetadataByKey", &Input::SetExecMetadataByKey,
             (arg("key"), arg("value")))

        .def("HasExecMetadata", &Input::HasExecMetadata)
        .def("HasExecMetadataByKey", &Input::HasExecMetadataByKey,
             (arg("key")))

        .def("ClearExecMetadata", &Input::ClearExecMetadata)
        .def("ClearExecMetadataByKey", 
             &Input::ClearExecMetadataByKey, (arg("key")))

        .def("SetDocumentation", &Input::SetDocumentation)
        .def("GetDocumentation", &Input::GetDocumentation)
        .def("SetDisplayGroup", &Input::SetDisplayGroup)
        .def("GetDisplayGroup", &Input::GetDisplayGroup)

        .def("SetConnectability", &Input::SetConnectability)
        .def("GetConnectability", &Input::GetConnectability)
        .def("ClearConnectability", &Input::ClearConnectability)

        .def("GetValueProducingAttributes",
            &Input::GetValueProducingAttributes,
            (arg("outputsOnly")=false))
        .def("GetValueProducingAttribute", _GetValueProducingAttribute)

        .def("GetAttr", &Input::GetAttr,
             return_value_policy<return_by_value>())

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
            (arg("input")))
        .def("ConnectToSource", ConnectToSource_4,
            (arg("output")))

        .def("SetConnectedSources", &Input::SetConnectedSources)

        .def("GetConnectedSources", _GetConnectedSources)
        .def("GetConnectedSource", _GetConnectedSource)
        .def("GetRawConnectedSourcePaths", _GetRawConnectedSourcePaths,
            return_value_policy<TfPySequenceToList>())
        .def("HasConnectedSource", &Input::HasConnectedSource)
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

