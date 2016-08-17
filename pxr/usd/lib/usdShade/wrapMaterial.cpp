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
#include "pxr/usd/usdShade/material.h"

#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/conversions.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;


void wrapUsdShadeMaterial()
{
    typedef UsdShadeMaterial This;

    class_<This, bases<UsdShadeSubgraph> >
        cls("Material");

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
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/usd/usd/editContext.h"

static UsdPyEditContext
_GetEditContextForVariant(const UsdShadeMaterial &self,
                          const TfToken &materialVariantName,
                          const SdfLayerHandle layer) {
    return UsdPyEditContext(
        self.GetEditContextForVariant(materialVariantName, layer));
}

WRAP_CUSTOM {
    _class
        .def("Bind", &UsdShadeMaterial::Bind)
        .def("Unbind", &UsdShadeMaterial::Unbind)
        .staticmethod("Unbind")
        .def("GetBindingRel", &UsdShadeMaterial::GetBindingRel)
        .staticmethod("GetBindingRel")
        .def("GetBoundMaterial", &UsdShadeMaterial::GetBoundMaterial)
        .staticmethod("GetBoundMaterial")
        .def("GetMaterialVariant", &UsdShadeMaterial::GetMaterialVariant)
        .def("CreateMasterMaterialVariant",
             &UsdShadeMaterial::CreateMasterMaterialVariant,
             (arg("masterPrim"), arg("materialPrims"),
              arg("masterVariantSetName")=TfToken()))
        .staticmethod("CreateMasterMaterialVariant")
        .def("GetEditContextForVariant", _GetEditContextForVariant,
             (arg("materialVariantName"), arg("layer")=SdfLayerHandle()))

        .def("GetBaseMaterialPath",
             &UsdShadeMaterial::GetBaseMaterialPath)
         .def("GetBaseMaterial",
              &UsdShadeMaterial::GetBaseMaterial)
        .def("SetBaseMaterialPath",
             &UsdShadeMaterial::SetBaseMaterialPath,
             (arg("baseLookPath")))
         .def("SetBaseMaterial",
              &UsdShadeMaterial::SetBaseMaterial,
              (arg("baseMaterial")))
        .def("ClearBaseMaterial",
             &UsdShadeMaterial::ClearBaseMaterial)
        .def("HasBaseMaterial",
             &UsdShadeMaterial::HasBaseMaterial)

        .def("CreateMaterialFaceSet", &UsdShadeMaterial::CreateMaterialFaceSet)
            .staticmethod("CreateMaterialFaceSet")

        .def("GetMaterialFaceSet", &UsdShadeMaterial::GetMaterialFaceSet)
            .staticmethod("GetMaterialFaceSet")

        .def("HasMaterialFaceSet", &UsdShadeMaterial::HasMaterialFaceSet)
            .staticmethod("HasMaterialFaceSet")

        .def("CreateSurfaceTerminal", 
             &UsdShadeMaterial::CreateSurfaceTerminal,
             (arg("targetPath")))
        .def("GetSurfaceTerminal",
             &UsdShadeMaterial::GetSurfaceTerminal)

        .def("CreateDisplacementTerminal", 
             &UsdShadeMaterial::CreateDisplacementTerminal,
             (arg("targetPath")))
        .def("GetDisplacementTerminal",
             &UsdShadeMaterial::GetDisplacementTerminal)
        ;
}
