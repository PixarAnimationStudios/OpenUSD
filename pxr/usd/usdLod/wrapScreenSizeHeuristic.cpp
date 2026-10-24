//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLod/screenSizeHeuristic.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateExtentAttr(UsdLodScreenSizeHeuristic &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateExtentAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3Array), writeSparsely);
}
        
static UsdAttribute
_CreateProjectionMethodAttr(UsdLodScreenSizeHeuristic &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateProjectionMethodAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateThresholdsAttr(UsdLodScreenSizeHeuristic &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateThresholdsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}
        
static UsdAttribute
_CreateBlendThresholdsAttr(UsdLodScreenSizeHeuristic &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateBlendThresholdsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}

static std::string
_Repr(const UsdLodScreenSizeHeuristic &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdLod.ScreenSizeHeuristic(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdLodScreenSizeHeuristic(nb::module_ &m)
{
    typedef UsdLodScreenSizeHeuristic This;

    nb::class_<This, UsdLodHeuristic> cls(m, "ScreenSizeHeuristic");

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

        
        .def("GetExtentAttr",
             &This::GetExtentAttr)
        .def("CreateExtentAttr",
             &_CreateExtentAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetProjectionMethodAttr",
             &This::GetProjectionMethodAttr)
        .def("CreateProjectionMethodAttr",
             &_CreateProjectionMethodAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetThresholdsAttr",
             &This::GetThresholdsAttr)
        .def("CreateThresholdsAttr",
             &_CreateThresholdsAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetBlendThresholdsAttr",
             &This::GetBlendThresholdsAttr)
        .def("CreateBlendThresholdsAttr",
             &_CreateBlendThresholdsAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)

        
        .def("GetBoundingVolumeRel",
             &This::GetBoundingVolumeRel)
        .def("CreateBoundingVolumeRel",
             &This::CreateBoundingVolumeRel)
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

static
double
_WrapComputeScreenSize1(
    const UsdLodScreenSizeHeuristic& heuristic,
    const GfFrustum& frustum,
    const GfMatrix4d& transform,
    UsdTimeCode time)
{
    return heuristic.ComputeScreenSize(frustum, transform, time);
}

static
double
_WrapComputeScreenSize2(
    const UsdLodScreenSizeHeuristic& heuristic,
    const GfFrustum& frustum,
    const GfMatrix4d& transform,
    const double prevSize,
    const double hysteresis,
    UsdTimeCode time)
{
    return heuristic.ComputeScreenSize(frustum, transform,
                                       prevSize, hysteresis, time);
}

static
double
_WrapComputeLOD1(
    const UsdLodScreenSizeHeuristic& heuristic,
    double screenSize,
    UsdTimeCode time)
{
    return heuristic.ComputeLOD(screenSize, time);
}

static
double
_WrapComputeLOD2(
    const UsdLodScreenSizeHeuristic& heuristic,
    const GfFrustum& frustum,
    const GfMatrix4d& transform,
    UsdTimeCode time)
{
    return heuristic.ComputeLOD(frustum, transform, time);
}

static
nb::tuple
_WrapComputeLOD3(const UsdLodScreenSizeHeuristic& heuristic,
                 const GfFrustum& frustum,
                 const GfMatrix4d& transform,
                 const double prevSize,
                 const double hysteresis,
                 UsdTimeCode time)
{
    double screenSizeOut;
    double lod = heuristic.ComputeLOD(frustum, transform,
                                      prevSize, hysteresis,
                                      &screenSizeOut);
    return nb::make_tuple(lod, screenSizeOut);
}

WRAP_CUSTOM {
    typedef UsdLodScreenSizeHeuristic This;

    _class
        .def("CreateScreenSizeHeuristicQuery",
             &This::CreateScreenSizeHeuristicQuery,
             nb::arg("time") = UsdTimeCode::Default())

        .def("ComputeScreenSize", _WrapComputeScreenSize1,
             nb::arg("frustum"),
             nb::arg("transform"),
             nb::arg("time") = UsdTimeCode::Default())
        .def("ComputeScreenSize", _WrapComputeScreenSize2,
             nb::arg("frustum"),
             nb::arg("transform"),
             nb::arg("prevSize") = 0.0,
             nb::arg("hysteresis") = 0.0,
             nb::arg("time") = UsdTimeCode::Default())

        .def("ComputeLOD", &_WrapComputeLOD1,
             nb::arg("screenSize"),
             nb::arg("time") = UsdTimeCode::Default())
        .def("ComputeLOD", &_WrapComputeLOD2,
             nb::arg("frustum"),
             nb::arg("transform"),
             nb::arg("time") = UsdTimeCode::Default())
        .def("ComputeLOD", &_WrapComputeLOD3,
             nb::arg("frustum"),
             nb::arg("transform"),
             nb::arg("prevSize"),
             nb::arg("hysteresis"),
             nb::arg("time") = UsdTimeCode::Default())
        ;
}

}
