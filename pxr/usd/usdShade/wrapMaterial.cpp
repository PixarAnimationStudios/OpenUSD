//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdShade/material.h"
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
_CreateSurfaceAttr(UsdShadeMaterial &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateSurfaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateDisplacementAttr(UsdShadeMaterial &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateDisplacementAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateVolumeAttr(UsdShadeMaterial &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateVolumeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdShadeMaterial &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdShade.Material(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdShadeMaterial(nb::module_ &m)
{
    typedef UsdShadeMaterial This;

    nb::class_<This, UsdShadeNodeGraph> cls(m, "Material");

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

#include "pxr/usd/usd/editContext.h"
#include "pxr/usd/usd/pyEditContext.h"
#include "pxr/usd/usdShade/utils.h"

namespace {

static UsdPyEditContext
_GetEditContextForVariant(const UsdShadeMaterial &self,
                          const TfToken &materialVariantName,
                          const SdfLayerHandle layer) {
    return UsdPyEditContext(
        self.GetEditContextForVariant(materialVariantName, layer));
}

static nb::object
_WrapComputeSurfaceSource(const UsdShadeMaterial &self, 
                          const TfToken &renderContext) 
{
    UsdShadeShader source; 
    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    source = self.ComputeSurfaceSource({renderContext}, &sourceName, &sourceType);
    return nb::make_tuple(source, sourceName, sourceType);
}

static nb::object
_WrapComputeDisplacementSource(const UsdShadeMaterial &self, 
                               const TfToken &renderContext) 
{
    UsdShadeShader source; 
    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    source = self.ComputeDisplacementSource({renderContext}, &sourceName, &sourceType);
    return nb::make_tuple(source, sourceName, sourceType);
}

static nb::object
_WrapComputeVolumeSource(const UsdShadeMaterial &self, 
                         const TfToken &renderContext) 
{
    UsdShadeShader source; 
    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    source = self.ComputeVolumeSource({renderContext}, &sourceName, &sourceType);
    return nb::make_tuple(source, sourceName, sourceType);
}

WRAP_CUSTOM {
    _class
        .def("GetMaterialVariant", &UsdShadeMaterial::GetMaterialVariant)
        .def_static("CreateMasterMaterialVariant",
             &UsdShadeMaterial::CreateMasterMaterialVariant,
             nb::arg("masterPrim"), nb::arg("materialPrims"),
              nb::arg("masterVariantSetName")=TfToken())
        .def("GetEditContextForVariant", _GetEditContextForVariant,
             nb::arg("materialVariantName"), nb::arg("layer")=SdfLayerHandle())

        .def("GetBaseMaterialPath",
             &UsdShadeMaterial::GetBaseMaterialPath)
         .def("GetBaseMaterial",
              &UsdShadeMaterial::GetBaseMaterial)
        .def("SetBaseMaterialPath",
             &UsdShadeMaterial::SetBaseMaterialPath,
             nb::arg("baseLookPath"))
         .def("SetBaseMaterial",
              &UsdShadeMaterial::SetBaseMaterial,
              nb::arg("baseMaterial"))
        .def("ClearBaseMaterial",
             &UsdShadeMaterial::ClearBaseMaterial)
        .def("HasBaseMaterial",
             &UsdShadeMaterial::HasBaseMaterial)

        .def("CreateSurfaceOutput", &UsdShadeMaterial::CreateSurfaceOutput, 
            (nb::arg("renderContext")=UsdShadeTokens->universalRenderContext))
        .def("GetSurfaceOutput", &UsdShadeMaterial::GetSurfaceOutput, 
            (nb::arg("renderContext")=UsdShadeTokens->universalRenderContext))
        .def("GetSurfaceOutputs", &UsdShadeMaterial::GetSurfaceOutputs)
        .def("ComputeSurfaceSource", &_WrapComputeSurfaceSource, 
            (nb::arg("renderContext")=UsdShadeTokens->universalRenderContext))

        .def("CreateDisplacementOutput", 
            &UsdShadeMaterial::CreateDisplacementOutput, 
            (nb::arg("renderContext")=UsdShadeTokens->universalRenderContext))
        .def("GetDisplacementOutput", 
            &UsdShadeMaterial::GetDisplacementOutput, 
            (nb::arg("renderContext")=UsdShadeTokens->universalRenderContext))
        .def("GetDisplacementOutputs",
            &UsdShadeMaterial::GetDisplacementOutputs)
        .def("ComputeDisplacementSource", &_WrapComputeDisplacementSource, 
            (nb::arg("renderContext")=UsdShadeTokens->universalRenderContext))

        .def("CreateVolumeOutput", &UsdShadeMaterial::CreateVolumeOutput, 
            (nb::arg("renderContext")=UsdShadeTokens->universalRenderContext))
        .def("GetVolumeOutput", &UsdShadeMaterial::GetVolumeOutput, 
            (nb::arg("renderContext")=UsdShadeTokens->universalRenderContext))
        .def("GetVolumeOutputs", &UsdShadeMaterial::GetVolumeOutputs)
        .def("ComputeVolumeSource", &_WrapComputeVolumeSource, 
            (nb::arg("renderContext")=UsdShadeTokens->universalRenderContext))

        ;

    TfPyRegisterStlSequencesFromPython<UsdShadeMaterial>();

}

} // anonymous namespace
