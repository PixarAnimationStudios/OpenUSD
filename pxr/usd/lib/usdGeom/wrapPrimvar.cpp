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

#include "pxr/usd/usd/conversions.h"

#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/implicit.hpp>

PXR_NAMESPACE_OPEN_SCOPE


using namespace boost::python;

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

void wrapUsdGeomPrimvar()
{
    typedef UsdGeomPrimvar Primvar;

    class_<Primvar>("Primvar")
        .def(init<UsdAttribute>(arg("attr")))
        .def(!self)

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
        .def("GetName", &Primvar::GetName,
             return_value_policy<return_by_value>())
        .def("GetBaseName", &Primvar::GetBaseName)
        .def("GetNamespace", &Primvar::GetNamespace)
        .def("SplitName", &Primvar::SplitName,
             return_value_policy<TfPySequenceToList>())
        .def("GetTypeName", &Primvar::GetTypeName)
        .def("Get", _Get, (arg("time")=UsdTimeCode::Default()))
        .def("Set", _Set, (arg("value"), arg("time")=UsdTimeCode::Default()))

        .def("SetIndices", &Primvar::SetIndices, 
            (arg("indices"),
             arg("time")=UsdTimeCode::Default()))
        .def("BlockIndices", &Primvar::BlockIndices)
        .def("GetIndices", _GetIndices, 
            (arg("time")=UsdTimeCode::Default()))
        .def("IsIndexed", &Primvar::IsIndexed)

        .def("GetUnauthoredValuesIndex", &Primvar::GetUnauthoredValuesIndex)
        .def("SetUnauthoredValuesIndex", &Primvar::SetUnauthoredValuesIndex,
            arg("unauthoredValuesIndex"))

        .def("ComputeFlattened", _ComputeFlattened,
            (arg("time")=UsdTimeCode::Default()))
    
        .def("IsIdTarget", &Primvar::IsIdTarget)
        .def("SetIdTarget", &Primvar::SetIdTarget)
        ;

    implicitly_convertible<Primvar, UsdAttribute>();
}

PXR_NAMESPACE_CLOSE_SCOPE

