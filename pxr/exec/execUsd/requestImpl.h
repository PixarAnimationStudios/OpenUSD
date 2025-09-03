//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_USD_REQUEST_IMPL_H
#define PXR_EXEC_EXEC_USD_REQUEST_IMPL_H

#include "pxr/pxr.h"

#include "pxr/exec/execUsd/api.h"

#include "pxr/base/tf/bits.h"
#include "pxr/exec/exec/request.h"
#include "pxr/exec/exec/requestImpl.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class ExecUsdCacheView;
class ExecUsdSystem;
class ExecUsdValueKey;
class ExecValueKey;

/// Contains Usd-specific data structures necessary to implement requests.
class ExecUsd_RequestImpl final : public Exec_RequestImpl
{
public:
    ExecUsd_RequestImpl(
        ExecUsdSystem *system,
        std::vector<ExecUsdValueKey> &&valueKeys,
        ExecRequestComputedValueInvalidationCallback &&valueCallback,
        ExecRequestTimeChangeInvalidationCallback &&timeCallback);

    ExecUsd_RequestImpl(const ExecUsd_RequestImpl&) = delete;
    ExecUsd_RequestImpl& operator=(const ExecUsd_RequestImpl&) = delete;

    ~ExecUsd_RequestImpl();

    /// Returns per-index expiration state.
    const TfBits &GetExpiredIndices() const {
        return _expired;
    }

    /// Compile the request.
    void Compile();

    /// Schedule the request.
    void Schedule();

    /// Computes the value keys in the request.
    ExecUsdCacheView Compute();

    /// Expires the request based on providers that have become invalid.
    void ExpireInvalidIndices();

    /// Removes the request from the system.
    ///
    /// This prevents any further notification and releases internal request
    /// data structures.
    ///
    void Discard();

private:
    std::vector<ExecUsdValueKey> _valueKeys;
    TfBits _expired;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
