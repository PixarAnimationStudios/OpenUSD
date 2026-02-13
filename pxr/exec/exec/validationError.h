//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_VALIDATION_ERROR_H
#define PXR_EXEC_EXEC_VALIDATION_ERROR_H

/// \file

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Types of errors reported by exec compilation.
///
/// Errors are reported by the TF_ERROR macro, for example:
/// \code
///     TF_ERROR(ExecValidationErrorType::DataDependencyCycle, "Message...");
/// \endcode
///
/// Clients can check if an Exec API reported an error by using a TfErrorMark.
///
enum class ExecValidationErrorType
{
    /// Exec compilation encountered a computation that depends on itself.
    DataDependencyCycle
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif