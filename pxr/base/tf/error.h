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
#ifndef PXR_BASE_TF_ERROR_H
#define PXR_BASE_TF_ERROR_H

/// \file tf/error.h

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnosticBase.h"

PXR_NAMESPACE_OPEN_SCOPE

class TfDiagnosticMgr;

/// \class TfError
/// \ingroup group_tf_Diagnostic
///
/// Represents an object that contains error information.
/// 
/// See \ref page_tf_Diagnostic in the C++ API reference for a detailed
/// description of the error issuing API.  For a example of how to post an
/// error, see \c TF_ERROR(), also in the C++ API reference.
///
/// In the Python API, you can raise several different types of errors,
/// including coding errors (Tf.RaiseCodingError), run time errors
/// (Tf.RaiseRuntimeError), fatal errors (Tf.Fatal).
///
class TfError: public TfDiagnosticBase {

public:
    /// Return the error code posted.
    TfEnum GetErrorCode() const {
        return GetDiagnosticCode();
    }

    /// Return the diagnostic code posted as a string.
    const std::string& GetErrorCodeAsString() const {
        return GetDiagnosticCodeAsString();
    }

private:
    TfError(TfEnum errorCode, char const *errCodeString,
            TfCallContext const &context, const std::string& commentary,
            TfDiagnosticInfo info, bool quiet);

    friend class TfDiagnosticMgr;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_ERROR_H
