//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdRi/materialAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
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
_CreateSurfaceAttr(UsdRiMaterialAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateSurfaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateDisplacementAttr(UsdRiMaterialAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateDisplacementAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateVolumeAttr(UsdRiMaterialAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateVolumeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdRiMaterialAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdRi.MaterialAPI(%s)",
        primRepr.c_str());
}

struct UsdRiMaterialAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdRiMaterialAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdRiMaterialAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdRiMaterialAPI::CanApply(prim, &whyNot);
    return UsdRiMaterialAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdRiMaterialAPI(nb::module_ &m)
{
    typedef UsdRiMaterialAPI This;

    UsdRiMaterialAPI_CanApplyResult::WrapNb<UsdRiMaterialAPI_CanApplyResult>(
        m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "MaterialAPI");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("CanApply", &_WrapCanApply, nb::arg("prim"))

        .def_static("Apply", &This::Apply, nb::arg("prim"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetSurfaceAttr",
             &This::GetSurfaceAttr)
        .def("CreateSurfaceAttr",
             &_CreateSurfaceAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetDisplacementAttr",
             &This::GetDisplacementAttr)
        .def("CreateDisplacementAttr",
             &_CreateDisplacementAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetVolumeAttr",
             &This::GetVolumeAttr)
        .def("CreateVolumeAttr",
             &_CreateVolumeAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)

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

WRAP_CUSTOM {
    typedef UsdRiMaterialAPI This;
    _class
        .def(nb::init<UsdShadeMaterial>(), nb::arg("material"))

        .def("GetSurface", &This::GetSurface, nb::arg("ignoreBaseMaterial")=false)
        .def("GetDisplacement", &This::GetDisplacement, 
             nb::arg("ignoreBaseMaterial")=false)
        .def("GetVolume", &This::GetVolume, nb::arg("ignoreBaseMaterial")=false)
 
        .def("GetSurfaceOutput", &This::GetSurfaceOutput)
        .def("GetDisplacementOutput", &This::GetDisplacementOutput)
        .def("GetVolumeOutput", &This::GetVolumeOutput)
 
        .def("SetSurfaceSource", &This::SetSurfaceSource)
        .def("SetDisplacementSource", &This::SetDisplacementSource)
        .def("SetVolumeSource", &This::SetVolumeSource)
 
        .def("ComputeInterfaceInputConsumersMap", 
            &This::ComputeInterfaceInputConsumersMap, 
            nb::arg("computeTransitiveConsumers")=false,
            nb::rv_policy::copy)
    ;
}

} // anonymous namespace
