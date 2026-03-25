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

FixerImplFn
_WrapFixerImplFn(pxr_boost::python::object pyFn)
{
    TfPyObjWrapper wrapper(pyFn);
    return [wrapper](const UsdValidationError &error,
                     const UsdEditTarget &editTarget,
                     const UsdTimeCode &timeCode) -> bool {
        TfPyLock lock;
        try {
            pxr_boost::python::object result = wrapper.Get()(
                pxr_boost::python::object(error),
                pxr_boost::python::object(editTarget),
                pxr_boost::python::object(timeCode));
            return pxr_boost::python::extract<bool>(result);
        }
        catch (pxr_boost::python::error_already_set const &) {
            TfPyConvertPythonExceptionToTfErrors();
            PyErr_Clear();
            return false;
        }
    };
}

FixerCanApplyFn
_WrapFixerCanApplyFn(pxr_boost::python::object pyFn)
{
    TfPyObjWrapper wrapper(pyFn);
    return [wrapper](const UsdValidationError &error,
                     const UsdEditTarget &editTarget,
                     const UsdTimeCode &timeCode) -> bool {
        TfPyLock lock;
        try {
            pxr_boost::python::object result = wrapper.Get()(
                pxr_boost::python::object(error),
                pxr_boost::python::object(editTarget),
                pxr_boost::python::object(timeCode));
            return pxr_boost::python::extract<bool>(result);
        }
        catch (pxr_boost::python::error_already_set const &) {
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
                     pxr_boost::python::object fixerImplFn,
                     pxr_boost::python::object canApplyFn,
                     const TfTokenVector &keywords,
                     const TfToken &errorName)
{
    return new UsdValidationFixer(
        name, description,
        _WrapFixerImplFn(fixerImplFn),
        _WrapFixerCanApplyFn(canApplyFn),
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
