/// \file glPlatformContextWindows.h
// Copyright 2013, Pixar Animation Studios.  All rights reserved.

#ifndef GARCH_GLPLATFORMCONTEXTW_H
#define GARCH_GLPLATFORMCONTEXTW_H

#include "pxr/base/arch/defines.h"
#include "pxr/imaging/garch/api.h"

#include <Windows.h>

class GARCH_API GarchGLWContextState {
public:
    /// Construct with the current state.
    GarchGLWContextState();

    /// Construct with the given state.
	GarchGLWContextState(HDC, HDC, HGLRC);

    /// Compare for equality.
    bool operator==(const GarchGLWContextState& rhs) const;

    /// Returns a hash value for the state.
    size_t GetHash() const;

    /// Returns \c true if the context state is valid.
    bool IsValid() const;

    /// Make the context current.
    void MakeCurrent();

    /// Make no context current.
    static void DoneCurrent();

public:
	HDC display;
	HDC drawable;
	HGLRC context;

private:
    bool _defaultCtor;
};

// Hide the platform specific type name behind a common name.
typedef GarchGLWContextState GarchGLPlatformContextState;

#endif  // GARCH_GLPLATFORMCONTEXTW_H
