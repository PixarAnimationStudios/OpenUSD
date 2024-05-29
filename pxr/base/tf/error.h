//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
