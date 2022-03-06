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
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>


PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdUsdShadeWrapMaterialBindingAPI {

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

void wrapUsdShadeMaterialBindingAPI()
{
    typedef UsdShadeMaterialBindingAPI This;

    pxrUsdUsdShadeWrapMaterialBindingAPI::UsdShadeMaterialBindingAPI_CanApplyResult::Wrap<pxrUsdUsdShadeWrapMaterialBindingAPI::UsdShadeMaterialBindingAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    boost::python::class_<This, boost::python::bases<UsdAPISchemaBase> >
        cls("MaterialBindingAPI");

    cls
        .def(boost::python::init<UsdPrim>(boost::python::arg("prim")))
        .def(boost::python::init<UsdSchemaBase const&>(boost::python::arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Get")

        .def("CanApply", &pxrUsdUsdShadeWrapMaterialBindingAPI::_WrapCanApply, (boost::python::arg("prim")))
        .staticmethod("CanApply")

        .def("Apply", &This::Apply, (boost::python::arg("prim")))
        .staticmethod("Apply")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             boost::python::arg("includeInherited")=true,
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!boost::python::self)


        .def("__repr__", pxrUsdUsdShadeWrapMaterialBindingAPI::_Repr)
    ;

    pxrUsdUsdShadeWrapMaterialBindingAPI::_CustomWrapCode(cls);
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

#include <boost/python/tuple.hpp>

namespace pxrUsdUsdShadeWrapMaterialBindingAPI {

static boost::python::object
_WrapComputeBoundMaterial(const UsdShadeMaterialBindingAPI &bindingAPI,
                          const TfToken &materialPurpose) {
    UsdRelationship bindingRel;
    UsdShadeMaterial mat = bindingAPI.ComputeBoundMaterial(materialPurpose,
            &bindingRel);
    return boost::python::make_tuple(mat, bindingRel);
}

static boost::python::object
_WrapComputeBoundMaterials(const std::vector<UsdPrim> &prims, 
                           const TfToken &materialPurpose)
{
    std::vector<UsdRelationship> bindingRels; 
    auto materials = UsdShadeMaterialBindingAPI::ComputeBoundMaterials(prims,
        materialPurpose, &bindingRels);
    return boost::python::make_tuple(materials, bindingRels);
}

WRAP_CUSTOM {

    using This = UsdShadeMaterialBindingAPI;

    // Create a root scope so that CollectionBinding is scoped under 
    // UsdShade.MaterialBindingAPI.
    boost::python::scope scope_root = _class;

    boost::python::class_<This::DirectBinding> directBinding("DirectBinding");
    directBinding
        .def(boost::python::init<>())
        .def(boost::python::init<UsdRelationship>(boost::python::arg("bindingRel")))
        .def("GetMaterial", &This::DirectBinding::GetMaterial)
        .def("GetBindingRel", &This::DirectBinding::GetBindingRel,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("GetMaterialPath", &This::DirectBinding::GetMaterialPath,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("GetMaterialPurpose", &This::DirectBinding::GetMaterialPurpose,
             boost::python::return_value_policy<boost::python::return_by_value>())
        ;

    boost::python::class_<This::CollectionBinding> collBinding("CollectionBinding");
    collBinding
        .def(boost::python::init<>())
        .def(boost::python::init<UsdRelationship>(boost::python::arg("collBindingRel")))
        .def("GetCollection", &This::CollectionBinding::GetCollection)
        .def("GetMaterial", &This::CollectionBinding::GetMaterial)
        .def("GetCollectionPath", &This::CollectionBinding::GetCollectionPath,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("GetMaterialPath", &This::CollectionBinding::GetMaterialPath,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("GetBindingRel", &This::CollectionBinding::GetBindingRel,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("IsValid", &This::CollectionBinding::IsValid)
        ;
    
    boost::python::to_python_converter<This::CollectionBindingVector,
                        TfPySequenceToPython<This::CollectionBindingVector>>();
    TfPyRegisterStlSequencesFromPython<This::CollectionBindingVector>();

    boost::python::scope scope_materialBindingAPI = _class
        .def("GetDirectBindingRel", &This::GetDirectBindingRel, 
             (boost::python::arg("materialPurpose")=UsdShadeTokens->allPurpose))

        .def("GetCollectionBindingRel", &This::GetCollectionBindingRel, 
             (boost::python::arg("bindingName"),
              boost::python::arg("materialPurpose")=UsdShadeTokens->allPurpose))

        .def("GetCollectionBindingRels", &This::GetCollectionBindingRels,
             boost::python::arg("materialPurpose")=UsdShadeTokens->allPurpose,
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("GetMaterialBindingStrength", &This::GetMaterialBindingStrength,
             boost::python::arg("bindingRel"))
             .staticmethod("GetMaterialBindingStrength")

        .def("SetMaterialBindingStrength", &This::SetMaterialBindingStrength,
             boost::python::arg("bindingRel"))
             .staticmethod("SetMaterialBindingStrength")

        .def("GetDirectBinding", &This::GetDirectBinding,
             (boost::python::arg("materialPurpose")=UsdShadeTokens->allPurpose))

        .def("GetCollectionBindings", &This::GetCollectionBindings,
             boost::python::arg("materialPurpose")=UsdShadeTokens->allPurpose,
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("Bind", (bool(This::*)(const UsdShadeMaterial &,
                              const TfToken &,
                              const TfToken &) const) &This::Bind,
             (boost::python::arg("material"), 
              boost::python::arg("bindingStrength")=UsdShadeTokens->fallbackStrength,
              boost::python::arg("materialPurpose")=UsdShadeTokens->allPurpose))

        .def("Bind", (bool(This::*)(const UsdCollectionAPI &collection,
                                    const UsdShadeMaterial &,
                                    const TfToken &,
                                    const TfToken &,
                                    const TfToken &) const) &This::Bind,
             (boost::python::arg("collection"), 
              boost::python::arg("material"), 
              boost::python::arg("bindingName")=TfToken(),
              boost::python::arg("bindingStrength")=UsdShadeTokens->fallbackStrength,
              boost::python::arg("materialPurpose")=UsdShadeTokens->allPurpose))

        .def("UnbindDirectBinding", &This::UnbindDirectBinding, 
             boost::python::arg("materialPurpose")=UsdShadeTokens->allPurpose)

        .def("UnbindCollectionBinding", &This::UnbindCollectionBinding, 
             (boost::python::arg("bindingName"), 
              boost::python::arg("materialPurpose")=UsdShadeTokens->allPurpose))

        .def("UnbindAllBindings", &This::UnbindAllBindings)

        .def("RemovePrimFromBindingCollection", 
             &This::RemovePrimFromBindingCollection,
             (boost::python::arg("prim"), boost::python::arg("bindingName"),
              boost::python::arg("materialPurpose")=UsdShadeTokens->allPurpose))

        .def("AddPrimToBindingCollection", 
             &This::AddPrimToBindingCollection,
             (boost::python::arg("prim"), boost::python::arg("bindingName"),
              boost::python::arg("materialPurpose")=UsdShadeTokens->allPurpose))

        .def("ComputeBoundMaterial", &_WrapComputeBoundMaterial,
             boost::python::arg("materialPurpose")=UsdShadeTokens->allPurpose)

        .def("ComputeBoundMaterials", &_WrapComputeBoundMaterials,
             (boost::python::arg("prims"), boost::python::arg("materialPurpose")=UsdShadeTokens->allPurpose))
            .staticmethod("ComputeBoundMaterials")

        .def("CreateMaterialBindSubset", 
             &This::CreateMaterialBindSubset,
             (boost::python::arg("subsetName"), 
              boost::python::arg("indices"), boost::python::arg("elementType")=UsdGeomTokens->face))
        .def("GetMaterialBindSubsets", 
             &This::GetMaterialBindSubsets, 
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("SetMaterialBindSubsetsFamilyType", 
             &This::SetMaterialBindSubsetsFamilyType,
             (boost::python::arg("familyType")))
        .def("GetMaterialBindSubsetsFamilyType",
             &This::GetMaterialBindSubsetsFamilyType)
        .def("CanContainPropertyName", 
            &UsdShadeMaterialBindingAPI::CanContainPropertyName, 
            boost::python::arg("name"))
        .staticmethod("CanContainPropertyName")
    ;
}

}
