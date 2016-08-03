/// \file glPlatformContextWindows.cpp
// Copyright 2013, Pixar Animation Studios.  All rights reserved.

#include "pxr/imaging/garch/glPlatformContext.h"
#include <boost/functional/hash.hpp>

//
// GarchGLWContextState
//

GarchGLWContextState::GarchGLWContextState() :
	display(wglGetCurrentDC()),
	drawable(wglGetCurrentDC()),
	context(wglGetCurrentContext()),
   _defaultCtor(true)
{
    // Do nothing
}

GarchGLWContextState::GarchGLWContextState(
	HDC display_, HDC drawable_, HGLRC context_):
    display(display_), drawable(drawable_), context(context_),
     _defaultCtor(false)
{
    // Do nothing
}

bool
GarchGLWContextState::operator==(const GarchGLWContextState& rhs) const
{
    return display  == rhs.display &&
           drawable == rhs.drawable &&
           context  == rhs.context;
}

size_t
GarchGLWContextState::GetHash() const
{
    size_t result = 0;
    boost::hash_combine(result, drawable);
	boost::hash_combine(result, display);
    boost::hash_combine(result, context);
    return result;
}

bool
GarchGLWContextState::IsValid() const
{
    return display && drawable && context;
}

void
GarchGLWContextState::MakeCurrent()
{
    if (IsValid()) {
		wglMakeCurrent(display, context);
    }
    else if (_defaultCtor) {
        DoneCurrent();
    }
}

void
GarchGLWContextState::DoneCurrent()
{
	if (HGLRC display = wglGetCurrentContext()) {
		wglMakeCurrent(NULL, display);
	}
}

GarchGLPlatformContextState
GarchGetNullGLPlatformContextState()
{
    return GarchGLWContextState(NULL, NULL, NULL);
}