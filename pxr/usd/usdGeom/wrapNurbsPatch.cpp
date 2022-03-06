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
#include "pxr/usd/usdGeom/nurbsPatch.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>


PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdUsdGeomWrapNurbsPatch {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateUVertexCountAttr(UsdGeomNurbsPatch &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateUVertexCountAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateVVertexCountAttr(UsdGeomNurbsPatch &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateVVertexCountAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateUOrderAttr(UsdGeomNurbsPatch &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateUOrderAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateVOrderAttr(UsdGeomNurbsPatch &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateVOrderAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateUKnotsAttr(UsdGeomNurbsPatch &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateUKnotsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->DoubleArray), writeSparsely);
}
        
static UsdAttribute
_CreateVKnotsAttr(UsdGeomNurbsPatch &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateVKnotsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->DoubleArray), writeSparsely);
}
        
static UsdAttribute
_CreateUFormAttr(UsdGeomNurbsPatch &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateUFormAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateVFormAttr(UsdGeomNurbsPatch &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateVFormAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateURangeAttr(UsdGeomNurbsPatch &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateURangeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double2), writeSparsely);
}
        
static UsdAttribute
_CreateVRangeAttr(UsdGeomNurbsPatch &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateVRangeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double2), writeSparsely);
}
        
static UsdAttribute
_CreatePointWeightsAttr(UsdGeomNurbsPatch &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreatePointWeightsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->DoubleArray), writeSparsely);
}
        
static UsdAttribute
_CreateTrimCurveCountsAttr(UsdGeomNurbsPatch &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateTrimCurveCountsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateTrimCurveOrdersAttr(UsdGeomNurbsPatch &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateTrimCurveOrdersAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateTrimCurveVertexCountsAttr(UsdGeomNurbsPatch &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateTrimCurveVertexCountsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateTrimCurveKnotsAttr(UsdGeomNurbsPatch &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateTrimCurveKnotsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->DoubleArray), writeSparsely);
}
        
static UsdAttribute
_CreateTrimCurveRangesAttr(UsdGeomNurbsPatch &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateTrimCurveRangesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double2Array), writeSparsely);
}
        
static UsdAttribute
_CreateTrimCurvePointsAttr(UsdGeomNurbsPatch &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateTrimCurvePointsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double3Array), writeSparsely);
}

static std::string
_Repr(const UsdGeomNurbsPatch &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.NurbsPatch(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomNurbsPatch()
{
    typedef UsdGeomNurbsPatch This;

    boost::python::class_<This, boost::python::bases<UsdGeomPointBased> >
        cls("NurbsPatch");

    cls
        .def(boost::python::init<UsdPrim>(boost::python::arg("prim")))
        .def(boost::python::init<UsdSchemaBase const&>(boost::python::arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Get")

        .def("Define", &This::Define, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Define")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             boost::python::arg("includeInherited")=true,
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!boost::python::self)

        
        .def("GetUVertexCountAttr",
             &This::GetUVertexCountAttr)
        .def("CreateUVertexCountAttr",
             &pxrUsdUsdGeomWrapNurbsPatch::_CreateUVertexCountAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetVVertexCountAttr",
             &This::GetVVertexCountAttr)
        .def("CreateVVertexCountAttr",
             &pxrUsdUsdGeomWrapNurbsPatch::_CreateVVertexCountAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetUOrderAttr",
             &This::GetUOrderAttr)
        .def("CreateUOrderAttr",
             &pxrUsdUsdGeomWrapNurbsPatch::_CreateUOrderAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetVOrderAttr",
             &This::GetVOrderAttr)
        .def("CreateVOrderAttr",
             &pxrUsdUsdGeomWrapNurbsPatch::_CreateVOrderAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetUKnotsAttr",
             &This::GetUKnotsAttr)
        .def("CreateUKnotsAttr",
             &pxrUsdUsdGeomWrapNurbsPatch::_CreateUKnotsAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetVKnotsAttr",
             &This::GetVKnotsAttr)
        .def("CreateVKnotsAttr",
             &pxrUsdUsdGeomWrapNurbsPatch::_CreateVKnotsAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetUFormAttr",
             &This::GetUFormAttr)
        .def("CreateUFormAttr",
             &pxrUsdUsdGeomWrapNurbsPatch::_CreateUFormAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetVFormAttr",
             &This::GetVFormAttr)
        .def("CreateVFormAttr",
             &pxrUsdUsdGeomWrapNurbsPatch::_CreateVFormAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetURangeAttr",
             &This::GetURangeAttr)
        .def("CreateURangeAttr",
             &pxrUsdUsdGeomWrapNurbsPatch::_CreateURangeAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetVRangeAttr",
             &This::GetVRangeAttr)
        .def("CreateVRangeAttr",
             &pxrUsdUsdGeomWrapNurbsPatch::_CreateVRangeAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetPointWeightsAttr",
             &This::GetPointWeightsAttr)
        .def("CreatePointWeightsAttr",
             &pxrUsdUsdGeomWrapNurbsPatch::_CreatePointWeightsAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetTrimCurveCountsAttr",
             &This::GetTrimCurveCountsAttr)
        .def("CreateTrimCurveCountsAttr",
             &pxrUsdUsdGeomWrapNurbsPatch::_CreateTrimCurveCountsAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetTrimCurveOrdersAttr",
             &This::GetTrimCurveOrdersAttr)
        .def("CreateTrimCurveOrdersAttr",
             &pxrUsdUsdGeomWrapNurbsPatch::_CreateTrimCurveOrdersAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetTrimCurveVertexCountsAttr",
             &This::GetTrimCurveVertexCountsAttr)
        .def("CreateTrimCurveVertexCountsAttr",
             &pxrUsdUsdGeomWrapNurbsPatch::_CreateTrimCurveVertexCountsAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetTrimCurveKnotsAttr",
             &This::GetTrimCurveKnotsAttr)
        .def("CreateTrimCurveKnotsAttr",
             &pxrUsdUsdGeomWrapNurbsPatch::_CreateTrimCurveKnotsAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetTrimCurveRangesAttr",
             &This::GetTrimCurveRangesAttr)
        .def("CreateTrimCurveRangesAttr",
             &pxrUsdUsdGeomWrapNurbsPatch::_CreateTrimCurveRangesAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetTrimCurvePointsAttr",
             &This::GetTrimCurvePointsAttr)
        .def("CreateTrimCurvePointsAttr",
             &pxrUsdUsdGeomWrapNurbsPatch::_CreateTrimCurvePointsAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))

        .def("__repr__", pxrUsdUsdGeomWrapNurbsPatch::_Repr)
    ;

    pxrUsdUsdGeomWrapNurbsPatch::_CustomWrapCode(cls);
}

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
// Just remember to wrap code in the appropriate delimiters:
// 'namespace {', '}'.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

namespace pxrUsdUsdGeomWrapNurbsPatch {

WRAP_CUSTOM {
}

} // anonymous namespace
