//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLod/distanceHeuristic.h"
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
_CreateCenterAttr(UsdLodDistanceHeuristic &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCenterAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3f), writeSparsely);
}
        
static UsdAttribute
_CreateThresholdsAttr(UsdLodDistanceHeuristic &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateThresholdsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}
        
static UsdAttribute
_CreateBlendThresholdsAttr(UsdLodDistanceHeuristic &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateBlendThresholdsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}

static std::string
_Repr(const UsdLodDistanceHeuristic &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdLod.DistanceHeuristic(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdLodDistanceHeuristic()
{
    typedef UsdLodDistanceHeuristic This;

    class_<This, bases<UsdLodHeuristic> >
        cls("DistanceHeuristic");

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

        
        .def("GetCenterAttr",
             &This::GetCenterAttr)
        .def("CreateCenterAttr",
             &_CreateCenterAttr,
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
_WrapComputeDistance1(
    const UsdLodDistanceHeuristic& heuristic,
    const GfVec3d& viewpoint,
    const GfMatrix4d& transform,
    UsdTimeCode time)
{
    return heuristic.ComputeDistance(viewpoint, transform, time);
}

static
double
_WrapComputeDistance2(
    const UsdLodDistanceHeuristic& heuristic,
    const GfVec3d& viewpoint,
    const GfMatrix4d& transform,
    const double prevDistance,
    const double hysteresis,
    UsdTimeCode time)
{
    return heuristic.ComputeDistance(viewpoint, transform,
                                     prevDistance, hysteresis, time);
}

static
double
_WrapComputeLOD1(
    const UsdLodDistanceHeuristic& heuristic,
    double distance,
    UsdTimeCode time)
{
    return heuristic.ComputeLOD(distance, time);
}

static
double
_WrapComputeLOD2(
    const UsdLodDistanceHeuristic& heuristic,
    const GfVec3d& viewpoint,
    const GfMatrix4d& transform,
    UsdTimeCode time)
{
    return heuristic.ComputeLOD(viewpoint, transform, time);
}

static
pxr_boost::python::tuple
_WrapComputeLOD3(const UsdLodDistanceHeuristic& heuristic,
                 const GfVec3d& viewpoint,
                 const GfMatrix4d& transform,
                 const double prevDistance,
                 const double hysteresis,
                 UsdTimeCode time)
{
    double distanceOut;
    double lod = heuristic.ComputeLOD(viewpoint, transform,
                                      prevDistance, hysteresis,
                                      &distanceOut);
    return pxr_boost::python::make_tuple(lod, distanceOut);
}

WRAP_CUSTOM {
    typedef UsdLodDistanceHeuristic This;

    _class
        .def("CreateDistanceHeuristicQuery",
             &This::CreateDistanceHeuristicQuery,
             arg("time") = UsdTimeCode::Default())

        .def("ComputeDistance", _WrapComputeDistance1,
             (arg("viewpoint"),
              arg("transform"),
              arg("time") = UsdTimeCode::Default()))
        .def("ComputeDistance", _WrapComputeDistance2,
             (arg("viewpoint"),
              arg("transform"),
              arg("prevDistance") = 0.0,
              arg("hysteresis") = 0.0,
              arg("time") = UsdTimeCode::Default()))

        .def("ComputeLOD", &_WrapComputeLOD1,
             (arg("distance"),
              arg("time") = UsdTimeCode::Default()))
        .def("ComputeLOD", &_WrapComputeLOD2,
             (arg("viewpoint"),
              arg("transform"),
              arg("time") = UsdTimeCode::Default()))
        .def("ComputeLOD", &_WrapComputeLOD3,
             (arg("viewpoint"),
              arg("transform"),
              arg("prevDistance"),
              arg("hysteresis"),
              arg("time") = UsdTimeCode::Default()))
        ;
}

}
