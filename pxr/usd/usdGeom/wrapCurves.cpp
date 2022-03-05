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
#include "pxr/usd/usdGeom/curves.h"
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

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateCurveVertexCountsAttr(UsdGeomCurves &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateCurveVertexCountsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateWidthsAttr(UsdGeomCurves &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateWidthsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}

static std::string
_Repr(const UsdGeomCurves &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.Curves(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomCurves()
{
    typedef UsdGeomCurves This;

    boost::python::class_<This, boost::python::bases<UsdGeomPointBased> >
        cls("Curves");

    cls
        .def(boost::python::init<UsdPrim>(boost::python::arg("prim")))
        .def(boost::python::init<UsdSchemaBase const&>(boost::python::arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Get")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             boost::python::arg("includeInherited")=true,
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!boost::python::self)

        
        .def("GetCurveVertexCountsAttr",
             &This::GetCurveVertexCountsAttr)
        .def("CreateCurveVertexCountsAttr",
             &_CreateCurveVertexCountsAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetWidthsAttr",
             &This::GetWidthsAttr)
        .def("CreateWidthsAttr",
             &_CreateWidthsAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))

        .def("__repr__", ::_Repr)
    ;

    _CustomWrapCode(cls);
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

namespace {

static TfPyObjWrapper 
_ComputeExtent(boost::python::object points, boost::python::object widths) {
  
    // Convert from python objects to VtValue
    VtVec3fArray extent;
    VtValue pointsAsVtValue = UsdPythonToSdfType(points, 
        SdfValueTypeNames->Float3Array);
    VtValue widthsAsVtValue = UsdPythonToSdfType(widths, 
        SdfValueTypeNames->FloatArray);

    // Check Proper conversion to VtVec3fArray
    if (!pointsAsVtValue.IsHolding<VtVec3fArray>()) {
        TF_CODING_ERROR("Improper value for 'points'");
        return boost::python::object();
    }

    if (!widthsAsVtValue.IsHolding<VtFloatArray>()) {
        TF_CODING_ERROR("Improper value for 'widths'");
        return boost::python::object();
    }

    // Convert from VtValue to VtVec3fArray
    VtVec3fArray pointsArray = pointsAsVtValue.UncheckedGet<VtVec3fArray>();
    VtFloatArray widthsArray = widthsAsVtValue.UncheckedGet<VtFloatArray>();

    if (UsdGeomCurves::ComputeExtent(pointsArray, widthsArray, &extent)) {
        return UsdVtValueToPython(VtValue(extent));
    } else {
        return boost::python::object();
    }
}

WRAP_CUSTOM {
    _class
        .def("GetWidthsInterpolation", &UsdGeomCurves::GetWidthsInterpolation)
        .def("SetWidthsInterpolation", &UsdGeomCurves::SetWidthsInterpolation,
             boost::python::arg("interpolation"))

        .def("ComputeExtent",
            &_ComputeExtent, 
            (boost::python::arg("points"), boost::python::arg("widths")))
        .def("GetCurveCount", &UsdGeomCurves::GetCurveCount,
            boost::python::arg("timeCode")=UsdTimeCode::Default())
        .staticmethod("ComputeExtent")
        ;
}

} // anonymous namespace 
