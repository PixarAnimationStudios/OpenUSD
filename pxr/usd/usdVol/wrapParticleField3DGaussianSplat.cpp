//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/particleField3DGaussianSplat.h"
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
_CreateProjectionModeHintAttr(UsdVolParticleField3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateProjectionModeHintAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateSortingModeHintAttr(UsdVolParticleField3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateSortingModeHintAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdVolParticleField3DGaussianSplat &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdVol.ParticleField3DGaussianSplat(%s)",
        primRepr.c_str());
}

        
static UsdAttribute
_CreatePositionsAttr(UsdVolParticleField3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePositionsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3fArray), writeSparsely);
}
        
static UsdAttribute
_CreatePositionshAttr(UsdVolParticleField3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePositionshAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3hArray), writeSparsely);
}
        
static UsdAttribute
_CreateOrientationsAttr(UsdVolParticleField3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateOrientationsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->QuatfArray), writeSparsely);
}
        
static UsdAttribute
_CreateOrientationshAttr(UsdVolParticleField3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateOrientationshAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->QuathArray), writeSparsely);
}
        
static UsdAttribute
_CreateScalesAttr(UsdVolParticleField3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateScalesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3Array), writeSparsely);
}
        
static UsdAttribute
_CreateScaleshAttr(UsdVolParticleField3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateScaleshAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Half3Array), writeSparsely);
}
        
static UsdAttribute
_CreateOpacitiesAttr(UsdVolParticleField3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateOpacitiesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}
        
static UsdAttribute
_CreateOpacitieshAttr(UsdVolParticleField3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateOpacitieshAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->HalfArray), writeSparsely);
}
        
static UsdAttribute
_CreateRadianceSphericalHarmonicsDegreeAttr(UsdVolParticleField3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRadianceSphericalHarmonicsDegreeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateRadianceSphericalHarmonicsCoefficientsAttr(UsdVolParticleField3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRadianceSphericalHarmonicsCoefficientsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3Array), writeSparsely);
}
        
static UsdAttribute
_CreateRadianceSphericalHarmonicsCoefficientshAttr(UsdVolParticleField3DGaussianSplat &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRadianceSphericalHarmonicsCoefficientshAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Half3Array), writeSparsely);
}
} // anonymous namespace

void wrapUsdVolParticleField3DGaussianSplat()
{
    typedef UsdVolParticleField3DGaussianSplat This;

    class_<This, bases<UsdVolParticleField> >
        cls("ParticleField3DGaussianSplat");

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

        .def("ParticleFieldPositionAttributeAPI", &This::ParticleFieldPositionAttributeAPI)
        
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

        .def("ParticleFieldOrientationAttributeAPI", &This::ParticleFieldOrientationAttributeAPI)
        
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

        .def("ParticleFieldScaleAttributeAPI", &This::ParticleFieldScaleAttributeAPI)
        
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

        .def("ParticleFieldOpacityAttributeAPI", &This::ParticleFieldOpacityAttributeAPI)

        .def("ParticleFieldKernelGaussianEllipsoidAPI", &This::ParticleFieldKernelGaussianEllipsoidAPI)
        
        .def("GetRadianceSphericalHarmonicsDegreeAttr",
             &This::GetRadianceSphericalHarmonicsDegreeAttr)
        .def("CreateRadianceSphericalHarmonicsDegreeAttr",
             &_CreateRadianceSphericalHarmonicsDegreeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRadianceSphericalHarmonicsCoefficientsAttr",
             &This::GetRadianceSphericalHarmonicsCoefficientsAttr)
        .def("CreateRadianceSphericalHarmonicsCoefficientsAttr",
             &_CreateRadianceSphericalHarmonicsCoefficientsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRadianceSphericalHarmonicsCoefficientshAttr",
             &This::GetRadianceSphericalHarmonicsCoefficientshAttr)
        .def("CreateRadianceSphericalHarmonicsCoefficientshAttr",
             &_CreateRadianceSphericalHarmonicsCoefficientshAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        .def("ParticleFieldSphericalHarmonicsAttributeAPI", &This::ParticleFieldSphericalHarmonicsAttributeAPI)
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
