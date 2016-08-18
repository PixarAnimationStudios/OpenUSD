/// \file glPlatformContextX.h
// Copyright 2013, Pixar Animation Studios.  All rights reserved.

#ifndef GARCH_GLPLATFORMCONTEXTX_H
#define GARCH_GLPLATFORMCONTEXTX_H

#include "pxr/base/arch/defines.h"
#include "pxr/imaging/garch/api.h"

#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)

#include <GL/glx.h>

class GARCH_API GarchGLXContextState {
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

#endif // #if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)

#endif  // GARCH_GLPLATFORMCONTEXT_H
