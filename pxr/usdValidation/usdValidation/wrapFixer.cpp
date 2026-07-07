//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
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

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/make_constructor.hpp"
#include "pxr/external/boost/python/object.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace
{

// ---------------------------------------------------------------------------
// Python callable → C++ fixer function wrappers
//
// These follow the same GIL-safety pattern as the validator task function
// wrappers in wrapRegistry.cpp: the Python callable is stored in a
// TfPyObjWrapper, and the GIL is acquired before invoking it.
//
// FixerImplFn and FixerCanApplyFn share the same signature
// (UsdValidationError, UsdEditTarget, UsdTimeCode) -> bool, so a single
// wrapper function handles both.

static std::function<bool(const UsdValidationError &,
                          const UsdEditTarget &,
                          const UsdTimeCode &)>
_WrapFixerFn(object pyFn)
{
    return [wrapper = TfPyObjWrapper(pyFn)](
        const UsdValidationError &error,
        const UsdEditTarget &editTarget,
        const UsdTimeCode &timeCode) -> bool {
        TfPyLock lock;
        try {
            object result = wrapper.Get()(
                object(error),
                object(editTarget),
                object(timeCode));
            return extract<bool>(result);
        }
        catch (error_already_set const &) {
            TfPyConvertPythonExceptionToTfErrors();
            PyErr_Clear();
            return false;
        }
    };
}

// Construct a UsdValidationFixer from Python arguments, wrapping the
// Python callables into C++ std::functions with GIL safety.
// Returns a heap-allocated pointer as required by make_constructor.
UsdValidationFixer *
_MakeValidationFixer(const TfToken &name,
                     const std::string &description,
                     object fixerImplFn,
                     object canApplyFn,
                     const TfTokenVector &keywords,
                     const TfToken &errorName)
{
    return new UsdValidationFixer(
        name, description,
        _WrapFixerFn(fixerImplFn),
        _WrapFixerFn(canApplyFn),
        keywords, errorName);
}

} // anonymous namespace

void wrapUsdValidationFixer()
{
    class_<UsdValidationFixer>("ValidationFixer", no_init)
        .def("__init__",
             make_constructor(
                 &_MakeValidationFixer,
                 default_call_policies(),
                 (arg("name"), arg("description"),
                  arg("fixerImplFn"), arg("canApplyFn"),
                  arg("keywords") = TfTokenVector(),
                  arg("errorName") = TfToken())))
        .add_property("name",
            make_function(
                +[](const UsdValidationFixer &fixer) { 
                      return fixer.GetName(); 
                },
                return_value_policy<return_by_value>()))
        .add_property("description",
            make_function(
                +[](const UsdValidationFixer &fixer) { 
                    return fixer.GetDescription(); 
                },
                return_value_policy<return_by_value>()))
        .add_property("errorName",
            make_function(
                +[](const UsdValidationFixer &fixer) { 
                    return fixer.GetErrorName(); 
                },
                return_value_policy<return_by_value>()))
        .add_property("keywords",
            make_function(
                +[](const UsdValidationFixer &fixer) { 
                    return fixer.GetKeywords(); 
                },
                return_value_policy<TfPySequenceToList>()))
        .def("IsAssociatedWithErrorName", 
             &UsdValidationFixer::IsAssociatedWithErrorName, (arg("errorName")))
        .def("HasKeyword", 
             &UsdValidationFixer::HasKeyword, (arg("keyword")))
        .def("CanApplyFix", 
             &UsdValidationFixer::CanApplyFix,
             (arg("error"), arg("editTarget"), 
              arg("timeCode") = UsdTimeCode::Default()))
        .def("ApplyFix", 
             &UsdValidationFixer::ApplyFix,
             (arg("error"), arg("editTarget"), 
              arg("timeCode") = UsdTimeCode::Default()));

    TfPyRegisterStlSequencesFromPython<UsdValidationFixer>();
    TfPyRegisterStlSequencesFromPython<const UsdValidationFixer*>();
}
