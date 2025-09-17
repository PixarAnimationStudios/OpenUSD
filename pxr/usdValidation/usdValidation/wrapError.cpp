//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/validator.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python/class.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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
        .def(self == self)
        .def(self != self);
}
