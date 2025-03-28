//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file glRawContext.cpp

#include "pxr/imaging/glf/glRawContext.h"

PXR_NAMESPACE_OPEN_SCOPE


//
// GlfGLRawContext
//

GlfGLRawContextSharedPtr
GlfGLRawContext::New()
{
    return GlfGLRawContextSharedPtr(
                new GlfGLRawContext(GarchGLPlatformContextState()));
}

GlfGLRawContextSharedPtr
GlfGLRawContext::New(const GarchGLPlatformContextState& state)
{
    return GlfGLRawContextSharedPtr(new GlfGLRawContext(state));
}

GlfGLRawContext::GlfGLRawContext(const GarchGLPlatformContextState& state) :
    _state(state)
{
    // Do nothing
}

GlfGLRawContext::~GlfGLRawContext()
{
    // Do nothing
}

bool
GlfGLRawContext::IsValid() const
{
    return _state.IsValid();
}

void
GlfGLRawContext::_MakeCurrent()
{
    _state.MakeCurrent();
}

bool
GlfGLRawContext::_IsSharing(const GlfGLContextSharedPtr& rhs) const
{
    return false;
}

bool
GlfGLRawContext::_IsEqual(const GlfGLContextSharedPtr& rhs) const
{
    if (const GlfGLRawContext* rhsRaw =
                dynamic_cast<const GlfGLRawContext*>(rhs.get())) {
        return _state == rhsRaw->_state;
    }
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE

