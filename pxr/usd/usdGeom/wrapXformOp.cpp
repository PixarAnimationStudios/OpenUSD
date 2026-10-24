//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/xformOp.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;
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
static nb::object
__getattribute__(nb::object selfObj, const char *name) {
    // Allow attribute lookups if the attribute name starts with '__', or
    // if the object's prim and attribute are both valid, or allow a few
    // methods if just the prim is valid, or an even smaller subset if neighter
    // is valid.
    UsdGeomXformOp &xformOp = nb::cast<UsdGeomXformOp &>(selfObj);
    if ((name[0] == '_' && name[1] == '_') ||
        // prim and attr are valid, let everything through.
        (xformOp.GetAttr().IsValid() &&
         xformOp.GetAttr().GetPrim().IsValid()) ||
        // prim is valid, but attr is invalid, let a few things through.
        (xformOp.GetAttr().GetPrim().IsValid() &&
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
        return (*_object__getattribute__).Get()(selfObj, name);
    } else {
        // Otherwise raise a runtime error.
        TfPyThrowRuntimeError(
            TfStringPrintf("Accessed schema on invalid prim"));
    }
    // Unreachable.
    return nb::none();
}

void wrapUsdGeomXformOp(nanobind::module_ &m)
{
    typedef UsdGeomXformOp XformOp;

    TF_NB_WRAP_PUBLIC_TOKENS(m, "XformOpTypes", UsdGeomXformOpTypes,
                             USDGEOM_XFORM_OP_TYPES);

    nb::class_<XformOp> cls(m, "XformOp");
    TfNbWrapEnum<UsdGeomXformOp::Type>(cls);
    TfNbWrapEnum<UsdGeomXformOp::Precision>(cls);
    cls
        .def(nb::init<>())
        .def(nb::init<const UsdAttribute &, bool>(),
             nb::arg("attr"), 
             nb::arg("isInverseOp") = false)

        .def("__bool__", [](const XformOp &self) { return bool(self); })
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)

        .def("GetAttr", &XformOp::GetAttr,
             nb::rv_policy::copy)

        .def("IsInverseOp", &XformOp::IsInverseOp)
        .def("IsDefined", &XformOp::IsDefined)
        .def("GetName", &XformOp::GetName,
             nb::rv_policy::copy)
        .def("GetBaseName", &XformOp::GetBaseName)
        .def("GetNamespace", &XformOp::GetNamespace)
        .def("SplitName", &XformOp::SplitName)
        .def("GetTypeName", &XformOp::GetTypeName)

        .def("Get", _Get, nb::arg("time") = UsdTimeCode::Default())
        .def("Set", _Set, nb::arg("value").none(), nb::arg("time") = UsdTimeCode::Default())

        .def("GetTimeSamples", _GetTimeSamples)

        .def("GetTimeSamplesInInterval", _GetTimeSamplesInInterval)

        .def("GetNumTimeSamples", &XformOp::GetNumTimeSamples)

        .def("GetOpTransform", _GetOpTransform)
        .def("GetOpName", _GetOpName)

        .def("GetOpType", &XformOp::GetOpType)

        .def("GetPrecision", &XformOp::GetPrecision)

        .def_static("GetOpTypeToken", _GetOpTypeToken)

        .def_static("GetOpTypeEnum", _GetOpTypeEnum)

        .def("MightBeTimeVarying", &XformOp::MightBeTimeVarying)
        ;

    nb::implicitly_convertible<XformOp, UsdAttribute>();
    nb::implicitly_convertible<XformOp, UsdProperty>();
    nb::implicitly_convertible<XformOp, UsdObject>();

    // Save existing __getattribute__ and replace.
    *_object__getattribute__ = TfPyObjWrapper(cls.attr("__getattribute__"));
    cls.def("__getattribute__", __getattribute__);
}

