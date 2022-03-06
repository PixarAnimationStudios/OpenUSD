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


PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdUsdGeomWrapPrimvar {

static boost::python::tuple
_GetDeclarationInfo(const UsdGeomPrimvar &self)
{
    TfToken name, interpolation;
    SdfValueTypeName typeName;
    int elementSize;
    self.GetDeclarationInfo(&name, &typeName, &interpolation, &elementSize);
    return boost::python::make_tuple(name, boost::python::object(typeName), interpolation, elementSize);
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

static std::vector<double>
_GetTimeSamples(const UsdGeomPrimvar &self) 
{
    std::vector<double> result;
    self.GetTimeSamples(&result);
    return result;
}

static std::vector<double>
_GetTimeSamplesInInterval(const UsdGeomPrimvar &self,
                          const GfInterval& interval) 
{
    std::vector<double> result;
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
static boost::python::object
__getattribute__(boost::python::object selfObj, const char *name) {

    // Allow attribute lookups if the attribute name starts with '__', or
    // if the object's prim and attribute are both valid, or allow a few
    // methods if just the prim is valid, or an even smaller subset if neither
    // are valid.
    if ((name[0] == '_' && name[1] == '_') ||
        // prim and attr are valid, let everything through.
        (boost::python::extract<UsdGeomPrimvar &>(selfObj)().GetAttr().IsValid() &&
         boost::python::extract<UsdGeomPrimvar &>(selfObj)().GetAttr().GetPrim().IsValid()) ||
        // prim is valid, but attr is invalid, let a few things through.
        (boost::python::extract<UsdGeomPrimvar &>(selfObj)().GetAttr().GetPrim().IsValid() &&
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
        return (*_object__getattribute__)(selfObj, name);
    } else {
        // Otherwise raise a runtime error.
        TfPyThrowRuntimeError(
            TfStringPrintf("Accessed invalid attribute as a primvar"));
    }
    // Unreachable.
    return boost::python::object();
}

} // anonymous namespace 

void wrapUsdGeomPrimvar()
{
    typedef UsdGeomPrimvar Primvar;

    boost::python::class_<Primvar> clsObj("Primvar");
    clsObj
        .def(boost::python::init<UsdAttribute>(boost::python::arg("attr")))

        .def(boost::python::self == boost::python::self)
        .def(boost::python::self != boost::python::self)
        .def(!boost::python::self)
        .def("__hash__", pxrUsdUsdGeomWrapPrimvar::__hash__)

        .def("GetInterpolation", &Primvar::GetInterpolation)
        .def("SetInterpolation", &Primvar::SetInterpolation,
             boost::python::arg("interpolation"))
        .def("HasAuthoredInterpolation", &Primvar::HasAuthoredInterpolation)

        .def("GetElementSize", &Primvar::GetElementSize)
        .def("SetElementSize", &Primvar::SetElementSize, boost::python::arg("eltSize"))
        .def("HasAuthoredElementSize", &Primvar::HasAuthoredElementSize)

        .def("IsPrimvar", Primvar::IsPrimvar, boost::python::arg("attr"))
        .staticmethod("IsPrimvar")

        .def("IsValidPrimvarName", Primvar::IsValidPrimvarName, 
                boost::python::arg("name"))
        .staticmethod("IsValidPrimvarName")
        
        .def("StripPrimvarsName", Primvar::StripPrimvarsName, boost::python::arg("name"))
        .staticmethod("StripPrimvarsName")

        .def("IsValidInterpolation", Primvar::IsValidInterpolation,
             boost::python::arg("interpolation"))
        .staticmethod("IsValidInterpolation")

        .def("GetDeclarationInfo", pxrUsdUsdGeomWrapPrimvar::_GetDeclarationInfo)
        .def("GetAttr", &Primvar::GetAttr,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("IsDefined", &Primvar::IsDefined)
        .def("HasValue", &Primvar::HasValue)
        .def("HasAuthoredValue", &Primvar::HasAuthoredValue)
        .def("GetName", &Primvar::GetName,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("GetPrimvarName", &Primvar::GetPrimvarName)
        .def("NameContainsNamespaces", &Primvar::NameContainsNamespaces)
        .def("GetBaseName", &Primvar::GetBaseName)
        .def("GetNamespace", &Primvar::GetNamespace)
        .def("SplitName", &Primvar::SplitName,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetTypeName", &Primvar::GetTypeName)
        .def("Get", pxrUsdUsdGeomWrapPrimvar::_Get, (boost::python::arg("time")=UsdTimeCode::Default()))
        .def("Set", pxrUsdUsdGeomWrapPrimvar::_Set, (boost::python::arg("value"), boost::python::arg("time")=UsdTimeCode::Default()))

        .def("GetTimeSamples", pxrUsdUsdGeomWrapPrimvar::_GetTimeSamples)
        .def("GetTimeSamplesInInterval", pxrUsdUsdGeomWrapPrimvar::_GetTimeSamplesInInterval)
        .def("ValueMightBeTimeVarying", &Primvar::ValueMightBeTimeVarying)

        .def("SetIndices", &Primvar::SetIndices, 
            (boost::python::arg("indices"),
             boost::python::arg("time")=UsdTimeCode::Default()))
        .def("BlockIndices", &Primvar::BlockIndices)
        .def("GetIndices", pxrUsdUsdGeomWrapPrimvar::_GetIndices, 
            (boost::python::arg("time")=UsdTimeCode::Default()))
        .def("GetIndicesAttr", &Primvar::GetIndicesAttr)
        .def("CreateIndicesAttr", &Primvar::GetIndicesAttr)
        .def("IsIndexed", &Primvar::IsIndexed)

        .def("GetUnauthoredValuesIndex", &Primvar::GetUnauthoredValuesIndex)
        .def("SetUnauthoredValuesIndex", &Primvar::SetUnauthoredValuesIndex,
            boost::python::arg("unauthoredValuesIndex"))

        .def("ComputeFlattened", pxrUsdUsdGeomWrapPrimvar::_ComputeFlattened,
            (boost::python::arg("time")=UsdTimeCode::Default()))
    
        .def("IsIdTarget", &Primvar::IsIdTarget)
        .def("SetIdTarget", &Primvar::SetIdTarget)
        ;

    TfPyRegisterStlSequencesFromPython<UsdGeomPrimvar>();
    boost::python::to_python_converter<std::vector<UsdGeomPrimvar>,
                        TfPySequenceToPython<std::vector<UsdGeomPrimvar>>>();
    boost::python::implicitly_convertible<Primvar, UsdAttribute>();

    // Save existing __getattribute__ and replace.
    *pxrUsdUsdGeomWrapPrimvar::_object__getattribute__ = boost::python::object(clsObj.attr("__getattribute__"));
    clsObj.def("__getattribute__", pxrUsdUsdGeomWrapPrimvar::__getattribute__);
}

