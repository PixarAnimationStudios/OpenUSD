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
#include "pxr/usd/usdGeom/primvar.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/implicit.hpp>

#include <vector>

using namespace boost::python;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static tuple
_GetDeclarationInfo(const UsdGeomPrimvar &self)
{
    TfToken name, interpolation;
    SdfValueTypeName typeName;
    int elementSize;
    self.GetDeclarationInfo(&name, &typeName, &interpolation, &elementSize);
    return make_tuple(name, object(typeName), interpolation, elementSize);
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

static size_t __hash__(const UsdGeomPrimvar &self) { return hash_value(self); }

// We override __getattribute__ for UsdGeomPrimvar to check object validity
// and raise an exception instead of crashing from Python.

// Store the original __getattribute__ so we can dispatch to it after verifying
// validity.
static TfStaticData<TfPyObjWrapper> _object__getattribute__;

// This function gets wrapped as __getattribute__ on UsdGeomPrimvar.
static object
__getattribute__(object selfObj, const char *name) {

    // Allow attribute lookups if the attribute name starts with '__', or
    // if the object's prim and attribute are both valid, or whitelist a few
    // methods if just the prim is valid, or an even smaller subset if neither
    // are valid.
    if ((name[0] == '_' && name[1] == '_') ||
        // prim and attr are valid, let everything through.
        (extract<UsdGeomPrimvar &>(selfObj)().GetAttr().IsValid() &&
         extract<UsdGeomPrimvar &>(selfObj)().GetAttr().GetPrim().IsValid()) ||
        // prim is valid, but attr is invalid, let a few things through.
        (extract<UsdGeomPrimvar &>(selfObj)().GetAttr().GetPrim().IsValid() &&
         (strcmp(name, "IsDefined") == 0 ||
          strcmp(name, "HasValue") == 0 ||
          strcmp(name, "HasAuthoredValue") == 0 ||
          strcmp(name, "GetName") == 0 ||
          strcmp(name, "GetPrimvarName") == 0 ||
          strcmp(name, "NameContainsNamespaces") == 0 ||
          strcmp(name, "GetBaseName") == 0 ||
          strcmp(name, "GetNamespace") == 0 ||
          strcmp(name, "SplitName") == 0)) ||
        // prim and attr are both invalid, let almost nothing through.
        strcmp(name, "GetAttr") == 0) {
        // Dispatch to object's __getattribute__.
        return (*_object__getattribute__)(selfObj, name);
    } else {
        // Otherwise raise a runtime error.
        TfPyThrowRuntimeError(
            TfStringPrintf("Accessed invalid attribute as a primvar"));
    }
    // Unreachable.
    return object();
}

} // anonymous namespace 

void wrapUsdGeomPrimvar()
{
    typedef UsdGeomPrimvar Primvar;

    class_<Primvar> clsObj("Primvar");
    clsObj
        .def(init<UsdAttribute>(arg("attr")))

        .def(self == self)
        .def(self != self)
        .def(!self)
        .def("__hash__", __hash__)

        .def("GetInterpolation", &Primvar::GetInterpolation)
        .def("SetInterpolation", &Primvar::SetInterpolation,
             arg("interpolation"))
        .def("HasAuthoredInterpolation", &Primvar::HasAuthoredInterpolation)

        .def("GetElementSize", &Primvar::GetElementSize)
        .def("SetElementSize", &Primvar::SetElementSize, arg("eltSize"))
        .def("HasAuthoredElementSize", &Primvar::HasAuthoredElementSize)

        .def("IsPrimvar", Primvar::IsPrimvar, arg("attr"))
        .staticmethod("IsPrimvar")

        .def("IsValidInterpolation", Primvar::IsValidInterpolation,
             arg("interpolation"))
        .staticmethod("IsValidInterpolation")

        .def("GetDeclarationInfo", _GetDeclarationInfo)
        .def("GetAttr", &Primvar::GetAttr,
             return_value_policy<return_by_value>())
        .def("IsDefined", &Primvar::IsDefined)
        .def("HasValue", &Primvar::HasValue)
        .def("HasAuthoredValue", &Primvar::HasAuthoredValue)
        .def("GetName", &Primvar::GetName,
             return_value_policy<return_by_value>())
        .def("GetPrimvarName", &Primvar::GetPrimvarName)
        .def("NameContainsNamespaces", &Primvar::NameContainsNamespaces)
        .def("GetBaseName", &Primvar::GetBaseName)
        .def("GetNamespace", &Primvar::GetNamespace)
        .def("SplitName", &Primvar::SplitName,
             return_value_policy<TfPySequenceToList>())
        .def("GetTypeName", &Primvar::GetTypeName)
        .def("Get", _Get, (arg("time")=UsdTimeCode::Default()))
        .def("Set", _Set, (arg("value"), arg("time")=UsdTimeCode::Default()))

        .def("GetTimeSamples", _GetTimeSamples)
        .def("GetTimeSamplesInInterval", _GetTimeSamplesInInterval)
        .def("ValueMightBeTimeVarying", &Primvar::ValueMightBeTimeVarying)

        .def("SetIndices", &Primvar::SetIndices, 
            (arg("indices"),
             arg("time")=UsdTimeCode::Default()))
        .def("BlockIndices", &Primvar::BlockIndices)
        .def("GetIndices", _GetIndices, 
            (arg("time")=UsdTimeCode::Default()))
        .def("GetIndicesAttr", &Primvar::GetIndicesAttr)
        .def("CreateIndicesAttr", &Primvar::GetIndicesAttr)
        .def("IsIndexed", &Primvar::IsIndexed)

        .def("GetUnauthoredValuesIndex", &Primvar::GetUnauthoredValuesIndex)
        .def("SetUnauthoredValuesIndex", &Primvar::SetUnauthoredValuesIndex,
            arg("unauthoredValuesIndex"))

        .def("ComputeFlattened", _ComputeFlattened,
            (arg("time")=UsdTimeCode::Default()))
    
        .def("IsIdTarget", &Primvar::IsIdTarget)
        .def("SetIdTarget", &Primvar::SetIdTarget)
        ;

    TfPyRegisterStlSequencesFromPython<UsdGeomPrimvar>();
    to_python_converter<std::vector<UsdGeomPrimvar>,
                        TfPySequenceToPython<std::vector<UsdGeomPrimvar>>>();
    implicitly_convertible<Primvar, UsdAttribute>();

    // Save existing __getattribute__ and replace.
    *_object__getattribute__ = object(clsObj.attr("__getattribute__"));
    clsObj.def("__getattribute__", __getattribute__);
}

