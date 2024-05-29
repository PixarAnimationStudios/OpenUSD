//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GARCH_GL_PLATFORM_CONTEXT_DARWIN_H
#define PXR_IMAGING_GARCH_GL_PLATFORM_CONTEXT_DARWIN_H

#include "pxr/pxr.h"
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


class GarchNSGLContextState {
public:
    /// Construct with the current state.
    GarchNSGLContextState();

    enum class NullState { nullstate };
    GarchNSGLContextState(NullState);

    /// Compare for equality.
    bool operator==(const GarchNSGLContextState& rhs) const;

    /// Returns a hash value for the state.
    size_t GetHash() const;

    /// Returns \c true if the context state is valid.
    bool IsValid() const;

    /// Make the context current.
    void MakeCurrent();

    /// Make no context current.
    static void DoneCurrent();

private:
    class Detail;
    std::shared_ptr<Detail> _detail;
};

// Hide the platform specific type name behind a common name.
typedef GarchNSGLContextState GarchGLPlatformContextState;


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_GARCH_GL_PLATFORM_CONTEXT_DARWIN_H
