//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usdValidation/usdValidation/notice.h"
#include "pxr/usdValidation/usdValidation/validator.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyNoticeWrapper.h"

#include <nanobind/nanobind.h>

PXR_NAMESPACE_USING_DIRECTIVE

void
wrapUsdValidationNotice(nanobind::module_ &m)
{
    namespace nb = nanobind;
    nb::class_<UsdValidationNotice> noticeScope(m, "Notice");

    TfNbNoticeWrapper<
        UsdValidationNotice::DidRegisterValidator, TfNotice>::Wrap(noticeScope)
        .def("GetValidator",
             &UsdValidationNotice::DidRegisterValidator::GetValidator,
             nb::rv_policy::reference)
        ;

    TfNbNoticeWrapper<
        UsdValidationNotice::DidRegisterValidatorSuite, TfNotice>::Wrap(
            noticeScope)
        .def("GetValidatorSuite",
             &UsdValidationNotice::DidRegisterValidatorSuite::GetValidatorSuite,
             nb::rv_policy::reference)
        ;
}
