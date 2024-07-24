//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GARCH_GL_PLATFORM_CONTEXT_GLX_H
#define PXR_IMAGING_GARCH_GL_PLATFORM_CONTEXT_GLX_H

#include "pxr/pxr.h"
#include <GL/glx.h>

PXR_NAMESPACE_OPEN_SCOPE


class GarchGLXContextState {
public:
    /// Construct with the current state.
    GarchGLXContextState();

    /// Construct with the given state.
    GarchGLXContextState(Display*, GLXDrawable, GLXContext);

    /// Compare for equality.
    bool operator==(const GarchGLXContextState& rhs) const;

    /// Returns a hash value for the state.
    size_t GetHash() const;

    /// Returns \c true if the context state is valid.
    bool IsValid() const;

    /// Make the context current.
    void MakeCurrent();

    /// Make no context current.
    static void DoneCurrent();

public:
    Display* display;
    GLXDrawable drawable;
    GLXContext context;

private:
    bool _defaultCtor;
};

// Hide the platform specific type name behind a common name.
typedef GarchGLXContextState GarchGLPlatformContextState;


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_GARCH_GL_PLATFORM_CONTEXT_GLX_H
