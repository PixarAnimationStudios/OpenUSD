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


} // anonymous namespace

void wrapUsdShadeMaterialBindingAPI()
{
    typedef UsdShadeMaterialBindingAPI This;

    class_<This, bases<UsdAPISchemaBase> >
        cls("MaterialBindingAPI");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("Apply", &This::Apply, (arg("prim")))
        .staticmethod("Apply")

        .def("IsConcrete",
            static_cast<bool (*)(void)>( [](){ return This::IsConcrete; }))
        .staticmethod("IsConcrete")

        .def("IsTyped",
            static_cast<bool (*)(void)>( [](){ return This::IsTyped; } ))
        .staticmethod("IsTyped")

        .def("IsApplied", 
            static_cast<bool (*)(void)>( [](){ return This::IsApplied; } ))
        .staticmethod("IsApplied")

        .def("IsMultipleApply", 
            static_cast<bool (*)(void)>( [](){ return This::IsMultipleApply; } ))
        .staticmethod("IsMultipleApply")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)


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

#include <boost/python/tuple.hpp>

namespace {

static object
_WrapComputeBoundMaterial(const UsdShadeMaterialBindingAPI &bindingAPI,
                          const TfToken &materialPurpose) {
    UsdRelationship bindingRel;
    UsdShadeMaterial mat = bindingAPI.ComputeBoundMaterial(materialPurpose,
            &bindingRel);
    return boost::python::make_tuple(mat, bindingRel);
}

static object
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
    scope scope_root = _class;

    class_<This::DirectBinding> directBinding("DirectBinding");
    directBinding
        .def(init<>())
        .def(init<UsdRelationship>(arg("bindingRel")))
        .def("GetMaterial", &This::DirectBinding::GetMaterial)
        .def("GetBindingRel", &This::DirectBinding::GetBindingRel,
             return_value_policy<return_by_value>())
        .def("GetMaterialPath", &This::DirectBinding::GetMaterialPath,
             return_value_policy<return_by_value>())
        .def("GetMaterialPurpose", &This::DirectBinding::GetMaterialPurpose,
             return_value_policy<return_by_value>())
        ;

    class_<This::CollectionBinding> collBinding("CollectionBinding");
    collBinding
        .def(init<>())
        .def(init<UsdRelationship>(arg("collBindingRel")))
        .def("GetCollection", &This::CollectionBinding::GetCollection)
        .def("GetMaterial", &This::CollectionBinding::GetMaterial)
        .def("GetCollectionPath", &This::CollectionBinding::GetCollectionPath,
             return_value_policy<return_by_value>())
        .def("GetMaterialPath", &This::CollectionBinding::GetMaterialPath,
             return_value_policy<return_by_value>())
        .def("GetBindingRel", &This::CollectionBinding::GetBindingRel,
             return_value_policy<return_by_value>())
        .def("IsValid", &This::CollectionBinding::IsValid)
        ;
    
    to_python_converter<This::CollectionBindingVector,
                        TfPySequenceToPython<This::CollectionBindingVector>>();
    TfPyRegisterStlSequencesFromPython<This::CollectionBindingVector>();

    scope scope_materialBindingAPI = _class
        .def("GetDirectBindingRel", &This::GetDirectBindingRel, 
             (arg("materialPurpose")=UsdShadeTokens->allPurpose))

        .def("GetCollectionBindingRel", &This::GetCollectionBindingRel, 
             (arg("bindingName"),
              arg("materialPurpose")=UsdShadeTokens->allPurpose))

        .def("GetCollectionBindingRels", &This::GetCollectionBindingRels,
             arg("materialPurpose")=UsdShadeTokens->allPurpose,
             return_value_policy<TfPySequenceToList>())

        .def("GetMaterialBindingStrength", &This::GetMaterialBindingStrength,
             arg("bindingRel"))
             .staticmethod("GetMaterialBindingStrength")

        .def("SetMaterialBindingStrength", &This::SetMaterialBindingStrength,
             arg("bindingRel"))
             .staticmethod("SetMaterialBindingStrength")

        .def("GetDirectBinding", &This::GetDirectBinding,
             (arg("materialPurpose")=UsdShadeTokens->allPurpose))

        .def("GetCollectionBindings", &This::GetCollectionBindings,
             arg("materialPurpose")=UsdShadeTokens->allPurpose,
             return_value_policy<TfPySequenceToList>())

        .def("Bind", (bool(This::*)(const UsdShadeMaterial &,
                              const TfToken &,
                              const TfToken &) const) &This::Bind,
             (arg("material"), 
              arg("bindingStrength")=UsdShadeTokens->fallbackStrength,
              arg("materialPurpose")=UsdShadeTokens->allPurpose))

        .def("Bind", (bool(This::*)(const UsdCollectionAPI &collection,
                                    const UsdShadeMaterial &,
                                    const TfToken &,
                                    const TfToken &,
                                    const TfToken &) const) &This::Bind,
             (arg("collection"), 
              arg("material"), 
              arg("bindingName")=TfToken(),
              arg("bindingStrength")=UsdShadeTokens->fallbackStrength,
              arg("materialPurpose")=UsdShadeTokens->allPurpose))

        .def("UnbindDirectBinding", &This::UnbindDirectBinding, 
             arg("materialPurpose")=UsdShadeTokens->allPurpose)

        .def("UnbindCollectionBinding", &This::UnbindCollectionBinding, 
             (arg("bindingName"), 
              arg("materialPurpose")=UsdShadeTokens->allPurpose))

        .def("UnbindAllBindings", &This::UnbindAllBindings)

        .def("RemovePrimFromBindingCollection", 
             &This::RemovePrimFromBindingCollection,
             (arg("prim"), arg("bindingName"),
              arg("materialPurpose")=UsdShadeTokens->allPurpose))

        .def("AddPrimToBindingCollection", 
             &This::AddPrimToBindingCollection,
             (arg("prim"), arg("bindingName"),
              arg("materialPurpose")=UsdShadeTokens->allPurpose))

        .def("ComputeBoundMaterial", &_WrapComputeBoundMaterial,
             arg("materialPurpose")=UsdShadeTokens->allPurpose)

        .def("ComputeBoundMaterials", &_WrapComputeBoundMaterials,
             (arg("prims"), arg("materialPurpose")=UsdShadeTokens->allPurpose))
            .staticmethod("ComputeBoundMaterials")

        .def("CreateMaterialBindSubset", 
             &This::CreateMaterialBindSubset,
             (arg("geom"), arg("subsetName"), 
              arg("indices"), arg("elementType")=UsdGeomTokens->face))
            .staticmethod("CreateMaterialBindSubset")
        .def("GetMaterialBindSubsets", 
             &This::GetMaterialBindSubsets, 
             arg("geom"), return_value_policy<TfPySequenceToList>())
             .staticmethod("GetMaterialBindSubsets")
        .def("SetMaterialBindSubsetsFamilyType", 
             &This::SetMaterialBindSubsetsFamilyType,
             (arg("geom"), arg("familyType")))
             .staticmethod("SetMaterialBindSubsetsFamilyType")
        .def("GetMaterialBindSubsetsFamilyType",
             &This::GetMaterialBindSubsetsFamilyType,
             arg("geom"))
             .staticmethod("GetMaterialBindSubsetsFamilyType")
    ;
}

}
