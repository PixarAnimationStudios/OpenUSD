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
#include "pxr/usd/usdSkel/bindingAPI.h"
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
_CreateGeomBindTransformAttr(UsdSkelBindingAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateGeomBindTransformAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4d), writeSparsely);
}
        
static UsdAttribute
_CreateJointsAttr(UsdSkelBindingAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateJointsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TokenArray), writeSparsely);
}
        
static UsdAttribute
_CreateJointIndicesAttr(UsdSkelBindingAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateJointIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateJointWeightsAttr(UsdSkelBindingAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateJointWeightsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}

} // anonymous namespace

void wrapUsdSkelBindingAPI()
{
    typedef UsdSkelBindingAPI This;

    class_<This, bases<UsdAPISchemaBase> >
        cls("BindingAPI");

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

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetGeomBindTransformAttr",
             &This::GetGeomBindTransformAttr)
        .def("CreateGeomBindTransformAttr",
             &_CreateGeomBindTransformAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetJointsAttr",
             &This::GetJointsAttr)
        .def("CreateJointsAttr",
             &_CreateJointsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetJointIndicesAttr",
             &This::GetJointIndicesAttr)
        .def("CreateJointIndicesAttr",
             &_CreateJointIndicesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetJointWeightsAttr",
             &This::GetJointWeightsAttr)
        .def("CreateJointWeightsAttr",
             &_CreateJointWeightsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        
        .def("GetAnimationSourceRel",
             &This::GetAnimationSourceRel)
        .def("CreateAnimationSourceRel",
             &This::CreateAnimationSourceRel)
        
        .def("GetSkeletonRel",
             &This::GetSkeletonRel)
        .def("CreateSkeletonRel",
             &This::CreateSkeletonRel)
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
    using This = UsdSkelBindingAPI;

    _class
        .def("GetJointIndicesPrimvar", &This::GetJointIndicesPrimvar)

        .def("CreateJointIndicesPrimvar", &This::CreateJointIndicesPrimvar,
             (arg("constant"), arg("elementSize")=-1))

        .def("GetJointWeightsPrimvar", &This::GetJointWeightsPrimvar)

        .def("CreateJointWeightsPrimvar", &This::CreateJointWeightsPrimvar,
             (arg("constant"), arg("elementSize")=-1))

        .def("SetRigidJointInfluence", &This::SetRigidJointInfluence,
             (arg("jointIndex"), arg("weight")=1.0f))
        ;
}

} // namespace
