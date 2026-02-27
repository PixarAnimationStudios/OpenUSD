//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_CACHE_VIEW_H
#define PXR_EXEC_EXEC_CACHE_VIEW_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include "pxr/base/tf/span.h"
#include "pxr/exec/vdf/dataManagerFacade.h"

#include <memory>
#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

class Exec_ValueExtractor;
class VdfExecutorInterface;
class VdfMaskedOutput;
class VtValue;

/// A view into values cached by ExecSystem.
///
/// This class is not intended to be used directly by users but as part of
/// higher level libraries.  Cache views must not outlive the ExecSystem or
/// request from which they were built.
///
class Exec_CacheView
{
public:
    /// Constructs an invalid cache view.
    Exec_CacheView() = default;

    /// Exec_CacheView is move-only.
    Exec_CacheView(const Exec_CacheView &) = delete;

    /// Constructs a cache view from another.
    ///
    /// The other cache view becomes invalid.
    ///
    EXEC_API
    Exec_CacheView(Exec_CacheView &&other);

    EXEC_API
    ~Exec_CacheView();

    /// Returns the computed value for the provided extraction \p index.
    /// 
    /// Emits an error and returns an empty value if the \p index is not
    /// evaluated.
    ///
    EXEC_API
    VtValue Get(int index) const;

private:
    friend class Exec_RequestImpl;

    // Constructs a cache view that extracts computed values from
    // \p dataManager. The data manager must outlive the cache view.
    Exec_CacheView(
        const VdfDataManagerFacade dataManager,
        TfSpan<const VdfMaskedOutput> outputs,
        TfSpan<const Exec_ValueExtractor> extractors);

    // Constructs a cache view that assumes ownership of an \p executor, so that
    // it remains valid while computed values are extracted from it.
    Exec_CacheView(
        std::unique_ptr<VdfExecutorInterface> &&executor,
        TfSpan<const VdfMaskedOutput> outputs,
        TfSpan<const Exec_ValueExtractor> extractors);

private:
    std::optional<const VdfDataManagerFacade> _dataManager;
    const TfSpan<const VdfMaskedOutput> _outputs;
    const TfSpan<const Exec_ValueExtractor> _extractors;
    std::unique_ptr<VdfExecutorInterface> _executor;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
