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
#include "pxr/usd/usdGeom/xformOp.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyStaticTokens.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/implicit.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static bool
_Set(const UsdGeomXformOp &self, TfPyObjWrapper pyVal, UsdTimeCode time)
{
    VtValue val = UsdPythonToSdfType(pyVal, self.GetTypeName());
    return self.Set(val, time);
}

static TfPyObjWrapper
_Get(const UsdGeomXformOp &self, UsdTimeCode time)
{
    VtValue retValue;
    self.Get(&retValue, time);
    return UsdVtValueToPython(retValue);
}    

static std::vector<double>
_GetTimeSamples(const UsdGeomXformOp &self)
{
    std::vector<double> result;
    self.GetTimeSamples(&result);
    return result;
}

static std::vector<double>
_GetTimeSamplesInInterval(const UsdGeomXformOp &self,
                          const GfInterval &interval)
{
    std::vector<double> result;
    self.GetTimeSamplesInInterval(interval, &result);
    return result;
}

static GfMatrix4d
_GetOpTransform(const UsdGeomXformOp &self, UsdTimeCode time) 
{
    return self.GetOpTransform(time);
}

static TfToken
_GetOpName(const UsdGeomXformOp &self) 
{
    return self.GetOpName();
}

static TfToken
_GetOpTypeToken(const UsdGeomXformOp::Type &opType)
{
    return UsdGeomXformOp::GetOpTypeToken(opType);
}

static UsdGeomXformOp::Type
_GetOpTypeEnum(const TfToken& opTypeToken)
{
    return UsdGeomXformOp::GetOpTypeEnum(opTypeToken);
}

} // anonymous namespace 


// We override __getattribute__ for UsdGeomXformOp to check object validity
// and raise an exception instead of crashing from Python.
//
// Store the original __getattribute__ so we can dispatch to it after verifying
// validity
static TfStaticData<TfPyObjWrapper> _object__getattribute__;

// This function gets wrapped as __getattribute__ on UsdGeomXformOp.
static object
__getattribute__(object selfObj, const char *name) {
    // Allow attribute lookups if the attribute name starts with '__', or
    // if the object's prim and attribute are both valid, or allow a few
    // methods if just the prim is valid, or an even smaller subset if neighter
    // is valid.
    if ((name[0] == '_' && name[1] == '_') ||
        // prim and attr are valid, let everything through.
        (extract<UsdGeomXformOp &>(selfObj)().GetAttr().IsValid() &&
         extract<UsdGeomXformOp &>(selfObj)().GetAttr().GetPrim().IsValid()) ||
        // prim is valid, but attr is invalid, let a few things through.
        (extract<UsdGeomXformOp &>(selfObj)().GetAttr().GetPrim().IsValid() &&
         (strcmp(name, "GetName") == 0 ||
          strcmp(name, "GetBaseName") == 0 ||
          strcmp(name, "GetNamespace") == 0 ||
          strcmp(name, "SplitName") == 0)) ||
        // prim and attr are both invalid, let almost nothing through.
          strcmp(name, "IsDefined") == 0 ||
          strcmp(name, "GetOpTypeToken") == 0 ||
          strcmp(name, "GetOpTypeEnum") == 0 ||
          strcmp(name, "GetAttr") == 0) {
        // Dispatch to object's __getattribute__.
        return (*_object__getattribute__)(selfObj, name);
    } else {
        // Otherwise raise a runtime error.
        TfPyThrowRuntimeError(
            TfStringPrintf("Accessed schema on invalid prim"));
    }
    // Unreachable.
    return object();
}

void wrapUsdGeomXformOp()
{
    typedef UsdGeomXformOp XformOp;

    TF_PY_WRAP_PUBLIC_TOKENS("XformOpTypes", UsdGeomXformOpTypes,
                             USDGEOM_XFORM_OP_TYPES);

    class_<XformOp> cls("XformOp");
    scope s = cls
        .def(init<UsdAttribute, bool>(
                (arg("attr"), 
                 arg("isInverseOp")=false)))

        .def(!self)
        .def(self == self)
        .def(self != self)

        .def("GetAttr", &XformOp::GetAttr,
             return_value_policy<return_by_value>())

        .def("IsInverseOp", &XformOp::IsInverseOp)
        .def("IsDefined", &XformOp::IsDefined)
        .def("GetName", &XformOp::GetName,
             return_value_policy<return_by_value>())
        .def("GetBaseName", &XformOp::GetBaseName)
        .def("GetNamespace", &XformOp::GetNamespace)
        .def("SplitName", &XformOp::SplitName,
             return_value_policy<TfPySequenceToList>())
        .def("GetTypeName", &XformOp::GetTypeName)

        .def("Get", _Get, (arg("time")=UsdTimeCode::Default()))
        .def("Set", _Set, (arg("value"), arg("time")=UsdTimeCode::Default()))

        .def("GetTimeSamples", _GetTimeSamples,
            return_value_policy<TfPySequenceToList>())

        .def("GetTimeSamplesInInterval", _GetTimeSamplesInInterval,
            return_value_policy<TfPySequenceToList>())

        .def("GetNumTimeSamples", &XformOp::GetNumTimeSamples)

        .def("GetOpTransform", _GetOpTransform)
        .def("GetOpName", _GetOpName)

        .def("GetOpType", &XformOp::GetOpType)

        .def("GetPrecision", &XformOp::GetPrecision)

        .def("GetOpTypeToken", _GetOpTypeToken)
        .staticmethod("GetOpTypeToken")

        .def("GetOpTypeEnum", _GetOpTypeEnum)
        .staticmethod("GetOpTypeEnum")

        .def("MightBeTimeVarying", &XformOp::MightBeTimeVarying)
        ;

        TfPyWrapEnum<UsdGeomXformOp::Type>();
        TfPyWrapEnum<UsdGeomXformOp::Precision>();

    implicitly_convertible<XformOp, UsdAttribute>();
    implicitly_convertible<XformOp, UsdProperty>();
    implicitly_convertible<XformOp, UsdObject>();

    // Register to and from vector conversions.
    boost::python::to_python_converter<std::vector<XformOp >, 
        TfPySequenceToPython<std::vector<XformOp > > >();

    TfPyContainerConversions::from_python_sequence<std::vector<XformOp >,
        TfPyContainerConversions::variable_capacity_policy >();

    // Save existing __getattribute__ and replace.
    *_object__getattribute__ = object(cls.attr("__getattribute__"));
    cls.def("__getattribute__", __getattribute__);
}

