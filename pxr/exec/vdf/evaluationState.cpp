//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/evaluationState.h"

#include "pxr/exec/vdf/executorErrorLogger.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

void
VdfEvaluationState::LogWarning(
    const VdfNode &node,
    const std::string &warning) const
{
    TRACE_FUNCTION();

    if (_errorLogger) {
        _errorLogger->LogWarning(node, warning);
    } else {
        VdfExecutorErrorLogger::IssueDefaultWarning(node, warning);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
