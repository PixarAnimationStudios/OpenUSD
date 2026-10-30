//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

#include "pxr/pxr.h"
#include "pxr/usdValidation/usdValidation/fixer.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usd/usd/editTarget.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyObjWrapper.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"

#include <nanobind/stl/string.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace
{

// ---------------------------------------------------------------------------
// Python callable → C++ fixer function wrappers
//
// The Python callable is stored in a TfPyObjWrapper and the GIL is acquired
// before invoking it.
//
// FixerImplFn and FixerCanApplyFn share the same signature
// (UsdValidationError, UsdEditTarget, UsdTimeCode) -> bool, so a single
// wrapper function handles both.

static std::function<bool(const UsdValidationError &,
                          const UsdEditTarget &,
                          const UsdTimeCode &)>
_WrapFixerFn(nb::object pyFn)
{
    TfPyObjWrapper wrapper(pyFn);
    return [wrapper](
        const UsdValidationError &error,
        const UsdEditTarget &editTarget,
        const UsdTimeCode &timeCode) -> bool {
        TfPyLock lock;
        try {
            nb::object fn = wrapper.Get();
            nb::object result = fn(
                nb::cast(error),
                nb::cast(editTarget),
                nb::cast(timeCode));
            return nb::cast<bool>(result);
        }
        catch (nb::python_error &e) {
            e.restore();
            TfPyConvertPythonExceptionToTfErrors();
            PyErr_Clear();
            return false;
        }
    };
}

} // anonymous namespace

void wrapUsdValidationFixer(nb::module_ &m)
{
    nb::class_<UsdValidationFixer>(m, "ValidationFixer")
        .def("__init__",
             [](UsdValidationFixer *self,
                const TfToken &name,
                const std::string &description,
                nb::object fixerImplFn,
                nb::object canApplyFn,
                const TfTokenVector &keywords,
                const TfToken &errorName) {
                 if (!PyCallable_Check(fixerImplFn.ptr()) ||
                     !PyCallable_Check(canApplyFn.ptr())) {
                     TfPyThrowTypeError("fixerImplFn and canApplyFn must be callable");
                 }
                 new (self) UsdValidationFixer(
                     name, description,
                     _WrapFixerFn(fixerImplFn),
                     _WrapFixerFn(canApplyFn),
                     keywords, errorName);
             },
             nb::arg("name"), nb::arg("description"),
             nb::arg("fixerImplFn"), nb::arg("canApplyFn"),
             nb::arg("keywords") = TfTokenVector(),
             nb::arg("errorName") = TfToken())
        .def_prop_ro("name",
            +[](const UsdValidationFixer &fixer) { 
                  return fixer.GetName(); 
            })
        .def_prop_ro("description",
            +[](const UsdValidationFixer &fixer) { 
                return fixer.GetDescription(); 
            })
        .def_prop_ro("errorName",
            +[](const UsdValidationFixer &fixer) { 
                return fixer.GetErrorName(); 
            })
        .def_prop_ro("keywords",
            +[](const UsdValidationFixer &fixer) { 
                return fixer.GetKeywords(); 
            })
        .def("IsAssociatedWithErrorName", 
             &UsdValidationFixer::IsAssociatedWithErrorName, nb::arg("errorName"))
        .def("HasKeyword", 
             &UsdValidationFixer::HasKeyword, nb::arg("keyword"))
        .def("CanApplyFix", 
             &UsdValidationFixer::CanApplyFix,
             nb::arg("error"), nb::arg("editTarget"), 
             nb::arg("timeCode") = UsdTimeCode::Default())
        .def("ApplyFix", 
             &UsdValidationFixer::ApplyFix,
             nb::arg("error"), nb::arg("editTarget"), 
             nb::arg("timeCode") = UsdTimeCode::Default());

    TfPyRegisterStlSequencesFromPython<UsdValidationFixer>();
    TfPyRegisterStlSequencesFromPython<const UsdValidationFixer*>();
}
