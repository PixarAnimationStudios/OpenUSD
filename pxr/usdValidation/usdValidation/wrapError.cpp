//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/vector.h>

#include "pxr/pxr.h"
#include "pxr/usd/usd/pyConversions.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/validator.h"
#include "pxr/usdValidation/usdValidation/fixer.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

    nb::list
    _GetFixers(const UsdValidationError &error) {
        nb::list result;
        for (const auto *fixer : error.GetFixers()) {
            result.append(nb::cast(fixer, nb::rv_policy::reference));
        }
        return result;
    }

    nb::list
    _GetFixersByErrorName(const UsdValidationError &error) {
        nb::list result;
        for (const auto *fixer : error.GetFixersByErrorName()) {
            result.append(nb::cast(fixer, nb::rv_policy::reference));
        }
        return result;
    }

    nb::list
    _GetFixersByKeywords(const UsdValidationError &error, 
                         const TfTokenVector &keywords) {
        nb::list result;
        for (const auto *fixer : error.GetFixersByKeywords(keywords)) {
            result.append(nb::cast(fixer, nb::rv_policy::reference));
        }
        return result;
    }
} // anonymous namespace

void wrapUsdValidationError(nb::module_ &m)
{
    TfNbWrapEnum<UsdValidationErrorType>(m, "ValidationErrorType");

    nb::class_<UsdValidationErrorSite>(m, "ValidationErrorSite")
        .def(nb::init<>())
        .def(nb::init<const SdfLayerHandle &, const SdfPath &>(),
             nb::arg("layer"), nb::arg("objectPath"))
        .def(nb::init<const UsdStagePtr &, const SdfPath &, const SdfLayerHandle &>(),
             nb::arg("stage"), nb::arg("objectPath"), nb::arg("layer") = SdfLayerHandle{})
        .def("IsValid", &UsdValidationErrorSite::IsValid)
        .def("IsValidSpecInLayer", &UsdValidationErrorSite::IsValidSpecInLayer)
        .def("IsPrim", &UsdValidationErrorSite::IsPrim)
        .def("IsProperty", &UsdValidationErrorSite::IsProperty)
        .def("GetPropertySpec", &UsdValidationErrorSite::GetPropertySpec)
        .def("GetPrimSpec", &UsdValidationErrorSite::GetPrimSpec)
        .def("GetLayer", &UsdValidationErrorSite::GetLayer)
        .def("GetStage", &UsdValidationErrorSite::GetStage)
        .def("GetPrim", &UsdValidationErrorSite::GetPrim)
        .def("GetProperty", &UsdValidationErrorSite::GetProperty)
        .def(nb::self == nb::self)
        .def(nb::self != nb::self);

    TfPyRegisterStlSequencesFromPython<UsdValidationErrorSite>();
    nb::class_<UsdValidationError>(m, "ValidationError")
        .def(nb::init<>())
        .def(nb::init<const TfToken &, const UsdValidationErrorType &, 
             const UsdValidationErrorSites &, const std::string &>(),
             nb::arg("name"), nb::arg("errorType"), nb::arg("errorSites"), nb::arg("errorMessage"))
        .def("GetName", 
             +[](const UsdValidationError &validationError) {
                 return validationError.GetName();
             })
        .def("GetIdentifier", &UsdValidationError::GetIdentifier)
        .def("GetType", &UsdValidationError::GetType)
        .def("GetSites", 
             +[](const UsdValidationError &validationError) {
                return validationError.GetSites();
             })
        .def("GetMessage", &UsdValidationError::GetMessage)
        .def("GetErrorAsString", &UsdValidationError::GetErrorAsString)
        .def("GetValidator", &UsdValidationError::GetValidator, nb::rv_policy::reference)
        .def("HasNoError", &UsdValidationError::HasNoError)
        .def("GetData", 
             +[](const UsdValidationError &validationError) {
                 return validationError.GetData();
             })
        .def("GetFixers", _GetFixers)
        .def("GetFixerByName", 
             +[](const UsdValidationError &validationError, 
                 const TfToken &name) 
                 -> const UsdValidationFixer* {
                 return validationError.GetFixerByName(name);
             },
             nb::rv_policy::reference,
             nb::arg("name"))
        .def("GetFixersByErrorName", _GetFixersByErrorName)
        .def("GetFixerByNameAndErrorName",
             +[](const UsdValidationError &validationError, 
                 const TfToken &name)
                 -> const UsdValidationFixer* {
                 return validationError.GetFixerByNameAndErrorName(name);
             },
             nb::rv_policy::reference,
             nb::arg("name"))
        .def("GetFixersByKeywords", _GetFixersByKeywords,
             nb::arg("keywords"))
        .def(nb::self == nb::self)
        .def(nb::self != nb::self);
}
