//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_USD_REQUEST_H
#define PXR_EXEC_EXEC_USD_REQUEST_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/execUsd/api.h"

#include <memory>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class ExecUsdSystem;
class ExecUsd_RequestImpl;

/// A batch of values to compute together.
///
/// ExecUsdRequest allows clients to specify multiple values to compute at the
/// same time.  It is more efficient to perform compilation, scheduling and
/// evaluation for many attributes at the same time than to perform each of
/// these steps value-by-value.
///
class ExecUsdRequest
{
public:
    EXECUSD_API
    ExecUsdRequest(ExecUsdRequest &&);

    EXECUSD_API
    ExecUsdRequest& operator=(ExecUsdRequest &&);

    EXECUSD_API
    ~ExecUsdRequest();

    /// Return true if this request may be used to compute values.
    ///
    /// Note that a return value of true does not mean that values are cached
    /// or even that the network has been compiled.  It only means that
    /// calling ExecUsdSystem::PrepareRequest or ExecUsdSystem::Compute is
    /// allowed.
    /// 
    EXECUSD_API
    bool IsValid() const;

private:
    friend class ExecUsdSystem;
    explicit ExecUsdRequest(std::unique_ptr<ExecUsd_RequestImpl> &&impl);

    ExecUsd_RequestImpl& _GetImpl() const {
        return *_impl;
    }

private:
    std::unique_ptr<ExecUsd_RequestImpl> _impl;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
