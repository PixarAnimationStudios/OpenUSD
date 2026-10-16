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
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/wrapTypeHelpers.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/tuple.h>

namespace nb = nanobind;

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateProjectionModeHintAttr(UsdVolParticleField3DGaussianSplat &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateProjectionModeHintAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateSortingModeHintAttr(UsdVolParticleField3DGaussianSplat &self,
                                      nb::handle defaultVal, bool writeSparsely) {
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
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreatePositionsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3fArray), writeSparsely);
}
        
static UsdAttribute
_CreatePositionshAttr(UsdVolParticleField3DGaussianSplat &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreatePositionshAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3hArray), writeSparsely);
}
        
static UsdAttribute
_CreateOrientationsAttr(UsdVolParticleField3DGaussianSplat &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateOrientationsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->QuatfArray), writeSparsely);
}
        
static UsdAttribute
_CreateOrientationshAttr(UsdVolParticleField3DGaussianSplat &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateOrientationshAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->QuathArray), writeSparsely);
}
        
static UsdAttribute
_CreateScalesAttr(UsdVolParticleField3DGaussianSplat &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateScalesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3Array), writeSparsely);
}
        
static UsdAttribute
_CreateScaleshAttr(UsdVolParticleField3DGaussianSplat &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateScaleshAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Half3Array), writeSparsely);
}
        
static UsdAttribute
_CreateOpacitiesAttr(UsdVolParticleField3DGaussianSplat &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateOpacitiesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}
        
static UsdAttribute
_CreateOpacitieshAttr(UsdVolParticleField3DGaussianSplat &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateOpacitieshAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->HalfArray), writeSparsely);
}
        
static UsdAttribute
_CreateRadianceSphericalHarmonicsDegreeAttr(UsdVolParticleField3DGaussianSplat &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateRadianceSphericalHarmonicsDegreeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateRadianceSphericalHarmonicsCoefficientsAttr(UsdVolParticleField3DGaussianSplat &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateRadianceSphericalHarmonicsCoefficientsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3Array), writeSparsely);
}
        
static UsdAttribute
_CreateRadianceSphericalHarmonicsCoefficientshAttr(UsdVolParticleField3DGaussianSplat &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateRadianceSphericalHarmonicsCoefficientshAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Half3Array), writeSparsely);
}
} // anonymous namespace

void wrapUsdVolParticleField3DGaussianSplat(nb::module_ &m)
{
    typedef UsdVolParticleField3DGaussianSplat This;

    nb::class_<This, UsdVolParticleField> cls(m, "ParticleField3DGaussianSplat");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("Define", &This::Define, nb::arg("stage"), nb::arg("path"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetProjectionModeHintAttr",
             &This::GetProjectionModeHintAttr)
        .def("CreateProjectionModeHintAttr",
             &_CreateProjectionModeHintAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetSortingModeHintAttr",
             &This::GetSortingModeHintAttr)
        .def("CreateSortingModeHintAttr",
             &_CreateSortingModeHintAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        
        .def("GetPositionsAttr",
             &This::GetPositionsAttr)
        .def("CreatePositionsAttr",
             &_CreatePositionsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetPositionshAttr",
             &This::GetPositionshAttr)
        .def("CreatePositionshAttr",
             &_CreatePositionshAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        .def("ParticleFieldPositionAttributeAPI", &This::ParticleFieldPositionAttributeAPI)
        
        .def("GetOrientationsAttr",
             &This::GetOrientationsAttr)
        .def("CreateOrientationsAttr",
             &_CreateOrientationsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetOrientationshAttr",
             &This::GetOrientationshAttr)
        .def("CreateOrientationshAttr",
             &_CreateOrientationshAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        .def("ParticleFieldOrientationAttributeAPI", &This::ParticleFieldOrientationAttributeAPI)
        
        .def("GetScalesAttr",
             &This::GetScalesAttr)
        .def("CreateScalesAttr",
             &_CreateScalesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetScaleshAttr",
             &This::GetScaleshAttr)
        .def("CreateScaleshAttr",
             &_CreateScaleshAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        .def("ParticleFieldScaleAttributeAPI", &This::ParticleFieldScaleAttributeAPI)
        
        .def("GetOpacitiesAttr",
             &This::GetOpacitiesAttr)
        .def("CreateOpacitiesAttr",
             &_CreateOpacitiesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetOpacitieshAttr",
             &This::GetOpacitieshAttr)
        .def("CreateOpacitieshAttr",
             &_CreateOpacitieshAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        .def("ParticleFieldOpacityAttributeAPI", &This::ParticleFieldOpacityAttributeAPI)

        .def("ParticleFieldKernelGaussianEllipsoidAPI", &This::ParticleFieldKernelGaussianEllipsoidAPI)
        
        .def("GetRadianceSphericalHarmonicsDegreeAttr",
             &This::GetRadianceSphericalHarmonicsDegreeAttr)
        .def("CreateRadianceSphericalHarmonicsDegreeAttr",
             &_CreateRadianceSphericalHarmonicsDegreeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetRadianceSphericalHarmonicsCoefficientsAttr",
             &This::GetRadianceSphericalHarmonicsCoefficientsAttr)
        .def("CreateRadianceSphericalHarmonicsCoefficientsAttr",
             &_CreateRadianceSphericalHarmonicsCoefficientsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetRadianceSphericalHarmonicsCoefficientshAttr",
             &This::GetRadianceSphericalHarmonicsCoefficientshAttr)
        .def("CreateRadianceSphericalHarmonicsCoefficientshAttr",
             &_CreateRadianceSphericalHarmonicsCoefficientshAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

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
