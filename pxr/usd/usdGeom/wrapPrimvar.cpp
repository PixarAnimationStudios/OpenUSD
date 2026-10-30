//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/primvar.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>


#include <vector>

namespace nb = nanobind;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static nb::tuple
_GetDeclarationInfo(const UsdGeomPrimvar &self)
{
    TfToken name, interpolation;
    SdfValueTypeName typeName;
    int elementSize;
    self.GetDeclarationInfo(&name, &typeName, &interpolation, &elementSize);
    return nb::make_tuple(name, nb::cast(typeName), interpolation, elementSize);
}

static bool
_Set(const UsdGeomPrimvar &self, TfPyObjWrapper pyVal, UsdTimeCode time)
{
    VtValue val = UsdPythonToSdfType(pyVal, self.GetTypeName());
    return self.Set(val, time);
}

static TfPyObjWrapper
_Get(const UsdGeomPrimvar &self, UsdTimeCode time=UsdTimeCode::Default())
{
    VtValue retValue;
    self.Get(&retValue, time);
    return UsdVtValueToPython(retValue);
}    

static VtIntArray
_GetIndices(const UsdGeomPrimvar &self, UsdTimeCode time=UsdTimeCode::Default())
{
    VtIntArray indices;
    self.GetIndices(&indices, time);
    return indices;
}

static TfPyObjWrapper
_ComputeFlattened(const UsdGeomPrimvar &self, 
                  UsdTimeCode time=UsdTimeCode::Default())
{
    VtValue retValue;
    self.ComputeFlattened(&retValue, time);
    return UsdVtValueToPython(retValue);
}    

static vector<double>
_GetTimeSamples(const UsdGeomPrimvar &self) 
{
    vector<double> result;
    self.GetTimeSamples(&result);
    return result;
}

static vector<double>
_GetTimeSamplesInInterval(const UsdGeomPrimvar &self,
                          const GfInterval& interval) 
{
    vector<double> result;
    self.GetTimeSamplesInInterval(interval, &result);
    return result;
}

static size_t __hash__(const UsdGeomPrimvar &self) { return TfHash{}(self); }

// We override __getattribute__ for UsdGeomPrimvar to check object validity
// and raise an exception instead of crashing from Python.

// Store the original __getattribute__ so we can dispatch to it after verifying
// validity.
static TfStaticData<TfPyObjWrapper> _object__getattribute__;

// This function gets wrapped as __getattribute__ on UsdGeomPrimvar.
static nb::object
__getattribute__(nb::object selfObj, const char *name) {

    // Allow attribute lookups if the attribute name starts with '__', or
    // if the object's prim and attribute are both valid, or allow a few
    // methods if just the prim is valid, or an even smaller subset if neither
    // are valid.
    UsdGeomPrimvar &primvar = nb::cast<UsdGeomPrimvar &>(selfObj);
    if ((name[0] == '_' && name[1] == '_') ||
        // prim and attr are valid, let everything through.
        (primvar.GetAttr().IsValid() &&
         primvar.GetAttr().GetPrim().IsValid()) ||
        // prim is valid, but attr is invalid, let a few things through.
        (primvar.GetAttr().GetPrim().IsValid() &&
         (strcmp(name, "HasValue") == 0 ||
          strcmp(name, "HasAuthoredValue") == 0 ||
          strcmp(name, "GetName") == 0 ||
          strcmp(name, "GetPrimvarName") == 0 ||
          strcmp(name, "NameContainsNamespaces") == 0 ||
          strcmp(name, "GetBaseName") == 0 ||
          strcmp(name, "GetNamespace") == 0 ||
          strcmp(name, "SplitName") == 0)) ||
        // prim and attr are both invalid, let almost nothing through.
        strcmp(name, "IsDefined") == 0 ||
        strcmp(name, "GetAttr") == 0) {
        // Dispatch to object's __getattribute__.
        return (*_object__getattribute__).Get()(selfObj, name);
    } else {
        // Otherwise raise a runtime error.
        TfPyThrowRuntimeError(
            TfStringPrintf("Accessed invalid attribute as a primvar"));
    }
    // Unreachable.
    return nb::none();
}

} // anonymous namespace 

