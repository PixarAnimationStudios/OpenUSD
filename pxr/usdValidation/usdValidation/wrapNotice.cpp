//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usdValidation/usdValidation/notice.h"
#include "pxr/usdValidation/usdValidation/validator.h"
#include "pxr/base/tf/pyNoticeWrapper.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/scope.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

TF_INSTANTIATE_NOTICE_WRAPPER(
    UsdValidationNotice::DidRegisterValidator, TfNotice);
TF_INSTANTIATE_NOTICE_WRAPPER(
    UsdValidationNotice::DidRegisterValidatorSuite, TfNotice);

} // anonymous namespace

void
wrapUsdValidationNotice()
{
    scope noticeScope = class_<UsdValidationNotice>("Notice", no_init);

    TfPyNoticeWrapper<
        UsdValidationNotice::DidRegisterValidator, TfNotice>::Wrap()
        .def("GetValidator",
             &UsdValidationNotice::DidRegisterValidator::GetValidator,
             return_value_policy<reference_existing_object>())
        ;

    TfPyNoticeWrapper<
        UsdValidationNotice::DidRegisterValidatorSuite, TfNotice>::Wrap()
        .def("GetValidatorSuite",
             &UsdValidationNotice::DidRegisterValidatorSuite::GetValidatorSuite,
             return_value_policy<reference_existing_object>())
        ;
}
