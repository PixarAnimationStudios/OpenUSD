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
_CreateSurfaceAttr(UsdShadeMaterial &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateSurfaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateDisplacementAttr(UsdShadeMaterial &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDisplacementAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateVolumeAttr(UsdShadeMaterial &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVolumeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

} // anonymous namespace

void wrapUsdShadeMaterial()
{
    typedef UsdShadeMaterial This;

    class_<This, bases<UsdShadeNodeGraph> >
        cls("Material");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("Define", &This::Define, (arg("stage"), arg("path")))
        .staticmethod("Define")

        .def("IsConcrete",
            static_cast<bool (*)(void)>( [](){ return This::IsConcrete; }))
        .staticmethod("IsConcrete")

        .def("IsTyped",
            static_cast<bool (*)(void)>( [](){ return This::IsTyped; } ))
        .staticmethod("IsTyped")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetSurfaceAttr",
             &This::GetSurfaceAttr)
        .def("CreateSurfaceAttr",
             &_CreateSurfaceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDisplacementAttr",
             &This::GetDisplacementAttr)
        .def("CreateDisplacementAttr",
             &_CreateDisplacementAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVolumeAttr",
             &This::GetVolumeAttr)
        .def("CreateVolumeAttr",
             &_CreateVolumeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

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

#include "pxr/usd/usd/editContext.h"
#include "pxr/usd/usd/pyEditContext.h"
#include "pxr/usd/usdShade/utils.h"

#include <boost/python/tuple.hpp>

namespace {

static UsdPyEditContext
_GetEditContextForVariant(const UsdShadeMaterial &self,
                          const TfToken &materialVariantName,
                          const SdfLayerHandle layer) {
    return UsdPyEditContext(
        self.GetEditContextForVariant(materialVariantName, layer));
}

static object
_WrapComputeSurfaceSource(const UsdShadeMaterial &self, 
                          const TfToken &renderContext) 
{
    UsdShadeShader source; 
    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    source = self.ComputeSurfaceSource(renderContext, &sourceName, &sourceType);
    return boost::python::make_tuple (source, sourceName, sourceType);
}

static object
_WrapComputeDisplacementSource(const UsdShadeMaterial &self, 
                               const TfToken &renderContext) 
{
    UsdShadeShader source; 
    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    source = self.ComputeDisplacementSource(renderContext, &sourceName, &sourceType);
    return boost::python::make_tuple (source, sourceName, sourceType);
}

static object
_WrapComputeVolumeSource(const UsdShadeMaterial &self, 
                         const TfToken &renderContext) 
{
    UsdShadeShader source; 
    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    source = self.ComputeVolumeSource(renderContext, &sourceName, &sourceType);
    return boost::python::make_tuple (source, sourceName, sourceType);
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


        .def("CreateMaterialBindSubset", 
             &UsdShadeMaterial::CreateMaterialBindSubset,
             (arg("geom"), arg("subsetName"), 
              arg("indices"), arg("elementType")=UsdGeomTokens->face))
            .staticmethod("CreateMaterialBindSubset")
        .def("GetMaterialBindSubsets", 
             &UsdShadeMaterial::GetMaterialBindSubsets, 
             arg("geom"), return_value_policy<TfPySequenceToList>())
             .staticmethod("GetMaterialBindSubsets")
        .def("SetMaterialBindSubsetsFamilyType", 
             &UsdShadeMaterial::SetMaterialBindSubsetsFamilyType,
             (arg("geom"), arg("familyType")))
             .staticmethod("SetMaterialBindSubsetsFamilyType")
        .def("GetMaterialBindSubsetsFamilyType",
             &UsdShadeMaterial::GetMaterialBindSubsetsFamilyType,
             arg("geom"))
             .staticmethod("GetMaterialBindSubsetsFamilyType")
        
        .def("CreateSurfaceOutput", &UsdShadeMaterial::CreateSurfaceOutput, 
            (arg("renderContext")=UsdShadeTokens->universalRenderContext))
        .def("GetSurfaceOutput", &UsdShadeMaterial::GetSurfaceOutput, 
            (arg("renderContext")=UsdShadeTokens->universalRenderContext))
        .def("ComputeSurfaceSource", &_WrapComputeSurfaceSource, 
            (arg("renderContext")=UsdShadeTokens->universalRenderContext))

        .def("CreateDisplacementOutput", 
            &UsdShadeMaterial::CreateDisplacementOutput, 
            (arg("renderContext")=UsdShadeTokens->universalRenderContext))
        .def("GetDisplacementOutput", 
            &UsdShadeMaterial::GetDisplacementOutput, 
            (arg("renderContext")=UsdShadeTokens->universalRenderContext))
        .def("ComputeDisplacementSource", &_WrapComputeDisplacementSource, 
            (arg("renderContext")=UsdShadeTokens->universalRenderContext))

        .def("CreateVolumeOutput", &UsdShadeMaterial::CreateVolumeOutput, 
            (arg("renderContext")=UsdShadeTokens->universalRenderContext))
        .def("GetVolumeOutput", &UsdShadeMaterial::GetVolumeOutput, 
            (arg("renderContext")=UsdShadeTokens->universalRenderContext))
        .def("ComputeVolumeSource", &_WrapComputeVolumeSource, 
            (arg("renderContext")=UsdShadeTokens->universalRenderContext))
        
        // These are now deprecated.
        .def("CreateMaterialFaceSet", &UsdShadeMaterial::CreateMaterialFaceSet)
            .staticmethod("CreateMaterialFaceSet")
        .def("GetMaterialFaceSet", &UsdShadeMaterial::GetMaterialFaceSet)
            .staticmethod("GetMaterialFaceSet")
        .def("HasMaterialFaceSet", &UsdShadeMaterial::HasMaterialFaceSet)
            .staticmethod("HasMaterialFaceSet")

        ;

    TfPyRegisterStlSequencesFromPython<UsdShadeMaterial>();
    to_python_converter<std::vector<UsdShadeMaterial>,
                        TfPySequenceToPython<std::vector<UsdShadeMaterial>>>();

}

} // anonymous namespace
