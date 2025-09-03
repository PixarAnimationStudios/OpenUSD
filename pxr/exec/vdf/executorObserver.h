//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_EXECUTOR_OBSERVER_H
#define PXR_EXEC_VDF_EXECUTOR_OBSERVER_H

/// \file

#include "pxr/pxr.h"
#include "pxr/exec/vdf/api.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfExecutorInterface;

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfExecutorObserver
///
/// \brief This is an interface for any class that wants to
/// listen to specific executor events, such as executor
/// deletion, or clearing data on the executor.
///
/// NOTE: This class MUST NOT be used for fine grained
/// observations, as this specific pattern could dramatically
/// reduce performance.
///
class VDF_API_TYPE VdfExecutorObserver
{
    friend class VdfExecutorInterface;

public:
    VDF_API
    virtual ~VdfExecutorObserver();

protected:
    /// This will be called once the observed executor gets deleted.
    ///
    /// The observed \p executor is passed as a parameter.
    ///
    virtual void _OnExecutorDelete(
        VdfExecutorInterface *executor) const = 0;

    /// This will get called once the data is being cleared on the
    /// observed executor.
    ///
    /// The observed \p executor is passed as a parameter.
    ///
    virtual void _OnExecutorClearData(
        VdfExecutorInterface *executor) const = 0;

};

////////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_EXEC_VDF_EXECUTOR_OBSERVER_H
