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
#include "pxr/usd/usdRi/lookAPI.h"
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

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;


} // anonymous namespace

void wrapUsdRiLookAPI()
{
    typedef UsdRiLookAPI This;

    class_<This, bases<UsdSchemaBase> >
        cls("LookAPI");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")


        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)


        
        .def("GetSurfaceRel",
             &This::GetSurfaceRel)
        .def("CreateSurfaceRel",
             &This::CreateSurfaceRel)
        
        .def("GetDisplacementRel",
             &This::GetDisplacementRel)
        .def("CreateDisplacementRel",
             &This::CreateDisplacementRel)
        
        .def("GetVolumeRel",
             &This::GetVolumeRel)
        .def("CreateVolumeRel",
             &This::CreateVolumeRel)
        
        .def("GetCoshadersRel",
             &This::GetCoshadersRel)
        .def("CreateCoshadersRel",
             &This::CreateCoshadersRel)
        
        .def("GetBxdfRel",
             &This::GetBxdfRel)
        .def("CreateBxdfRel",
             &This::CreateBxdfRel)
        
        .def("GetPatternsRel",
             &This::GetPatternsRel)
        .def("CreatePatternsRel",
             &This::CreatePatternsRel)
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

bool
_SetInterfaceRecipient0(
        UsdRiLookAPI& self,
        UsdShadeInterfaceAttribute& interfaceAttr,
        const SdfPath& receiverPath)
{
    return self.SetInterfaceRecipient(interfaceAttr, receiverPath);
}

bool
_SetInterfaceRecipient1(
        UsdRiLookAPI& self,
        UsdShadeInterfaceAttribute& interfaceAttr,
        const UsdShadeParameter& receiver)
{
    return self.SetInterfaceRecipient(interfaceAttr, receiver);
}

WRAP_CUSTOM {
    typedef UsdRiLookAPI This;
    _class
        .def(init<UsdShadeMaterial>(arg("material")))

        .def("GetSurface", &This::GetSurface)
        .def("GetDisplacement", &This::GetDisplacement)
        .def("GetVolume", &This::GetVolume)
        .def("GetCoshaders", &This::GetCoshaders,
             return_value_policy<TfPySequenceToList>())

        .def("GetBxdf", &This::GetBxdf)
        .def("GetPatterns", &This::GetPatterns,
             return_value_policy<TfPySequenceToList>())

        .def("SetInterfaceInputConsumer", &This::SetInterfaceInputConsumer)
        .def("ComputeInterfaceInputConsumersMap", 
            &This::ComputeInterfaceInputConsumersMap, 
            (arg("computeTransitiveConsumers")=false))
        .def("GetInterfaceInputs", &This::GetInterfaceInputs,
             return_value_policy<TfPySequenceToList>())

        // These are deprecated.
        .def("SetInterfaceRecipient", &_SetInterfaceRecipient0)
        .def("SetInterfaceRecipient", &_SetInterfaceRecipient1)
        .def("GetInterfaceRecipientParameters", &This::GetInterfaceRecipientParameters)

    ;
}

} // anonymous namespace
