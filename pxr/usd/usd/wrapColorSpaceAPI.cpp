//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usd/colorSpaceAPI.h"
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
#include <nanobind/trampoline.h>
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
_CreateColorSpaceNameAttr(UsdColorSpaceAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateColorSpaceNameAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdColorSpaceAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "Usd.ColorSpaceAPI(%s)",
        primRepr.c_str());
}

struct UsdColorSpaceAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdColorSpaceAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdColorSpaceAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdColorSpaceAPI::CanApply(prim, &whyNot);
    return UsdColorSpaceAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdColorSpaceAPI(nb::module_ &m)
{
    typedef UsdColorSpaceAPI This;

    UsdColorSpaceAPI_CanApplyResult::WrapNb<UsdColorSpaceAPI_CanApplyResult>(m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "ColorSpaceAPI");

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

        
        .def("GetColorSpaceNameAttr",
             &This::GetColorSpaceNameAttr)
        .def("CreateColorSpaceNameAttr",
             &_CreateColorSpaceNameAttr,
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

namespace {

// Wrapper class to handle the abstract ColorSpaceCache
struct ColorSpaceCacheWrapper : UsdColorSpaceAPI::ColorSpaceCache {
    NB_TRAMPOLINE(UsdColorSpaceAPI::ColorSpaceCache, 2);

    TfToken Find(const SdfPath& prim) override {
        NB_OVERRIDE_PURE(Find, prim);
    }

    void Insert(const SdfPath& prim, const TfToken& colorSpace) override {
        NB_OVERRIDE_PURE(Insert, prim, colorSpace);
    }
};

void wrapColorSpaceCache() {
    typedef UsdColorSpaceAPI::ColorSpaceCache This;
    nb::module_ m = TfPyGetCurrentNanobindModule();

    nb::class_<This, ColorSpaceCacheWrapper>(m, "ColorSpaceCache")
        .def(nb::init<>())
        .def("Find", &This::Find)
        .def("Insert", &This::Insert)
    ;
}

// Wrapper class for ColorSpaceHashCache which inherits ColorSpaceCache
struct ColorSpaceHashCacheWrapper : UsdColorSpaceAPI::ColorSpaceHashCache {
    NB_TRAMPOLINE(UsdColorSpaceAPI::ColorSpaceHashCache, 2);

    TfToken Find(const SdfPath& prim) override {
        NB_OVERRIDE(Find, prim);
    }

    void Insert(const SdfPath& prim, const TfToken& colorSpace) override {
        NB_OVERRIDE(Insert, prim, colorSpace);
    }
};

void wrapColorSpaceHashCache() {
    typedef UsdColorSpaceAPI::ColorSpaceHashCache This;
    nb::module_ m = TfPyGetCurrentNanobindModule();

    nb::class_<This, UsdColorSpaceAPI::ColorSpaceCache, ColorSpaceHashCacheWrapper>(
        m, "ColorSpaceHashCache")
        .def(nb::init<>())
        .def("Find", &This::Find)
        .def("Insert", &This::Insert)
    ;
}

WRAP_CUSTOM {

    wrapColorSpaceCache();
    wrapColorSpaceHashCache();

    _class
        .def_static("ComputeColorSpaceName", 
                (TfToken (*)(UsdPrim, 
                             UsdColorSpaceAPI::ColorSpaceCache*)) 
                &UsdColorSpaceAPI::ComputeColorSpaceName,
                nb::arg("prim"), nb::arg("cache") = nullptr)
        .def_static("ComputeColorSpaceName",
                (TfToken (*)(const UsdAttribute&, 
                             UsdColorSpaceAPI::ColorSpaceCache*))
                &UsdColorSpaceAPI::ComputeColorSpaceName,
                nb::arg("attribute"), nb::arg("cache") = nullptr)
        .def_static("ComputeColorSpace",
                (GfColorSpace (*)(UsdPrim, 
                                  UsdColorSpaceAPI::ColorSpaceCache*))
                &UsdColorSpaceAPI::ComputeColorSpace,
                nb::arg("prim"), nb::arg("cache") = nullptr)
        .def_static("ComputeColorSpace",
                (GfColorSpace (*)(const UsdAttribute&,
                                  UsdColorSpaceAPI::ColorSpaceCache*))
                &UsdColorSpaceAPI::ComputeColorSpace,
                nb::arg("attribute"), nb::arg("cache") = nullptr)
        .def_static("IsValidColorSpaceName",
                (bool (*)(UsdPrim, const TfToken&, 
                          UsdColorSpaceAPI::ColorSpaceCache*))
                &UsdColorSpaceAPI::IsValidColorSpaceName,
                nb::arg("prim"), nb::arg("colorSpace"), nb::arg("cache") = nullptr)
    ;
}

}
