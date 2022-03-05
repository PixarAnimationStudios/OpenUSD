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


PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateSurfaceAttr(UsdShadeMaterial &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateSurfaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateDisplacementAttr(UsdShadeMaterial &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateDisplacementAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateVolumeAttr(UsdShadeMaterial &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
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

void wrapUsdShadeMaterial()
{
    typedef UsdShadeMaterial This;

    boost::python::class_<This, boost::python::bases<UsdShadeNodeGraph> >
        cls("Material");

    cls
        .def(boost::python::init<UsdPrim>(boost::python::arg("prim")))
        .def(boost::python::init<UsdSchemaBase const&>(boost::python::arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Get")

        .def("Define", &This::Define, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Define")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             boost::python::arg("includeInherited")=true,
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!boost::python::self)

        
        .def("GetSurfaceAttr",
             &This::GetSurfaceAttr)
        .def("CreateSurfaceAttr",
             &_CreateSurfaceAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetDisplacementAttr",
             &This::GetDisplacementAttr)
        .def("CreateDisplacementAttr",
             &_CreateDisplacementAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetVolumeAttr",
             &This::GetVolumeAttr)
        .def("CreateVolumeAttr",
             &_CreateVolumeAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))

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

#include <boost/python/tuple.hpp>

namespace {

static UsdPyEditContext
_GetEditContextForVariant(const UsdShadeMaterial &self,
                          const TfToken &materialVariantName,
                          const SdfLayerHandle layer) {
    return UsdPyEditContext(
        self.GetEditContextForVariant(materialVariantName, layer));
}

static boost::python::object
_WrapComputeSurfaceSource(const UsdShadeMaterial &self, 
                          const TfToken &renderContext) 
{
    UsdShadeShader source; 
    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    source = self.ComputeSurfaceSource({renderContext}, &sourceName, &sourceType);
    return boost::python::make_tuple (source, sourceName, sourceType);
}

static boost::python::object
_WrapComputeDisplacementSource(const UsdShadeMaterial &self, 
                               const TfToken &renderContext) 
{
    UsdShadeShader source; 
    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    source = self.ComputeDisplacementSource({renderContext}, &sourceName, &sourceType);
    return boost::python::make_tuple (source, sourceName, sourceType);
}

static boost::python::object
_WrapComputeVolumeSource(const UsdShadeMaterial &self, 
                         const TfToken &renderContext) 
{
    UsdShadeShader source; 
    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    source = self.ComputeVolumeSource({renderContext}, &sourceName, &sourceType);
    return boost::python::make_tuple (source, sourceName, sourceType);
}

WRAP_CUSTOM {
    _class
        .def("GetMaterialVariant", &UsdShadeMaterial::GetMaterialVariant)
        .def("CreateMasterMaterialVariant",
             &UsdShadeMaterial::CreateMasterMaterialVariant,
             (boost::python::arg("masterPrim"), boost::python::arg("materialPrims"),
              boost::python::arg("masterVariantSetName")=TfToken()))
        .staticmethod("CreateMasterMaterialVariant")
        .def("GetEditContextForVariant", _GetEditContextForVariant,
             (boost::python::arg("materialVariantName"), boost::python::arg("layer")=SdfLayerHandle()))

        .def("GetBaseMaterialPath",
             &UsdShadeMaterial::GetBaseMaterialPath)
         .def("GetBaseMaterial",
              &UsdShadeMaterial::GetBaseMaterial)
        .def("SetBaseMaterialPath",
             &UsdShadeMaterial::SetBaseMaterialPath,
             (boost::python::arg("baseLookPath")))
         .def("SetBaseMaterial",
              &UsdShadeMaterial::SetBaseMaterial,
              (boost::python::arg("baseMaterial")))
        .def("ClearBaseMaterial",
             &UsdShadeMaterial::ClearBaseMaterial)
        .def("HasBaseMaterial",
             &UsdShadeMaterial::HasBaseMaterial)


        .def("CreateSurfaceOutput", &UsdShadeMaterial::CreateSurfaceOutput, 
            (boost::python::arg("renderContext")=UsdShadeTokens->universalRenderContext))
        .def("GetSurfaceOutput", &UsdShadeMaterial::GetSurfaceOutput, 
            (boost::python::arg("renderContext")=UsdShadeTokens->universalRenderContext))
        .def("GetSurfaceOutputs", &UsdShadeMaterial::GetSurfaceOutputs)
        .def("ComputeSurfaceSource", &_WrapComputeSurfaceSource, 
            (boost::python::arg("renderContext")=UsdShadeTokens->universalRenderContext))

        .def("CreateDisplacementOutput", 
            &UsdShadeMaterial::CreateDisplacementOutput, 
            (boost::python::arg("renderContext")=UsdShadeTokens->universalRenderContext))
        .def("GetDisplacementOutput", 
            &UsdShadeMaterial::GetDisplacementOutput, 
            (boost::python::arg("renderContext")=UsdShadeTokens->universalRenderContext))
        .def("GetDisplacementOutputs",
            &UsdShadeMaterial::GetDisplacementOutputs)
        .def("ComputeDisplacementSource", &_WrapComputeDisplacementSource, 
            (boost::python::arg("renderContext")=UsdShadeTokens->universalRenderContext))

        .def("CreateVolumeOutput", &UsdShadeMaterial::CreateVolumeOutput, 
            (boost::python::arg("renderContext")=UsdShadeTokens->universalRenderContext))
        .def("GetVolumeOutput", &UsdShadeMaterial::GetVolumeOutput, 
            (boost::python::arg("renderContext")=UsdShadeTokens->universalRenderContext))
        .def("GetVolumeOutputs", &UsdShadeMaterial::GetVolumeOutputs)
        .def("ComputeVolumeSource", &_WrapComputeVolumeSource, 
            (boost::python::arg("renderContext")=UsdShadeTokens->universalRenderContext))

        ;

    TfPyRegisterStlSequencesFromPython<UsdShadeMaterial>();
    boost::python::to_python_converter<std::vector<UsdShadeMaterial>,
                        TfPySequenceToPython<std::vector<UsdShadeMaterial>>>();

}

} // anonymous namespace
