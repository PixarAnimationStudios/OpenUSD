//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/particleFieldSphericalHarmonicsAttributeAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
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
_CreateRadianceSphericalHarmonicsDegreeAttr(UsdVolParticleFieldSphericalHarmonicsAttributeAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRadianceSphericalHarmonicsDegreeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateRadianceSphericalHarmonicsCoefficientsAttr(UsdVolParticleFieldSphericalHarmonicsAttributeAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRadianceSphericalHarmonicsCoefficientsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3Array), writeSparsely);
}
        
static UsdAttribute
_CreateRadianceSphericalHarmonicsCoefficientshAttr(UsdVolParticleFieldSphericalHarmonicsAttributeAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRadianceSphericalHarmonicsCoefficientshAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Half3Array), writeSparsely);
}

static std::string
_Repr(const UsdVolParticleFieldSphericalHarmonicsAttributeAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdVol.ParticleFieldSphericalHarmonicsAttributeAPI(%s)",
        primRepr.c_str());
}

struct UsdVolParticleFieldSphericalHarmonicsAttributeAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdVolParticleFieldSphericalHarmonicsAttributeAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdVolParticleFieldSphericalHarmonicsAttributeAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdVolParticleFieldSphericalHarmonicsAttributeAPI::CanApply(prim, &whyNot);
    return UsdVolParticleFieldSphericalHarmonicsAttributeAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdVolParticleFieldSphericalHarmonicsAttributeAPI()
{
    typedef UsdVolParticleFieldSphericalHarmonicsAttributeAPI This;

    UsdVolParticleFieldSphericalHarmonicsAttributeAPI_CanApplyResult::Wrap<UsdVolParticleFieldSphericalHarmonicsAttributeAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    class_<This, bases<UsdAPISchemaBase> >
        cls("ParticleFieldSphericalHarmonicsAttributeAPI");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("CanApply", &_WrapCanApply, (arg("prim")))
        .staticmethod("CanApply")

        .def("Apply", &This::Apply, (arg("prim")))
        .staticmethod("Apply")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetRadianceSphericalHarmonicsDegreeAttr",
             &This::GetRadianceSphericalHarmonicsDegreeAttr)
        .def("CreateRadianceSphericalHarmonicsDegreeAttr",
             &_CreateRadianceSphericalHarmonicsDegreeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRadianceSphericalHarmonicsCoefficientsAttr",
             &This::GetRadianceSphericalHarmonicsCoefficientsAttr)
        .def("CreateRadianceSphericalHarmonicsCoefficientsAttr",
             &_CreateRadianceSphericalHarmonicsCoefficientsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRadianceSphericalHarmonicsCoefficientshAttr",
             &This::GetRadianceSphericalHarmonicsCoefficientshAttr)
        .def("CreateRadianceSphericalHarmonicsCoefficientshAttr",
             &_CreateRadianceSphericalHarmonicsCoefficientshAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))


        .def("ParticleFieldRadianceBaseAPI", &This::ParticleFieldRadianceBaseAPI)
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
