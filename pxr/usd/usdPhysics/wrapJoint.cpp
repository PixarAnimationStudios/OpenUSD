//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdPhysics/joint.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/external/boost/python.hpp"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateLocalPos0Attr(UsdPhysicsJoint &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateLocalPos0Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3f), writeSparsely);
}
        
static UsdAttribute
_CreateLocalRot0Attr(UsdPhysicsJoint &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateLocalRot0Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Quatf), writeSparsely);
}
        
static UsdAttribute
_CreateLocalPos1Attr(UsdPhysicsJoint &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateLocalPos1Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3f), writeSparsely);
}
        
static UsdAttribute
_CreateLocalRot1Attr(UsdPhysicsJoint &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateLocalRot1Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Quatf), writeSparsely);
}
        
static UsdAttribute
_CreateJointEnabledAttr(UsdPhysicsJoint &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateJointEnabledAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateCollisionEnabledAttr(UsdPhysicsJoint &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCollisionEnabledAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateExcludeFromArticulationAttr(UsdPhysicsJoint &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateExcludeFromArticulationAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateBreakForceAttr(UsdPhysicsJoint &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateBreakForceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateBreakTorqueAttr(UsdPhysicsJoint &self,
                                      object defaultVal, bool writeSparsely) {
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

    class_<This, bases<UsdGeomImageable> >
        cls("Joint");

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

        
        .def("GetLocalPos0Attr",
             &This::GetLocalPos0Attr)
        .def("CreateLocalPos0Attr",
             &_CreateLocalPos0Attr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetLocalRot0Attr",
             &This::GetLocalRot0Attr)
        .def("CreateLocalRot0Attr",
             &_CreateLocalRot0Attr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetLocalPos1Attr",
             &This::GetLocalPos1Attr)
        .def("CreateLocalPos1Attr",
             &_CreateLocalPos1Attr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetLocalRot1Attr",
             &This::GetLocalRot1Attr)
        .def("CreateLocalRot1Attr",
             &_CreateLocalRot1Attr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetJointEnabledAttr",
             &This::GetJointEnabledAttr)
        .def("CreateJointEnabledAttr",
             &_CreateJointEnabledAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCollisionEnabledAttr",
             &This::GetCollisionEnabledAttr)
        .def("CreateCollisionEnabledAttr",
             &_CreateCollisionEnabledAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetExcludeFromArticulationAttr",
             &This::GetExcludeFromArticulationAttr)
        .def("CreateExcludeFromArticulationAttr",
             &_CreateExcludeFromArticulationAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetBreakForceAttr",
             &This::GetBreakForceAttr)
        .def("CreateBreakForceAttr",
             &_CreateBreakForceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetBreakTorqueAttr",
             &This::GetBreakTorqueAttr)
        .def("CreateBreakTorqueAttr",
             &_CreateBreakTorqueAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        
        .def("GetBody0Rel",
             &This::GetBody0Rel)
        .def("CreateBody0Rel",
             &This::CreateBody0Rel)
        
        .def("GetBody1Rel",
             &This::GetBody1Rel)
        .def("CreateBody1Rel",
             &This::CreateBody1Rel)
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

WRAP_CUSTOM {
}

}
