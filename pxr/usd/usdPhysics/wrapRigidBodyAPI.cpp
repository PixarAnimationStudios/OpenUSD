//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdPhysics/rigidBodyAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
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
_CreateRigidBodyEnabledAttr(UsdPhysicsRigidBodyAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateRigidBodyEnabledAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateKinematicEnabledAttr(UsdPhysicsRigidBodyAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateKinematicEnabledAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateStartsAsleepAttr(UsdPhysicsRigidBodyAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateStartsAsleepAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateVelocityAttr(UsdPhysicsRigidBodyAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateVelocityAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3f), writeSparsely);
}
        
static UsdAttribute
_CreateAngularVelocityAttr(UsdPhysicsRigidBodyAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateAngularVelocityAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3f), writeSparsely);
}

static std::string
_Repr(const UsdPhysicsRigidBodyAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdPhysics.RigidBodyAPI(%s)",
        primRepr.c_str());
}

struct UsdPhysicsRigidBodyAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdPhysicsRigidBodyAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdPhysicsRigidBodyAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdPhysicsRigidBodyAPI::CanApply(prim, &whyNot);
    return UsdPhysicsRigidBodyAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdPhysicsRigidBodyAPI(nb::module_ &m)
{
    typedef UsdPhysicsRigidBodyAPI This;

    UsdPhysicsRigidBodyAPI_CanApplyResult::WrapNb<UsdPhysicsRigidBodyAPI_CanApplyResult>(m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "RigidBodyAPI");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("CanApply", &_WrapCanApply, nb::arg("prim"))

        .def_static("Apply", &This::Apply, nb::arg("prim"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetRigidBodyEnabledAttr",
             &This::GetRigidBodyEnabledAttr)
        .def("CreateRigidBodyEnabledAttr",
             &_CreateRigidBodyEnabledAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetKinematicEnabledAttr",
             &This::GetKinematicEnabledAttr)
        .def("CreateKinematicEnabledAttr",
             &_CreateKinematicEnabledAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetStartsAsleepAttr",
             &This::GetStartsAsleepAttr)
        .def("CreateStartsAsleepAttr",
             &_CreateStartsAsleepAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetVelocityAttr",
             &This::GetVelocityAttr)
        .def("CreateVelocityAttr",
             &_CreateVelocityAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetAngularVelocityAttr",
             &This::GetAngularVelocityAttr)
        .def("CreateAngularVelocityAttr",
             &_CreateAngularVelocityAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        
        .def("GetSimulationOwnerRel",
             &This::GetSimulationOwnerRel)
        .def("CreateSimulationOwnerRel",
             &This::CreateSimulationOwnerRel)
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

#include "pxr/base/tf/pyFunction.h"

namespace {

    static nb::tuple
    ComputeMassPropertiesHelper( const UsdPhysicsRigidBodyAPI &self, const UsdPhysicsRigidBodyAPI::MassInformationFn& massInfoFn)
    {    
        GfVec3f diagonalInertia;
        GfVec3f com;
        GfQuatf principalAxes;
        float mass = self.ComputeMassProperties(&diagonalInertia, &com, &principalAxes, massInfoFn);        
        return nb::make_tuple( mass, diagonalInertia, com, principalAxes );
    }
        

    WRAP_CUSTOM {

        typedef UsdPhysicsRigidBodyAPI This;        

        TfPyFunctionFromPython<UsdPhysicsRigidBodyAPI::MassInformationFnSig>();

        nb::class_<UsdPhysicsRigidBodyAPI::MassInformation>(_class, "MassInformation")
            .def(nb::init<>())
            .def_rw("volume", &UsdPhysicsRigidBodyAPI::MassInformation::volume)
            .def_rw("inertia", &UsdPhysicsRigidBodyAPI::MassInformation::inertia)
            .def_rw("centerOfMass", &UsdPhysicsRigidBodyAPI::MassInformation::centerOfMass)
            .def_rw("localPos", &UsdPhysicsRigidBodyAPI::MassInformation::localPos)
            .def_rw("localRot", &UsdPhysicsRigidBodyAPI::MassInformation::localRot)
        ;

        _class
            .def("ComputeMassProperties", ComputeMassPropertiesHelper)

        ;
    }

} // anonymous namespace 
