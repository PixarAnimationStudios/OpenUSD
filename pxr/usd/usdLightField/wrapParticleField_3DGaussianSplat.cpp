//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/particleField_3DGaussianSplat.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/external/boost/python.hpp"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateProjectionModeHintAttr(UsdLightFieldParticleField_3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateProjectionModeHintAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateSortingModeHintAttr(UsdLightFieldParticleField_3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateSortingModeHintAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdLightFieldParticleField_3DGaussianSplat &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdLightField.ParticleField_3DGaussianSplat(%s)",
        primRepr.c_str());
}

        
static UsdAttribute
_CreatePositionsAttr(UsdLightFieldParticleField_3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePositionsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3fArray), writeSparsely);
}
        
static UsdAttribute
_CreatePositionshAttr(UsdLightFieldParticleField_3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePositionshAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3hArray), writeSparsely);
}
        
static UsdAttribute
_CreateOrientationsAttr(UsdLightFieldParticleField_3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateOrientationsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->QuatfArray), writeSparsely);
}
        
static UsdAttribute
_CreateOrientationshAttr(UsdLightFieldParticleField_3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateOrientationshAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->QuathArray), writeSparsely);
}
        
static UsdAttribute
_CreateScalesAttr(UsdLightFieldParticleField_3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateScalesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3Array), writeSparsely);
}
        
static UsdAttribute
_CreateScaleshAttr(UsdLightFieldParticleField_3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateScaleshAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Half3Array), writeSparsely);
}
        
static UsdAttribute
_CreateOpacitiesAttr(UsdLightFieldParticleField_3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateOpacitiesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}
        
static UsdAttribute
_CreateOpacitieshAttr(UsdLightFieldParticleField_3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateOpacitieshAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->HalfArray), writeSparsely);
}
        
static UsdAttribute
_CreateRadianceSphericalHarmonicsDegreeAttr(UsdLightFieldParticleField_3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRadianceSphericalHarmonicsDegreeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreatePrimvarsRadianceSphericalHarmonicsCoefficientsAttr(UsdLightFieldParticleField_3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePrimvarsRadianceSphericalHarmonicsCoefficientsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3Array), writeSparsely);
}
        
static UsdAttribute
_CreatePrimvarsRadianceSphericalHarmonicsCoefficientshAttr(UsdLightFieldParticleField_3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePrimvarsRadianceSphericalHarmonicsCoefficientshAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Half3Array), writeSparsely);
}
} // anonymous namespace

void wrapUsdLightFieldParticleField_3DGaussianSplat()
{
    typedef UsdLightFieldParticleField_3DGaussianSplat This;

    class_<This, bases<UsdLightFieldParticleField> >
        cls("ParticleField_3DGaussianSplat");

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

        
        .def("GetProjectionModeHintAttr",
             &This::GetProjectionModeHintAttr)
        .def("CreateProjectionModeHintAttr",
             &_CreateProjectionModeHintAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetSortingModeHintAttr",
             &This::GetSortingModeHintAttr)
        .def("CreateSortingModeHintAttr",
             &_CreateSortingModeHintAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        
        .def("GetPositionsAttr",
             &This::GetPositionsAttr)
        .def("CreatePositionsAttr",
             &_CreatePositionsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPositionshAttr",
             &This::GetPositionshAttr)
        .def("CreatePositionshAttr",
             &_CreatePositionshAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        .def("PositionAttributeAPI", &This::PositionAttributeAPI)
        
        .def("GetOrientationsAttr",
             &This::GetOrientationsAttr)
        .def("CreateOrientationsAttr",
             &_CreateOrientationsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetOrientationshAttr",
             &This::GetOrientationshAttr)
        .def("CreateOrientationshAttr",
             &_CreateOrientationshAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        .def("OrientationAttributeAPI", &This::OrientationAttributeAPI)
        
        .def("GetScalesAttr",
             &This::GetScalesAttr)
        .def("CreateScalesAttr",
             &_CreateScalesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetScaleshAttr",
             &This::GetScaleshAttr)
        .def("CreateScaleshAttr",
             &_CreateScaleshAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        .def("ScaleAttributeAPI", &This::ScaleAttributeAPI)
        
        .def("GetOpacitiesAttr",
             &This::GetOpacitiesAttr)
        .def("CreateOpacitiesAttr",
             &_CreateOpacitiesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetOpacitieshAttr",
             &This::GetOpacitieshAttr)
        .def("CreateOpacitieshAttr",
             &_CreateOpacitieshAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        .def("OpacityAttributeAPI", &This::OpacityAttributeAPI)

        .def("GaussianShapeAPI", &This::GaussianShapeAPI)

        .def("GaussianFalloffFunctionAPI", &This::GaussianFalloffFunctionAPI)
        
        .def("GetRadianceSphericalHarmonicsDegreeAttr",
             &This::GetRadianceSphericalHarmonicsDegreeAttr)
        .def("CreateRadianceSphericalHarmonicsDegreeAttr",
             &_CreateRadianceSphericalHarmonicsDegreeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPrimvarsRadianceSphericalHarmonicsCoefficientsAttr",
             &This::GetPrimvarsRadianceSphericalHarmonicsCoefficientsAttr)
        .def("CreatePrimvarsRadianceSphericalHarmonicsCoefficientsAttr",
             &_CreatePrimvarsRadianceSphericalHarmonicsCoefficientsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPrimvarsRadianceSphericalHarmonicsCoefficientshAttr",
             &This::GetPrimvarsRadianceSphericalHarmonicsCoefficientshAttr)
        .def("CreatePrimvarsRadianceSphericalHarmonicsCoefficientshAttr",
             &_CreatePrimvarsRadianceSphericalHarmonicsCoefficientshAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        .def("SphericalHarmonicsAttributeAPI", &This::SphericalHarmonicsAttributeAPI)
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

WRAP_CUSTOM {
}

}
