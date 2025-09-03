//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_USD_CACHE_VIEW_H
#define PXR_EXEC_EXEC_USD_CACHE_VIEW_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/execUsd/api.h"

#include "pxr/exec/exec/cacheView.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// Provides a view of values computed by ExecUsdSystem::Compute().
///
/// Cache views must not outlive the ExecUsdSystem or ExecUsdRequest from
/// which they were built.
///
class ExecUsdCacheView
{
public:
    /// Construct an invalid view.
    ExecUsdCacheView() = default;

    /// Returns the computed value for the provided extraction \p index.
    /// 
    /// Emits an error and returns an empty value if the \p index is not
    /// evaluated.
    ///
    EXECUSD_API
    VtValue Get(int index) const;

private:
    friend class ExecUsd_RequestImpl;
    explicit ExecUsdCacheView(
        Exec_CacheView &&view)
        : _view(std::move(view))
    {}

private:
    Exec_CacheView _view;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
