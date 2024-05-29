//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file glPlatformContext.cpp

#include "pxr/imaging/garch/glPlatformContext.h"
#include "pxr/base/tf/hash.h"

PXR_NAMESPACE_OPEN_SCOPE


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
    return display  == rhs.display  &&
           drawable == rhs.drawable &&
           context  == rhs.context;
}

size_t
GarchGLXContextState::GetHash() const
{
    return TfHash::Combine(        
        display,
        drawable,
        context
    );
}

bool
GarchGLXContextState::IsValid() const
{
    return display && drawable && context;
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
    return GarchGLXContextState(NULL, None, NULL);
}

PXR_NAMESPACE_CLOSE_SCOPE

