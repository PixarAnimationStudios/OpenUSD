//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/base/tf/diagnosticNotice.h"
#include "pxr/base/tf/error.h"
#include "pxr/base/tf/pyNoticeWrapper.h"
#include "pxr/base/tf/status.h"
#include "pxr/base/tf/warning.h"

#include <boost/python/class.hpp>
#include <boost/python/scope.hpp>

using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_NOTICE_WRAPPER(TfDiagnosticNotice::Base, TfNotice);
TF_INSTANTIATE_NOTICE_WRAPPER(TfDiagnosticNotice::IssuedError,
    TfDiagnosticNotice::Base);
TF_INSTANTIATE_NOTICE_WRAPPER(TfDiagnosticNotice::IssuedWarning,
    TfDiagnosticNotice::Base);
TF_INSTANTIATE_NOTICE_WRAPPER(TfDiagnosticNotice::IssuedStatus,
    TfDiagnosticNotice::Base);
TF_INSTANTIATE_NOTICE_WRAPPER(TfDiagnosticNotice::IssuedFatalError,
    TfDiagnosticNotice::Base);

void wrapDiagnosticNotice() {
    scope noticeScope = class_<TfDiagnosticNotice>("DiagnosticNotice", no_init);

    TfPyNoticeWrapper<TfDiagnosticNotice::Base, TfNotice>::Wrap()
        .def(init<>());

    TfPyNoticeWrapper<TfDiagnosticNotice::IssuedError,
        TfDiagnosticNotice::Base>::Wrap()
        .def(init<const TfError &>())
        .add_property("error", make_function(
            &TfDiagnosticNotice::IssuedError::GetError,
            return_value_policy<return_by_value>()))
        ;

    TfPyNoticeWrapper<TfDiagnosticNotice::IssuedWarning,
        TfDiagnosticNotice::Base>::Wrap()
        .def(init<const TfWarning &>())
        .add_property("warning", make_function(
            &TfDiagnosticNotice::IssuedWarning::GetWarning,
            return_value_policy<return_by_value>()))
        ;

    TfPyNoticeWrapper<TfDiagnosticNotice::IssuedStatus,
        TfDiagnosticNotice::Base>::Wrap()
        .def(init<const TfStatus &>())
        .add_property("status", make_function(
            &TfDiagnosticNotice::IssuedStatus::GetStatus,
            return_value_policy<return_by_value>()))
        ;

    TfPyNoticeWrapper<TfDiagnosticNotice::IssuedFatalError,
        TfDiagnosticNotice::Base>::Wrap()
        .def(init<const std::string &, const TfCallContext &>())
        .add_property("message", make_function(
            &TfDiagnosticNotice::IssuedFatalError::GetMessage,
            return_value_policy<return_by_value>()))
        .add_property("context", make_function(
            &TfDiagnosticNotice::IssuedFatalError::GetContext,
            return_value_policy<return_by_value>()))
        ;
}

PXR_NAMESPACE_CLOSE_SCOPE