void wrapUsdGeomPrimvar(nanobind::module_ &m)
{
    typedef UsdGeomPrimvar Primvar;

    nb::class_<Primvar> clsObj(m, "Primvar");
    clsObj
        .def(nb::init<>())
        .def(nb::init<const UsdAttribute &>(), nb::arg("attr"))

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def("__bool__", [](const Primvar &self) { return bool(self); })
        .def("__hash__", __hash__)

        .def("GetInterpolation", &Primvar::GetInterpolation)
        .def("SetInterpolation", &Primvar::SetInterpolation,
             nb::arg("interpolation"))
        .def("HasAuthoredInterpolation", &Primvar::HasAuthoredInterpolation)

        .def("GetElementSize", &Primvar::GetElementSize)
        .def("SetElementSize", &Primvar::SetElementSize, nb::arg("eltSize"))
        .def("HasAuthoredElementSize", &Primvar::HasAuthoredElementSize)

        .def_static("IsPrimvar", &Primvar::IsPrimvar, nb::arg("attr"))

        .def_static("IsValidPrimvarName", &Primvar::IsValidPrimvarName, 
                    nb::arg("name"))
        
        .def_static("StripPrimvarsName", &Primvar::StripPrimvarsName, nb::arg("name"))

        .def_static("IsValidInterpolation", &Primvar::IsValidInterpolation,
                    nb::arg("interpolation"))

        .def("GetDeclarationInfo", _GetDeclarationInfo)
        .def("GetAttr", &Primvar::GetAttr,
             nb::rv_policy::copy)
        .def("IsDefined", &Primvar::IsDefined)
        .def("HasValue", &Primvar::HasValue)
        .def("HasAuthoredValue", &Primvar::HasAuthoredValue)
        .def("GetName", &Primvar::GetName,
             nb::rv_policy::copy)
        .def("GetPrimvarName", &Primvar::GetPrimvarName)
        .def("NameContainsNamespaces", &Primvar::NameContainsNamespaces)
        .def("GetBaseName", &Primvar::GetBaseName)
        .def("GetNamespace", &Primvar::GetNamespace)
        .def("SplitName", &Primvar::SplitName)
        .def("GetTypeName", &Primvar::GetTypeName)
        .def("Get", _Get, nb::arg("time") = UsdTimeCode::Default())
        .def("Set", _Set, nb::arg("value").none(), nb::arg("time") = UsdTimeCode::Default())

        .def("GetTimeSamples", _GetTimeSamples)
        .def("GetTimeSamplesInInterval", _GetTimeSamplesInInterval)
        .def("ValueMightBeTimeVarying", &Primvar::ValueMightBeTimeVarying)

        .def("SetIndices", &Primvar::SetIndices, 
             nb::arg("indices"),
             nb::arg("time") = UsdTimeCode::Default())
        .def("BlockIndices", &Primvar::BlockIndices)
        .def("GetIndices", _GetIndices, 
             nb::arg("time") = UsdTimeCode::Default())
        .def("GetIndicesAttr", &Primvar::GetIndicesAttr)
        .def("CreateIndicesAttr", &Primvar::CreateIndicesAttr)
        .def("IsIndexed", &Primvar::IsIndexed)

        .def("GetUnauthoredValuesIndex", &Primvar::GetUnauthoredValuesIndex)
        .def("SetUnauthoredValuesIndex", &Primvar::SetUnauthoredValuesIndex,
             nb::arg("unauthoredValuesIndex"))

        .def("ComputeFlattened", _ComputeFlattened,
             nb::arg("time") = UsdTimeCode::Default())
    
        .def("IsIdTarget", &Primvar::IsIdTarget)
        .def("SetIdTarget", &Primvar::SetIdTarget)
        ;

    nb::implicitly_convertible<Primvar, UsdAttribute>();

    // Save existing __getattribute__ and replace.
    *_object__getattribute__ = TfPyObjWrapper(clsObj.attr("__getattribute__"));
    clsObj.def("__getattribute__", __getattribute__);
}

