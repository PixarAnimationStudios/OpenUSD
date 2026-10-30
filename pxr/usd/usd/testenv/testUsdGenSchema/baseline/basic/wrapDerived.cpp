//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/derived.h"
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

namespace nb = nanobind;

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreatePivotPositionAttr(UsdContrivedDerived &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreatePivotPositionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3), writeSparsely);
}
        
static UsdAttribute
_CreateMyVecfArrayAttr(UsdContrivedDerived &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateMyVecfArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3Array), writeSparsely);
}
        
static UsdAttribute
_CreateHoleIndicesAttr(UsdContrivedDerived &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateHoleIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateCornerIndicesAttr(UsdContrivedDerived &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateCornerIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateCornerSharpnessesAttr(UsdContrivedDerived &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateCornerSharpnessesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}
        
static UsdAttribute
_CreateCreaseLengthsAttr(UsdContrivedDerived &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateCreaseLengthsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateTransformAttr(UsdContrivedDerived &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateTransformAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4d), writeSparsely);
}
        
static UsdAttribute
_CreateTestingAssetAttr(UsdContrivedDerived &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateTestingAssetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->AssetArray), writeSparsely);
}
        
static UsdAttribute
_CreateNamespacedPropertyAttr(UsdContrivedDerived &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateNamespacedPropertyAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateJustDefaultAttr(UsdContrivedDerived &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateJustDefaultAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateOverrideBaseTrueDerivedFalseAttr(UsdContrivedDerived &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateOverrideBaseTrueDerivedFalseAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateOverrideBaseTrueDerivedNoneAttr(UsdContrivedDerived &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateOverrideBaseTrueDerivedNoneAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateOverrideBaseFalseDerivedFalseAttr(UsdContrivedDerived &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateOverrideBaseFalseDerivedFalseAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateOverrideBaseFalseDerivedNoneAttr(UsdContrivedDerived &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateOverrideBaseFalseDerivedNoneAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateOverrideBaseNoneDerivedFalseAttr(UsdContrivedDerived &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateOverrideBaseNoneDerivedFalseAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}

static std::string
_Repr(const UsdContrivedDerived &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdContrived.Derived(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdContrivedDerived(nb::module_ &m)
{
    typedef UsdContrivedDerived This;

    nb::class_<This, UsdContrivedBase>
        cls(m, "Derived");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("Define", &This::Define, nb::arg("stage"), nb::arg("path"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetPivotPositionAttr",
             &This::GetPivotPositionAttr)
        .def("CreatePivotPositionAttr",
             &_CreatePivotPositionAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetMyVecfArrayAttr",
             &This::GetMyVecfArrayAttr)
        .def("CreateMyVecfArrayAttr",
             &_CreateMyVecfArrayAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetHoleIndicesAttr",
             &This::GetHoleIndicesAttr)
        .def("CreateHoleIndicesAttr",
             &_CreateHoleIndicesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetCornerIndicesAttr",
             &This::GetCornerIndicesAttr)
        .def("CreateCornerIndicesAttr",
             &_CreateCornerIndicesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetCornerSharpnessesAttr",
             &This::GetCornerSharpnessesAttr)
        .def("CreateCornerSharpnessesAttr",
             &_CreateCornerSharpnessesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetCreaseLengthsAttr",
             &This::GetCreaseLengthsAttr)
        .def("CreateCreaseLengthsAttr",
             &_CreateCreaseLengthsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetTransformAttr",
             &This::GetTransformAttr)
        .def("CreateTransformAttr",
             &_CreateTransformAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetTestingAssetAttr",
             &This::GetTestingAssetAttr)
        .def("CreateTestingAssetAttr",
             &_CreateTestingAssetAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetNamespacedPropertyAttr",
             &This::GetNamespacedPropertyAttr)
        .def("CreateNamespacedPropertyAttr",
             &_CreateNamespacedPropertyAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetJustDefaultAttr",
             &This::GetJustDefaultAttr)
        .def("CreateJustDefaultAttr",
             &_CreateJustDefaultAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetOverrideBaseTrueDerivedFalseAttr",
             &This::GetOverrideBaseTrueDerivedFalseAttr)
        .def("CreateOverrideBaseTrueDerivedFalseAttr",
             &_CreateOverrideBaseTrueDerivedFalseAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetOverrideBaseTrueDerivedNoneAttr",
             &This::GetOverrideBaseTrueDerivedNoneAttr)
        .def("CreateOverrideBaseTrueDerivedNoneAttr",
             &_CreateOverrideBaseTrueDerivedNoneAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetOverrideBaseFalseDerivedFalseAttr",
             &This::GetOverrideBaseFalseDerivedFalseAttr)
        .def("CreateOverrideBaseFalseDerivedFalseAttr",
             &_CreateOverrideBaseFalseDerivedFalseAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetOverrideBaseFalseDerivedNoneAttr",
             &This::GetOverrideBaseFalseDerivedNoneAttr)
        .def("CreateOverrideBaseFalseDerivedNoneAttr",
             &_CreateOverrideBaseFalseDerivedNoneAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetOverrideBaseNoneDerivedFalseAttr",
             &This::GetOverrideBaseNoneDerivedFalseAttr)
        .def("CreateOverrideBaseNoneDerivedFalseAttr",
             &_CreateOverrideBaseNoneDerivedFalseAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        
        .def("GetBindingRel",
             &This::GetBindingRel)
        .def("CreateBindingRel",
             &This::CreateBindingRel)
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
}

}
