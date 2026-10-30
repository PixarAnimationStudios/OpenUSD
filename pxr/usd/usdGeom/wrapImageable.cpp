//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/imageable.h"
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
_CreateVisibilityAttr(UsdGeomImageable &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateVisibilityAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreatePurposeAttr(UsdGeomImageable &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreatePurposeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdGeomImageable &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.Imageable(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomImageable(nb::module_ &m)
{
    typedef UsdGeomImageable This;

    nb::class_<This, UsdTyped> cls(m, "Imageable");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetVisibilityAttr",
             &This::GetVisibilityAttr)
        .def("CreateVisibilityAttr",
             &_CreateVisibilityAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetPurposeAttr",
             &This::GetPurposeAttr)
        .def("CreatePurposeAttr",
             &_CreatePurposeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        
        .def("GetProxyPrimRel",
             &This::GetProxyPrimRel)
        .def("CreateProxyPrimRel",
             &This::CreateProxyPrimRel)
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

#include "pxr/base/tf/pyObjWrapper.h"

namespace {

static TfPyObjWrapper
_ComputeProxyPrim(UsdGeomImageable const &self)
{
    UsdPrim  renderPrim, proxyPrim;
    
    if (self){
        proxyPrim = self.ComputeProxyPrim(&renderPrim);
        if (proxyPrim){
            return TfPyObjWrapper(nb::make_tuple(proxyPrim, renderPrim));
        }
    }
    return TfPyObjWrapper();
}

static std::string _GetPurpose(const UsdGeomImageable::PurposeInfo &purposeInfo)
{
    return purposeInfo.purpose;
}

static void _SetPurpose(UsdGeomImageable::PurposeInfo &purposeInfo, 
                        const std::string &purpose)
{
    purposeInfo.purpose = TfToken(purpose);
}

static bool _IsValidPurposeInfo(const UsdGeomImageable::PurposeInfo &purposeInfo)
{
    return bool(purposeInfo);
}

WRAP_CUSTOM {

    _class
        .def_static("GetOrderedPurposeTokens",
             &UsdGeomImageable::GetOrderedPurposeTokens,
             nb::rv_policy::copy)

        .def("ComputeVisibility", 
             &UsdGeomImageable::ComputeVisibility,
             nb::arg("time")=UsdTimeCode::Default())

        .def("GetPurposeVisibilityAttr",
             &UsdGeomImageable::GetPurposeVisibilityAttr,
             (nb::arg("purpose") = UsdGeomTokens->default_))
        .def("ComputeEffectiveVisibility",
             &UsdGeomImageable::ComputeEffectiveVisibility,
             nb::arg("purpose") = UsdGeomTokens->default_,
              nb::arg("time") = UsdTimeCode::Default())

        .def("ComputePurpose", 
             (TfToken (UsdGeomImageable::*)() const)
                &UsdGeomImageable::ComputePurpose)
        .def("ComputePurposeInfo",
             (UsdGeomImageable::PurposeInfo (UsdGeomImageable::*)() const)
                &UsdGeomImageable::ComputePurposeInfo)
        .def("ComputePurposeInfo",
             (UsdGeomImageable::PurposeInfo (UsdGeomImageable::*)(
                const UsdGeomImageable::PurposeInfo &) const)
                &UsdGeomImageable::ComputePurposeInfo,
             nb::arg("parentPurposeInfo"))

        .def("ComputeProxyPrim", &_ComputeProxyPrim,
            "Returns None if neither this prim nor any of its ancestors "
            "has a valid renderProxy prim.  Otherwise, returns a tuple of "
            "(proxyPrim, renderPrimWithAuthoredProxyPrimRel)")
        .def("SetProxyPrim", 
             (bool (UsdGeomImageable::*)(const UsdPrim &) const)
             &UsdGeomImageable::SetProxyPrim, 
             nb::arg("proxy"))
        .def("SetProxyPrim", 
             (bool (UsdGeomImageable::*)(const UsdSchemaBase &) const)
             &UsdGeomImageable::SetProxyPrim, 
             nb::arg("proxy"))
        .def("MakeVisible", &UsdGeomImageable::MakeVisible, 
            nb::arg("time")=UsdTimeCode::Default())
        .def("MakeInvisible", &UsdGeomImageable::MakeInvisible, 
            nb::arg("time")=UsdTimeCode::Default())
        .def("ComputeWorldBound", &UsdGeomImageable::ComputeWorldBound,
             nb::arg("time"), nb::arg("purpose1")=TfToken(), nb::arg("purpose2")=TfToken(),
              nb::arg("purpose3")=TfToken(), nb::arg("purpose4")=TfToken())
        .def("ComputeLocalBound", &UsdGeomImageable::ComputeLocalBound,
             nb::arg("time"), nb::arg("purpose1")=TfToken(), nb::arg("purpose2")=TfToken(),
              nb::arg("purpose3")=TfToken(), nb::arg("purpose4")=TfToken())
        .def("ComputeUntransformedBound",
             &UsdGeomImageable::ComputeUntransformedBound,
             nb::arg("time"), nb::arg("purpose1")=TfToken(), nb::arg("purpose2")=TfToken(),
              nb::arg("purpose3")=TfToken(), nb::arg("purpose4")=TfToken())
        .def("ComputeLocalToWorldTransform",
             &UsdGeomImageable::ComputeLocalToWorldTransform, nb::arg("time"))
        .def("ComputeParentToWorldTransform",
             &UsdGeomImageable::ComputeParentToWorldTransform, nb::arg("time"))
        ;

        {
            nb::class_<UsdGeomImageable::PurposeInfo>(_class, "PurposeInfo")
                .def(nb::init<>())
                .def(nb::init<const TfToken &, bool>())
                .def("__bool__", &_IsValidPurposeInfo)
                .def("__eq__", [](UsdGeomImageable::PurposeInfo &self, const UsdGeomImageable::PurposeInfo &rhs) { return self == rhs; }, nb::is_operator())
                .def("__ne__", [](UsdGeomImageable::PurposeInfo &self, const UsdGeomImageable::PurposeInfo &rhs) { return self != rhs; }, nb::is_operator())
                .def_prop_rw("purpose", &_GetPurpose, &_SetPurpose)
                .def_rw("isInheritable",
                        &UsdGeomImageable::PurposeInfo::isInheritable)
                .def("GetInheritablePurpose", 
                     &UsdGeomImageable::PurposeInfo::GetInheritablePurpose,
                     nb::rv_policy::copy)
            ;
        }
}

} // anonymous namespace 
