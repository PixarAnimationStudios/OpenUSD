/// \file glPlatformContextX.cpp
// Copyright 2013, Pixar Animation Studios.  All rights reserved.

#include "pxr/imaging/garch/glPlatformContext.h"
#include <boost/functional/hash.hpp>
#include <ciso646>

#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)

//
// GarchGLXContextState
//

GarchGLXContextState::GarchGLXContextState() :
    display(glXGetCurrentDisplay()),
    drawable(glXGetCurrentDrawable()),
    context(glXGetCurrentContext()),
    _defaultCtor(true)
{
    // Do nothing
}

GarchGLXContextState::GarchGLXContextState(
	Display* display_, GLXDrawable drawable_, GLXContext context_) :
    display(display_), drawable(drawable_), context(context_),
     _defaultCtor(false)
{
    // Do nothing
}

bool
GarchGLXContextState::operator==(const GarchGLXContextState& rhs) const
{
    return display  == rhs.display and
           drawable == rhs.drawable and
           context  == rhs.context;
}

size_t
GarchGLXContextState::GetHash() const
{
    size_t result = 0;
    boost::hash_combine(result, drawable);
	boost::hash_combine(result, display);
    boost::hash_combine(result, context);
    return result;
}

bool
GarchGLXContextState::IsValid() const
{
    return display and drawable and context;
}

void
GarchGLXContextState::MakeCurrent()
{
    if (IsValid()) {
        glXMakeCurrent(display, drawable, context);
    }
    else if (_defaultCtor) {
        DoneCurrent();
    }
}

void
GarchGLXContextState::DoneCurrent()
{
    if (Display* display = glXGetCurrentDisplay()) {
        glXMakeCurrent(display, None, NULL);
    }
}

GarchGLPlatformContextState
GarchGetNullGLPlatformContextState()
{
    return GarchGLXContextState(NULL, NULL, NULL);
}

#endif // #if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)