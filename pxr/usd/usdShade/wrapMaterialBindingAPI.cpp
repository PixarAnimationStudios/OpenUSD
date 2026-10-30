//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
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

static std::string
_Repr(const UsdShadeMaterialBindingAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdShade.MaterialBindingAPI(%s)",
        primRepr.c_str());
}

struct UsdShadeMaterialBindingAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdShadeMaterialBindingAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdShadeMaterialBindingAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdShadeMaterialBindingAPI::CanApply(prim, &whyNot);
    return UsdShadeMaterialBindingAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdShadeMaterialBindingAPI(nb::module_ &m)
{
    typedef UsdShadeMaterialBindingAPI This;

    UsdShadeMaterialBindingAPI_CanApplyResult::WrapNb<UsdShadeMaterialBindingAPI_CanApplyResult>(m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "MaterialBindingAPI");

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

static nb::object
_WrapComputeBoundMaterial(const UsdShadeMaterialBindingAPI &bindingAPI,
                          const TfToken &materialPurpose,
                          bool supportLegacyBindings) {
    UsdRelationship bindingRel;
    UsdShadeMaterial mat = bindingAPI.ComputeBoundMaterial(materialPurpose,
            &bindingRel, supportLegacyBindings);
    return nb::make_tuple(mat, bindingRel);
}

static nb::object
_WrapComputeBoundMaterials(const std::vector<UsdPrim> &prims, 
                           const TfToken &materialPurpose, 
                           bool supportLegacyBindings)
{
    std::vector<UsdRelationship> bindingRels; 
    auto materials = UsdShadeMaterialBindingAPI::ComputeBoundMaterials(prims,
        materialPurpose, &bindingRels, supportLegacyBindings);
    return nb::make_tuple(materials, bindingRels);
}

WRAP_CUSTOM {

    using This = UsdShadeMaterialBindingAPI;

    nb::class_<This::DirectBinding> directBinding(_class, "DirectBinding");
    directBinding
        .def(nb::init<>())
        .def(nb::init<UsdRelationship>(), nb::arg("bindingRel"))
        .def("GetMaterial", &This::DirectBinding::GetMaterial)
        .def("GetBindingRel", &This::DirectBinding::GetBindingRel,
             nb::rv_policy::copy)
        .def("GetMaterialPath", &This::DirectBinding::GetMaterialPath,
             nb::rv_policy::copy)
        .def("GetMaterialPurpose", &This::DirectBinding::GetMaterialPurpose,
             nb::rv_policy::copy)
        ;

    nb::class_<This::CollectionBinding> collBinding(_class, "CollectionBinding");
    collBinding
        .def(nb::init<>())
        .def(nb::init<UsdRelationship>(), nb::arg("collBindingRel"))
        .def("GetCollection", &This::CollectionBinding::GetCollection)
        .def("GetMaterial", &This::CollectionBinding::GetMaterial)
        .def("GetCollectionPath", &This::CollectionBinding::GetCollectionPath,
             nb::rv_policy::copy)
        .def("GetMaterialPath", &This::CollectionBinding::GetMaterialPath,
             nb::rv_policy::copy)
        .def("GetBindingRel", &This::CollectionBinding::GetBindingRel,
             nb::rv_policy::copy)
        .def("IsValid", &This::CollectionBinding::IsValid)
        .def_static("IsCollectionBindingRel", &UsdShadeMaterialBindingAPI
            ::CollectionBinding::IsCollectionBindingRel,
            nb::arg("bindingRel"))
        ;
    
    TfPyRegisterStlSequencesFromPython<This::CollectionBindingVector>();

    _class
        .def("GetDirectBindingRel", &This::GetDirectBindingRel, 
             (nb::arg("materialPurpose")=UsdShadeTokens->allPurpose))

        .def("GetCollectionBindingRel", &This::GetCollectionBindingRel, 
             nb::arg("bindingName"),
              nb::arg("materialPurpose")=UsdShadeTokens->allPurpose)

        .def("GetCollectionBindingRels", &This::GetCollectionBindingRels,
             nb::arg("materialPurpose")=UsdShadeTokens->allPurpose,
             nb::rv_policy::copy)

        .def_static("GetMaterialBindingStrength", &This::GetMaterialBindingStrength,
             nb::arg("bindingRel"))

        .def_static("SetMaterialBindingStrength", &This::SetMaterialBindingStrength,
             nb::arg("bindingRel"), nb::arg("bindingStrength"))

        .def("GetDirectBinding", &This::GetDirectBinding,
             (nb::arg("materialPurpose")=UsdShadeTokens->allPurpose))

        .def("GetCollectionBindings", &This::GetCollectionBindings,
             nb::arg("materialPurpose")=UsdShadeTokens->allPurpose,
             nb::rv_policy::copy)

        .def("Bind", (bool(This::*)(const UsdShadeMaterial &,
                              const TfToken &,
                              const TfToken &) const) &This::Bind,
             nb::arg("material"), 
              nb::arg("bindingStrength")=UsdShadeTokens->fallbackStrength,
              nb::arg("materialPurpose")=UsdShadeTokens->allPurpose)

        .def("Bind", (bool(This::*)(const UsdCollectionAPI &collection,
                                    const UsdShadeMaterial &,
                                    const TfToken &,
                                    const TfToken &,
                                    const TfToken &) const) &This::Bind,
             nb::arg("collection"), 
              nb::arg("material"), 
              nb::arg("bindingName")=TfToken(),
              nb::arg("bindingStrength")=UsdShadeTokens->fallbackStrength,
              nb::arg("materialPurpose")=UsdShadeTokens->allPurpose)

        .def("UnbindDirectBinding", &This::UnbindDirectBinding, 
             nb::arg("materialPurpose")=UsdShadeTokens->allPurpose)

        .def("UnbindCollectionBinding", &This::UnbindCollectionBinding, 
             nb::arg("bindingName"), 
              nb::arg("materialPurpose")=UsdShadeTokens->allPurpose)

        .def("UnbindAllBindings", &This::UnbindAllBindings)

        .def("RemovePrimFromBindingCollection", 
             &This::RemovePrimFromBindingCollection,
             nb::arg("prim"), nb::arg("bindingName"),
              nb::arg("materialPurpose")=UsdShadeTokens->allPurpose)

        .def("AddPrimToBindingCollection", 
             &This::AddPrimToBindingCollection,
             nb::arg("prim"), nb::arg("bindingName"),
              nb::arg("materialPurpose")=UsdShadeTokens->allPurpose)

        .def_static("GetMaterialPurposes", &This::GetMaterialPurposes)

        .def_static("GetResolvedTargetPathFromBindingRel",
             &UsdShadeMaterialBindingAPI::GetResolvedTargetPathFromBindingRel,
             nb::arg("bindingRel"))

        .def("ComputeBoundMaterial", &_WrapComputeBoundMaterial,
             nb::arg("materialPurpose")=UsdShadeTokens->allPurpose,
             nb::arg("supportLegacyBindings")=true)

        .def_static("ComputeBoundMaterials", &_WrapComputeBoundMaterials,
             nb::arg("prims"), nb::arg("materialPurpose")=UsdShadeTokens->allPurpose,
              nb::arg("supportLegacyBindings")=true)

        .def("CreateMaterialBindSubset", 
             &This::CreateMaterialBindSubset,
             nb::arg("subsetName"), 
              nb::arg("indices"), nb::arg("elementType")=UsdGeomTokens->face)
        .def("GetMaterialBindSubsets", 
             &This::GetMaterialBindSubsets,
             nb::rv_policy::copy)
        .def("SetMaterialBindSubsetsFamilyType", 
             &This::SetMaterialBindSubsetsFamilyType,
             nb::arg("familyType"))
        .def("GetMaterialBindSubsetsFamilyType",
             &This::GetMaterialBindSubsetsFamilyType)
        .def_static("CanContainPropertyName", 
            &UsdShadeMaterialBindingAPI::CanContainPropertyName, 
            nb::arg("name"))
    ;
}

}
