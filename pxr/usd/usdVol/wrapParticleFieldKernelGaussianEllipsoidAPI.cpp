//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/particleFieldKernelGaussianEllipsoidAPI.h"
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

static std::string
_Repr(const UsdVolParticleFieldKernelGaussianEllipsoidAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdVol.ParticleFieldKernelGaussianEllipsoidAPI(%s)",
        primRepr.c_str());
}

struct UsdVolParticleFieldKernelGaussianEllipsoidAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdVolParticleFieldKernelGaussianEllipsoidAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdVolParticleFieldKernelGaussianEllipsoidAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdVolParticleFieldKernelGaussianEllipsoidAPI::CanApply(prim, &whyNot);
    return UsdVolParticleFieldKernelGaussianEllipsoidAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdVolParticleFieldKernelGaussianEllipsoidAPI(nb::module_ &m)
{
    typedef UsdVolParticleFieldKernelGaussianEllipsoidAPI This;

    UsdVolParticleFieldKernelGaussianEllipsoidAPI_CanApplyResult::WrapNb<UsdVolParticleFieldKernelGaussianEllipsoidAPI_CanApplyResult>(m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "ParticleFieldKernelGaussianEllipsoidAPI");

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

        .def("ParticleFieldKernelBaseAPI", &This::ParticleFieldKernelBaseAPI)
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
