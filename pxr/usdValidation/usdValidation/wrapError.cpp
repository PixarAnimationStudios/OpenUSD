//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/validator.h"
#include "pxr/usdValidation/usdValidation/fixer.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python/class.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

    list
    _GetFixers(const UsdValidationError &error) {
        list result;
        for (const auto *fixer : error.GetFixers()) {
            result.append(pointer_wrapper(fixer));
        }
        return result;
    }

    list
    _GetFixersByErrorName(const UsdValidationError &error) {
        list result;
        for (const auto *fixer : error.GetFixersByErrorName()) {
            result.append(pointer_wrapper(fixer));
        }
        return result;
    }

    list
    _GetFixersByKeywords(const UsdValidationError &error, 
                         const TfTokenVector &keywords) {
        list result;
        for (const auto *fixer : error.GetFixersByKeywords(keywords)) {
            result.append(pointer_wrapper(fixer));
        }
        return result;
    }
} // anonymous namespace

void wrapUsdValidationError()
{
    TfPyWrapEnum<UsdValidationErrorType>("ValidationErrorType");

    class_<UsdValidationErrorSite>("ValidationErrorSite")
        .def(init<>())
        .def(init<const SdfLayerHandle &, const SdfPath &>(
            args("layer", "objectPath")))
        .def(init<const UsdStagePtr &, const SdfPath &, const SdfLayerHandle &>(
            (arg("stage"), arg("objectPath"), arg("layer") = SdfLayerHandle{})))
        .def("IsValid", &UsdValidationErrorSite::IsValid)
        .def("IsValidSpecInLayer", &UsdValidationErrorSite::IsValidSpecInLayer)
        .def("IsPrim", &UsdValidationErrorSite::IsPrim)
        .def("IsProperty", &UsdValidationErrorSite::IsProperty)
        .def("GetPropertySpec", &UsdValidationErrorSite::GetPropertySpec)
        .def("GetPrimSpec", &UsdValidationErrorSite::GetPrimSpec)
        .def("GetLayer", &UsdValidationErrorSite::GetLayer, 
             return_value_policy<return_by_value>())
        .def("GetStage", &UsdValidationErrorSite::GetStage, 
             return_value_policy<return_by_value>())
        .def("GetPrim", &UsdValidationErrorSite::GetPrim)
        .def("GetProperty", &UsdValidationErrorSite::GetProperty)
        .def(self == self)
        .def(self != self);

    TfPyRegisterStlSequencesFromPython<UsdValidationErrorSite>();
    class_<UsdValidationError>("ValidationError")
        .def(init<>())
        .def(init<const TfToken &, const UsdValidationErrorType &, 
             const UsdValidationErrorSites &, const std::string &>(
             args("name", "errorType", "errorSites", "errorMessage")))
        .def("GetName", 
             +[](const UsdValidationError &validationError) {
                 return validationError.GetName();
             }, 
             return_value_policy<return_by_value>())
        .def("GetIdentifier", &UsdValidationError::GetIdentifier, 
             return_value_policy<return_by_value>())
        .def("GetType", &UsdValidationError::GetType)
        .def("GetSites", 
             +[](const UsdValidationError &validationError) {
                return validationError.GetSites();
             }, 
            return_value_policy<TfPySequenceToList>())
        .def("GetMessage", &UsdValidationError::GetMessage, 
             return_value_policy<return_by_value>())
        .def("GetErrorAsString", &UsdValidationError::GetErrorAsString)
        .def("GetValidator", &UsdValidationError::GetValidator, return_value_policy<reference_existing_object>())
        .def("HasNoError", &UsdValidationError::HasNoError)
        .def("GetData", 
             +[](const UsdValidationError &validationError) {
                 return validationError.GetData();
             }, 
             return_value_policy<return_by_value>())
        .def("GetFixers", _GetFixers)
        .def("GetFixerByName", 
             +[](const UsdValidationError &validationError, 
                 const TfToken &name) 
                 -> const UsdValidationFixer* {
                 return validationError.GetFixerByName(name);
             },
             return_value_policy<reference_existing_object>(),
             (arg("name")))
        .def("GetFixersByErrorName", _GetFixersByErrorName)
        .def("GetFixerByNameAndErrorName",
             +[](const UsdValidationError &validationError, 
                 const TfToken &name)
                 -> const UsdValidationFixer* {
                 return validationError.GetFixerByNameAndErrorName(name);
             },
             return_value_policy<reference_existing_object>(),
             (arg("name")))
        .def("GetFixersByKeywords", _GetFixersByKeywords,
             (arg("keywords")))
        .def(self == self)
        .def(self != self);
}
