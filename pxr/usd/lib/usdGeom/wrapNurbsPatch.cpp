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
#include "pxr/usd/usdGeom/nurbsPatch.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/conversions.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateUVertexCountAttr(UsdGeomNurbsPatch &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateUVertexCountAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateVVertexCountAttr(UsdGeomNurbsPatch &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVVertexCountAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateUOrderAttr(UsdGeomNurbsPatch &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateUOrderAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateVOrderAttr(UsdGeomNurbsPatch &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVOrderAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateUKnotsAttr(UsdGeomNurbsPatch &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateUKnotsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->DoubleArray), writeSparsely);
}
        
static UsdAttribute
_CreateVKnotsAttr(UsdGeomNurbsPatch &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVKnotsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->DoubleArray), writeSparsely);
}
        
static UsdAttribute
_CreateUFormAttr(UsdGeomNurbsPatch &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateUFormAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateVFormAttr(UsdGeomNurbsPatch &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVFormAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateURangeAttr(UsdGeomNurbsPatch &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateURangeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double2), writeSparsely);
}
        
static UsdAttribute
_CreateVRangeAttr(UsdGeomNurbsPatch &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVRangeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double2), writeSparsely);
}
        
static UsdAttribute
_CreatePointWeightsAttr(UsdGeomNurbsPatch &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePointWeightsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->DoubleArray), writeSparsely);
}
        
static UsdAttribute
_CreateTrimCurveCountsAttr(UsdGeomNurbsPatch &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTrimCurveCountsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateTrimCurveOrdersAttr(UsdGeomNurbsPatch &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTrimCurveOrdersAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateTrimCurveVertexCountsAttr(UsdGeomNurbsPatch &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTrimCurveVertexCountsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateTrimCurveKnotsAttr(UsdGeomNurbsPatch &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTrimCurveKnotsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->DoubleArray), writeSparsely);
}
        
static UsdAttribute
_CreateTrimCurveRangesAttr(UsdGeomNurbsPatch &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTrimCurveRangesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double2Array), writeSparsely);
}
        
static UsdAttribute
_CreateTrimCurvePointsAttr(UsdGeomNurbsPatch &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTrimCurvePointsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double3Array), writeSparsely);
}

void wrapUsdGeomNurbsPatch()
{
    typedef UsdGeomNurbsPatch This;

    class_<This, bases<UsdGeomPointBased> >
        cls("NurbsPatch");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("Define", &This::Define, (arg("stage"), arg("path")))
        .staticmethod("Define")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetUVertexCountAttr",
             &This::GetUVertexCountAttr)
        .def("CreateUVertexCountAttr",
             &_CreateUVertexCountAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVVertexCountAttr",
             &This::GetVVertexCountAttr)
        .def("CreateVVertexCountAttr",
             &_CreateVVertexCountAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetUOrderAttr",
             &This::GetUOrderAttr)
        .def("CreateUOrderAttr",
             &_CreateUOrderAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVOrderAttr",
             &This::GetVOrderAttr)
        .def("CreateVOrderAttr",
             &_CreateVOrderAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetUKnotsAttr",
             &This::GetUKnotsAttr)
        .def("CreateUKnotsAttr",
             &_CreateUKnotsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVKnotsAttr",
             &This::GetVKnotsAttr)
        .def("CreateVKnotsAttr",
             &_CreateVKnotsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetUFormAttr",
             &This::GetUFormAttr)
        .def("CreateUFormAttr",
             &_CreateUFormAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVFormAttr",
             &This::GetVFormAttr)
        .def("CreateVFormAttr",
             &_CreateVFormAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetURangeAttr",
             &This::GetURangeAttr)
        .def("CreateURangeAttr",
             &_CreateURangeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVRangeAttr",
             &This::GetVRangeAttr)
        .def("CreateVRangeAttr",
             &_CreateVRangeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPointWeightsAttr",
             &This::GetPointWeightsAttr)
        .def("CreatePointWeightsAttr",
             &_CreatePointWeightsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTrimCurveCountsAttr",
             &This::GetTrimCurveCountsAttr)
        .def("CreateTrimCurveCountsAttr",
             &_CreateTrimCurveCountsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTrimCurveOrdersAttr",
             &This::GetTrimCurveOrdersAttr)
        .def("CreateTrimCurveOrdersAttr",
             &_CreateTrimCurveOrdersAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTrimCurveVertexCountsAttr",
             &This::GetTrimCurveVertexCountsAttr)
        .def("CreateTrimCurveVertexCountsAttr",
             &_CreateTrimCurveVertexCountsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTrimCurveKnotsAttr",
             &This::GetTrimCurveKnotsAttr)
        .def("CreateTrimCurveKnotsAttr",
             &_CreateTrimCurveKnotsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTrimCurveRangesAttr",
             &This::GetTrimCurveRangesAttr)
        .def("CreateTrimCurveRangesAttr",
             &_CreateTrimCurveRangesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTrimCurvePointsAttr",
             &This::GetTrimCurvePointsAttr)
        .def("CreateTrimCurvePointsAttr",
             &_CreateTrimCurvePointsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

    ;

    _CustomWrapCode(cls);
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line, it will be preserved by 
// the code generator.  The entry point for your custom code should look
// minimally like the following:
//
// WRAP_CUSTOM {
//     _class
//         .def("MyCustomMethod", ...)
//     ;
// }
//
// Of course any other ancillary or support code may be provided.
// 
// Just remember to wrap code in the pxr namespace macros:
// PXR_NAMESPACE_OPEN_SCOPE, PXR_NAMESPACE_CLOSE_SCOPE.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

PXR_NAMESPACE_OPEN_SCOPE

WRAP_CUSTOM {
}

PXR_NAMESPACE_CLOSE_SCOPE
