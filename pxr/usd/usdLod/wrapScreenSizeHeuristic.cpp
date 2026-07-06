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
_CreateExtentAttr(UsdLodScreenSizeHeuristic &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateExtentAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3Array), writeSparsely);
}
        
static UsdAttribute
_CreateProjectionMethodAttr(UsdLodScreenSizeHeuristic &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateProjectionMethodAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateThresholdsAttr(UsdLodScreenSizeHeuristic &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateThresholdsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}
        
static UsdAttribute
_CreateBlendThresholdsAttr(UsdLodScreenSizeHeuristic &self,
                                      object defaultVal, bool writeSparsely) {
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

void wrapUsdLodScreenSizeHeuristic()
{
    typedef UsdLodScreenSizeHeuristic This;

    class_<This, bases<UsdLodHeuristic> >
        cls("ScreenSizeHeuristic");

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

        
        .def("GetExtentAttr",
             &This::GetExtentAttr)
        .def("CreateExtentAttr",
             &_CreateExtentAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetProjectionMethodAttr",
             &This::GetProjectionMethodAttr)
        .def("CreateProjectionMethodAttr",
             &_CreateProjectionMethodAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetThresholdsAttr",
             &This::GetThresholdsAttr)
        .def("CreateThresholdsAttr",
             &_CreateThresholdsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetBlendThresholdsAttr",
             &This::GetBlendThresholdsAttr)
        .def("CreateBlendThresholdsAttr",
             &_CreateBlendThresholdsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        
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
pxr_boost::python::tuple
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
    return pxr_boost::python::make_tuple(lod, screenSizeOut);
}

WRAP_CUSTOM {
    typedef UsdLodScreenSizeHeuristic This;

    _class
        .def("CreateScreenSizeHeuristicQuery",
             &This::CreateScreenSizeHeuristicQuery,
             arg("time") = UsdTimeCode::Default())

        .def("ComputeScreenSize", _WrapComputeScreenSize1,
             (arg("frustum"),
              arg("transform"),
              arg("time") = UsdTimeCode::Default()))
        .def("ComputeScreenSize", _WrapComputeScreenSize2,
             (arg("frustum"),
              arg("transform"),
              arg("prevSize") = 0.0,
              arg("hysteresis") = 0.0,
              arg("time") = UsdTimeCode::Default()))

        .def("ComputeLOD", &_WrapComputeLOD1,
             (arg("screenSize"),
              arg("time") = UsdTimeCode::Default()))
        .def("ComputeLOD", &_WrapComputeLOD2,
             (arg("frustum"),
              arg("transform"),
              arg("time") = UsdTimeCode::Default()))
        .def("ComputeLOD", &_WrapComputeLOD3,
             (arg("frustum"),
              arg("transform"),
              arg("prevSize"),
              arg("hysteresis"),
              arg("time") = UsdTimeCode::Default()))
        ;
}

}
