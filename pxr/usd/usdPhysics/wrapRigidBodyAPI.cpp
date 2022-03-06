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
#include "pxr/usd/usdPhysics/rigidBodyAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>


PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdUsdPhysicsWrapRigidBodyAPI {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateRigidBodyEnabledAttr(UsdPhysicsRigidBodyAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateRigidBodyEnabledAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateKinematicEnabledAttr(UsdPhysicsRigidBodyAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateKinematicEnabledAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateStartsAsleepAttr(UsdPhysicsRigidBodyAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateStartsAsleepAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateVelocityAttr(UsdPhysicsRigidBodyAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateVelocityAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3f), writeSparsely);
}
        
static UsdAttribute
_CreateAngularVelocityAttr(UsdPhysicsRigidBodyAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
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

void wrapUsdPhysicsRigidBodyAPI()
{
    typedef UsdPhysicsRigidBodyAPI This;

    pxrUsdUsdPhysicsWrapRigidBodyAPI::UsdPhysicsRigidBodyAPI_CanApplyResult::Wrap<pxrUsdUsdPhysicsWrapRigidBodyAPI::UsdPhysicsRigidBodyAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    boost::python::class_<This, boost::python::bases<UsdAPISchemaBase> >
        cls("RigidBodyAPI");

    cls
        .def(boost::python::init<UsdPrim>(boost::python::arg("prim")))
        .def(boost::python::init<UsdSchemaBase const&>(boost::python::arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Get")

        .def("CanApply", &pxrUsdUsdPhysicsWrapRigidBodyAPI::_WrapCanApply, (boost::python::arg("prim")))
        .staticmethod("CanApply")

        .def("Apply", &This::Apply, (boost::python::arg("prim")))
        .staticmethod("Apply")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             boost::python::arg("includeInherited")=true,
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!boost::python::self)

        
        .def("GetRigidBodyEnabledAttr",
             &This::GetRigidBodyEnabledAttr)
        .def("CreateRigidBodyEnabledAttr",
             &pxrUsdUsdPhysicsWrapRigidBodyAPI::_CreateRigidBodyEnabledAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetKinematicEnabledAttr",
             &This::GetKinematicEnabledAttr)
        .def("CreateKinematicEnabledAttr",
             &pxrUsdUsdPhysicsWrapRigidBodyAPI::_CreateKinematicEnabledAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetStartsAsleepAttr",
             &This::GetStartsAsleepAttr)
        .def("CreateStartsAsleepAttr",
             &pxrUsdUsdPhysicsWrapRigidBodyAPI::_CreateStartsAsleepAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetVelocityAttr",
             &This::GetVelocityAttr)
        .def("CreateVelocityAttr",
             &pxrUsdUsdPhysicsWrapRigidBodyAPI::_CreateVelocityAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetAngularVelocityAttr",
             &This::GetAngularVelocityAttr)
        .def("CreateAngularVelocityAttr",
             &pxrUsdUsdPhysicsWrapRigidBodyAPI::_CreateAngularVelocityAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))

        
        .def("GetSimulationOwnerRel",
             &This::GetSimulationOwnerRel)
        .def("CreateSimulationOwnerRel",
             &This::CreateSimulationOwnerRel)
        .def("__repr__", pxrUsdUsdPhysicsWrapRigidBodyAPI::_Repr)
    ;

    pxrUsdUsdPhysicsWrapRigidBodyAPI::_CustomWrapCode(cls);
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

namespace pxrUsdUsdPhysicsWrapRigidBodyAPI {

    static boost::python::tuple
    ComputeMassPropertiesHelper( const UsdPhysicsRigidBodyAPI &self, const UsdPhysicsRigidBodyAPI::MassInformationFn& massInfoFn)
    {    
        GfVec3f diagonalInertia;
        GfVec3f com;
        GfQuatf principalAxes;
        float mass = self.ComputeMassProperties(&diagonalInertia, &com, &principalAxes, massInfoFn);        
        return boost::python::make_tuple( mass, diagonalInertia, com, principalAxes );
    }
        

    WRAP_CUSTOM {

        typedef UsdPhysicsRigidBodyAPI This;        

        TfPyFunctionFromPython<UsdPhysicsRigidBodyAPI::MassInformationFnSig>();

        boost::python::scope s = _class
            .def("ComputeMassProperties", ComputeMassPropertiesHelper)

        ;

        boost::python::class_<UsdPhysicsRigidBodyAPI::MassInformation>("MassInformation")
            .def_readwrite("volume", &UsdPhysicsRigidBodyAPI::MassInformation::volume)
            .def_readwrite("inertia", &UsdPhysicsRigidBodyAPI::MassInformation::inertia)
            .def_readwrite("centerOfMass", &UsdPhysicsRigidBodyAPI::MassInformation::centerOfMass)
            .def_readwrite("localPos", &UsdPhysicsRigidBodyAPI::MassInformation::localPos)
            .def_readwrite("localRot", &UsdPhysicsRigidBodyAPI::MassInformation::localRot)
        ;


    }

} // anonymous namespace 
