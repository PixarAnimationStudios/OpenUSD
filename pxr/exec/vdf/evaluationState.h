//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_EVALUATION_STATE_H
#define PXR_EXEC_VDF_EVALUATION_STATE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class VdfExecutorInterface;
class VdfExecutorErrorLogger;
class VdfNode;
class VdfSchedule;

////////////////////////////////////////////////////////////////////////////////
///
/// This object holds state that remains persistent during one round of
/// network evaluation.
///
class VdfEvaluationState
{
public:

    /// Constructor.
    ///
    VdfEvaluationState(
        const VdfExecutorInterface &executor,
        const VdfSchedule &schedule,
        VdfExecutorErrorLogger *errorLogger) :
        _executor(executor),
        _schedule(schedule),
        _errorLogger(errorLogger)
    {}

    /// The executor used for evaluation.
    ///
    const VdfExecutorInterface &GetExecutor() const {
        return _executor;
    }

    /// The schedule used for evaluation.
    ///
    const VdfSchedule &GetSchedule() const {
        return _schedule;
    }

    /// The executor error logger.
    ///
    VdfExecutorErrorLogger *GetErrorLogger() const {
        return _errorLogger;
    }

    /// Logs an execution warning to the executor error logger.
    ///
    VDF_API
    void LogWarning(
        const VdfNode &node,
        const std::string &warning) const;

private:

    // The executor that created this object.
    const VdfExecutorInterface &_executor;

    // The current schedule.
    const VdfSchedule &_schedule;

    // The error logger.
    VdfExecutorErrorLogger *_errorLogger;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
