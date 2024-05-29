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
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreatePivotPositionAttr(UsdContrivedDerived &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePivotPositionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3), writeSparsely);
}
        
static UsdAttribute
_CreateMyVecfArrayAttr(UsdContrivedDerived &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMyVecfArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3Array), writeSparsely);
}
        
static UsdAttribute
_CreateHoleIndicesAttr(UsdContrivedDerived &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateHoleIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateCornerIndicesAttr(UsdContrivedDerived &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCornerIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateCornerSharpnessesAttr(UsdContrivedDerived &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCornerSharpnessesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}
        
static UsdAttribute
_CreateCreaseLengthsAttr(UsdContrivedDerived &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCreaseLengthsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateTransformAttr(UsdContrivedDerived &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTransformAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4d), writeSparsely);
}
        
static UsdAttribute
_CreateTestingAssetAttr(UsdContrivedDerived &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTestingAssetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->AssetArray), writeSparsely);
}
        
static UsdAttribute
_CreateNamespacedPropertyAttr(UsdContrivedDerived &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateNamespacedPropertyAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateJustDefaultAttr(UsdContrivedDerived &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateJustDefaultAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateOverrideBaseTrueDerivedFalseAttr(UsdContrivedDerived &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateOverrideBaseTrueDerivedFalseAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateOverrideBaseTrueDerivedNoneAttr(UsdContrivedDerived &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateOverrideBaseTrueDerivedNoneAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateOverrideBaseFalseDerivedFalseAttr(UsdContrivedDerived &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateOverrideBaseFalseDerivedFalseAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateOverrideBaseFalseDerivedNoneAttr(UsdContrivedDerived &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateOverrideBaseFalseDerivedNoneAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateOverrideBaseNoneDerivedFalseAttr(UsdContrivedDerived &self,
                                      object defaultVal, bool writeSparsely) {
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

void wrapUsdContrivedDerived()
{
    typedef UsdContrivedDerived This;

    class_<This, bases<UsdContrivedBase> >
        cls("Derived");

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

        
        .def("GetPivotPositionAttr",
             &This::GetPivotPositionAttr)
        .def("CreatePivotPositionAttr",
             &_CreatePivotPositionAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMyVecfArrayAttr",
             &This::GetMyVecfArrayAttr)
        .def("CreateMyVecfArrayAttr",
             &_CreateMyVecfArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetHoleIndicesAttr",
             &This::GetHoleIndicesAttr)
        .def("CreateHoleIndicesAttr",
             &_CreateHoleIndicesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCornerIndicesAttr",
             &This::GetCornerIndicesAttr)
        .def("CreateCornerIndicesAttr",
             &_CreateCornerIndicesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCornerSharpnessesAttr",
             &This::GetCornerSharpnessesAttr)
        .def("CreateCornerSharpnessesAttr",
             &_CreateCornerSharpnessesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCreaseLengthsAttr",
             &This::GetCreaseLengthsAttr)
        .def("CreateCreaseLengthsAttr",
             &_CreateCreaseLengthsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTransformAttr",
             &This::GetTransformAttr)
        .def("CreateTransformAttr",
             &_CreateTransformAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTestingAssetAttr",
             &This::GetTestingAssetAttr)
        .def("CreateTestingAssetAttr",
             &_CreateTestingAssetAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetNamespacedPropertyAttr",
             &This::GetNamespacedPropertyAttr)
        .def("CreateNamespacedPropertyAttr",
             &_CreateNamespacedPropertyAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetJustDefaultAttr",
             &This::GetJustDefaultAttr)
        .def("CreateJustDefaultAttr",
             &_CreateJustDefaultAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetOverrideBaseTrueDerivedFalseAttr",
             &This::GetOverrideBaseTrueDerivedFalseAttr)
        .def("CreateOverrideBaseTrueDerivedFalseAttr",
             &_CreateOverrideBaseTrueDerivedFalseAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetOverrideBaseTrueDerivedNoneAttr",
             &This::GetOverrideBaseTrueDerivedNoneAttr)
        .def("CreateOverrideBaseTrueDerivedNoneAttr",
             &_CreateOverrideBaseTrueDerivedNoneAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetOverrideBaseFalseDerivedFalseAttr",
             &This::GetOverrideBaseFalseDerivedFalseAttr)
        .def("CreateOverrideBaseFalseDerivedFalseAttr",
             &_CreateOverrideBaseFalseDerivedFalseAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetOverrideBaseFalseDerivedNoneAttr",
             &This::GetOverrideBaseFalseDerivedNoneAttr)
        .def("CreateOverrideBaseFalseDerivedNoneAttr",
             &_CreateOverrideBaseFalseDerivedNoneAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetOverrideBaseNoneDerivedFalseAttr",
             &This::GetOverrideBaseNoneDerivedFalseAttr)
        .def("CreateOverrideBaseNoneDerivedFalseAttr",
             &_CreateOverrideBaseNoneDerivedFalseAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        
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
