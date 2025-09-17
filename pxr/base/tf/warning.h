//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_WARNING_H
#define PXR_BASE_TF_WARNING_H

#include "pxr/pxr.h"

#include "pxr/base/tf/diagnosticBase.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfWarning
/// \ingroup group_tf_Diagnostic
///
/// Represents an object that contains information about a warning.
///
/// See \ref page_tf_Diagnostic in the C++ API reference for a detailed
/// description of the warning issuing API.  For a example of how to post a
/// warning, see \c TF_WARN(), also in the C++ API reference.
///
/// In the Python API, you can issue a warning with Tf.Warn().
///
class TfWarning: public TfDiagnosticBase
{
private:
    TfWarning(TfEnum warningCode, char const *warningCodeString,
            TfCallContext const &context, const std::string& commentary,
            TfDiagnosticInfo info, bool quiet)
        : TfDiagnosticBase(warningCode, warningCodeString, context,
                            commentary, info, quiet)
    { }

    friend class TfDiagnosticMgr;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_WARNING_H
