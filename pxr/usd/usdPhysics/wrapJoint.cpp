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
#include "pxr/usd/usdPhysics/joint.h"
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

namespace pxrUsdUsdPhysicsWrapJoint {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateLocalPos0Attr(UsdPhysicsJoint &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateLocalPos0Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3f), writeSparsely);
}
        
static UsdAttribute
_CreateLocalRot0Attr(UsdPhysicsJoint &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateLocalRot0Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Quatf), writeSparsely);
}
        
static UsdAttribute
_CreateLocalPos1Attr(UsdPhysicsJoint &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateLocalPos1Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3f), writeSparsely);
}
        
static UsdAttribute
_CreateLocalRot1Attr(UsdPhysicsJoint &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateLocalRot1Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Quatf), writeSparsely);
}
        
static UsdAttribute
_CreateJointEnabledAttr(UsdPhysicsJoint &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateJointEnabledAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateCollisionEnabledAttr(UsdPhysicsJoint &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateCollisionEnabledAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateExcludeFromArticulationAttr(UsdPhysicsJoint &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateExcludeFromArticulationAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateBreakForceAttr(UsdPhysicsJoint &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateBreakForceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateBreakTorqueAttr(UsdPhysicsJoint &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateBreakTorqueAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}

static std::string
_Repr(const UsdPhysicsJoint &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdPhysics.Joint(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdPhysicsJoint()
{
    typedef UsdPhysicsJoint This;

    boost::python::class_<This, boost::python::bases<UsdGeomImageable> >
        cls("Joint");

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

        
        .def("GetLocalPos0Attr",
             &This::GetLocalPos0Attr)
        .def("CreateLocalPos0Attr",
             &pxrUsdUsdPhysicsWrapJoint::_CreateLocalPos0Attr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetLocalRot0Attr",
             &This::GetLocalRot0Attr)
        .def("CreateLocalRot0Attr",
             &pxrUsdUsdPhysicsWrapJoint::_CreateLocalRot0Attr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetLocalPos1Attr",
             &This::GetLocalPos1Attr)
        .def("CreateLocalPos1Attr",
             &pxrUsdUsdPhysicsWrapJoint::_CreateLocalPos1Attr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetLocalRot1Attr",
             &This::GetLocalRot1Attr)
        .def("CreateLocalRot1Attr",
             &pxrUsdUsdPhysicsWrapJoint::_CreateLocalRot1Attr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetJointEnabledAttr",
             &This::GetJointEnabledAttr)
        .def("CreateJointEnabledAttr",
             &pxrUsdUsdPhysicsWrapJoint::_CreateJointEnabledAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetCollisionEnabledAttr",
             &This::GetCollisionEnabledAttr)
        .def("CreateCollisionEnabledAttr",
             &pxrUsdUsdPhysicsWrapJoint::_CreateCollisionEnabledAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetExcludeFromArticulationAttr",
             &This::GetExcludeFromArticulationAttr)
        .def("CreateExcludeFromArticulationAttr",
             &pxrUsdUsdPhysicsWrapJoint::_CreateExcludeFromArticulationAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetBreakForceAttr",
             &This::GetBreakForceAttr)
        .def("CreateBreakForceAttr",
             &pxrUsdUsdPhysicsWrapJoint::_CreateBreakForceAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetBreakTorqueAttr",
             &This::GetBreakTorqueAttr)
        .def("CreateBreakTorqueAttr",
             &pxrUsdUsdPhysicsWrapJoint::_CreateBreakTorqueAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))

        
        .def("GetBody0Rel",
             &This::GetBody0Rel)
        .def("CreateBody0Rel",
             &This::CreateBody0Rel)
        
        .def("GetBody1Rel",
             &This::GetBody1Rel)
        .def("CreateBody1Rel",
             &This::CreateBody1Rel)
        .def("__repr__", pxrUsdUsdPhysicsWrapJoint::_Repr)
    ;

    pxrUsdUsdPhysicsWrapJoint::_CustomWrapCode(cls);
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

namespace pxrUsdUsdPhysicsWrapJoint {

WRAP_CUSTOM {
}

}
