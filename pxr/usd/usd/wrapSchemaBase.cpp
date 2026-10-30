//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/vector.h>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

// We override __getattribute__ for UsdSchemaBase to check object validity
// and raise an exception instead of crashing from Python.

// Store the original __getattribute__ so we can dispatch to it after verifying
// validity. Note that this TfPyObjWrapper is intentionally leaked to avoid
// running Python refcount operations in its d'tor during process shutdown, 
// which is unsafe if Python has been finalized.
static TfStaticData<TfPyObjWrapper> _object__getattribute__;

// This function gets wrapped as __getattribute__ on UsdSchemaBase.
static nanobind::object
__getattribute__(nanobind::object selfObj, const char *name) {
    // Allow attribute lookups if the attribute name starts with '__', or
    // if the object's prim is valid. Also add explicit exceptions for every
    // method on this base class. The real purpose here is to protect against
    // invalid calls in subclasses which will try to actually manipulate the
    // underlying (invalid) prim and likely crash.
    if ((name[0] == '_' && name[1] == '_') ||
        nanobind::cast<UsdSchemaBase &>(selfObj).GetPrim().IsValid() ||
        strcmp(name, "GetPrim") == 0 ||
        strcmp(name, "GetPath") == 0 ||
        strcmp(name, "GetSchemaClassPrimDefinition") == 0 ||
        strcmp(name, "GetSchemaAttributeNames") == 0 ||
        strcmp(name, "GetSchemaKind") == 0 ||
        strcmp(name, "IsAPISchema") == 0 ||
        strcmp(name, "IsConcrete") == 0 ||
        strcmp(name, "IsTyped") == 0 ||
        strcmp(name, "IsAppliedAPISchema") == 0 ||
        strcmp(name, "IsMultipleApplyAPISchema") == 0) {
        // Dispatch to object's __getattribute__.
        return nanobind::borrow<nanobind::object>(
            _object__getattribute__->ptr())(selfObj, name);
    } else {
        // Otherwise raise a runtime error.
        TfPyThrowRuntimeError(
            TfStringPrintf("Accessed schema on invalid prim"));
    }
    // Unreachable.
    return nanobind::none();
}

void wrapUsdSchemaBase(nanobind::module_ &m)
{
    namespace nb = nanobind;
    nb::class_<UsdSchemaBase> cls(m, "SchemaBase");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim") = UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("otherSchema"))
        .def(TfTypePythonClass())

        .def("GetPrim", &UsdSchemaBase::GetPrim)
        .def("GetPath", &UsdSchemaBase::GetPath)
        .def("GetSchemaClassPrimDefinition",
             &UsdSchemaBase::GetSchemaClassPrimDefinition,
             nb::rv_policy::reference_internal)
        .def_static("GetSchemaAttributeNames",
             &UsdSchemaBase::GetSchemaAttributeNames,
             nb::arg("includeInherited") = true)

        .def("IsAPISchema", &UsdSchemaBase::IsAPISchema)
        .def("IsConcrete", &UsdSchemaBase::IsConcrete) 
        .def("IsTyped", &UsdSchemaBase::IsTyped) 
        .def("IsAppliedAPISchema", &UsdSchemaBase::IsAppliedAPISchema) 
        .def("IsMultipleApplyAPISchema", &UsdSchemaBase::IsMultipleApplyAPISchema) 

        .def("GetSchemaKind", &UsdSchemaBase::GetSchemaKind)

        .def("__bool__", [](const UsdSchemaBase &self) { return bool(self); })

        ;

    // Save existing __getattribute__ and replace.
    *_object__getattribute__ = TfPyObjWrapper(cls.attr("__getattribute__"));
    cls.def("__getattribute__", __getattribute__);

}
