//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GARCH_GL_PLATFORM_CONTEXT_WINDOWS_H
#define PXR_IMAGING_GARCH_GL_PLATFORM_CONTEXT_WINDOWS_H

#include "pxr/pxr.h"
#include "pxr/imaging/garch/api.h"
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


class GarchWGLContextState {
public:
    /// Construct with the current state.
    GARCH_API
    GarchWGLContextState();

    enum class NullState { nullstate };

    /// Construct with the null state.
    GARCH_API
    GarchWGLContextState(NullState);

    /// Compare for equality.
    GARCH_API
    bool operator==(const GarchWGLContextState& rhs) const;

    /// Returns a hash value for the state.
    GARCH_API
    size_t GetHash() const;

    /// Returns \c true if the context state is valid.
    GARCH_API
    bool IsValid() const;

    /// Make the context current.
    GARCH_API
    void MakeCurrent();

    /// Make no context current.
    GARCH_API
    static void DoneCurrent();

private:
    class _Detail;
    std::shared_ptr<_Detail> _detail;
};

// Hide the platform specific type name behind a common name.
typedef GarchWGLContextState GarchGLPlatformContextState;


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_GARCH_GL_PLATFORM_CONTEXT_WINDOWS_H
