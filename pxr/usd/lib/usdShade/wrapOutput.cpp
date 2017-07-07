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

using std::vector;
using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static bool
_Set(const UsdShadeOutput &self, object val, const UsdTimeCode &time) 
{
    return self.Set(UsdPythonToSdfType(val, self.GetTypeName()), time);
}

static object
_GetConnectedSource(const UsdShadeOutput &self)
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

    bool (Output::*CanConnect_1)(
        UsdAttribute const &) const = &Output::CanConnect;

    class_<Output>("Output")
        .def(init<UsdAttribute>(arg("attr")))
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

        .def("GetAttr", &Output::GetAttr)
        .def("GetRel", &Output::GetRel)
        .def("GetProperty", &Output::GetProperty,
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
            (arg("sourceInput")))
        .def("ConnectToSource", ConnectToSource_4,
            (arg("sourceOutput")))

        .def("GetConnectedSource", _GetConnectedSource)
        .def("GetRawConnectedSourcePaths", _GetRawConnectedSourcePaths,
            return_value_policy<TfPySequenceToList>())
        .def("HasConnectedSource", &Output::HasConnectedSource)
        .def("IsSourceFromBaseMaterial", &Output::IsSourceFromBaseMaterial)
        .def("DisconnectSource", &Output::DisconnectSource)
        .def("ClearSource", &Output::ClearSource)
        
        ;

    implicitly_convertible<Output, UsdAttribute>();
    implicitly_convertible<Output, UsdProperty>();

    to_python_converter<
        std::vector<Output>,
        TfPySequenceToPython<std::vector<Output> > >();
}


