//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/glContextRegistry.h"
#include "pxr/imaging/garch/glPlatformContext.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE


//
// GlfGLContext
//

GlfGLContext::GlfGLContext()
{
    // Do nothing
}

GlfGLContext::~GlfGLContext()
{
    GlfGLContextRegistry::GetInstance().Remove(this);
}

GlfGLContextSharedPtr
GlfGLContext::GetCurrentGLContext()
{
    return GlfGLContextRegistry::GetInstance().GetCurrent();
}

GlfGLContextSharedPtr
GlfGLContext::GetSharedGLContext()
{
    return GlfGLContextRegistry::GetInstance().GetShared();
}

void
GlfGLContext::MakeCurrent(const GlfGLContextSharedPtr& context)
{
    TRACE_FUNCTION();

    if (context && context->IsValid()) {
        context->_MakeCurrent();

        // Now that this context is current add it to the registry for
        // later lookup.
        GlfGLContextRegistry::GetInstance().DidMakeCurrent(context);
    }
    else {
        DoneCurrent();
    }
}

bool
GlfGLContext::AreSharing(GlfGLContextSharedPtr const & context1,
                         GlfGLContextSharedPtr const & context2)
{
    return (context1 && context1->IsSharing(context2));
}

bool
GlfGLContext::IsInitialized()
{
    return GlfGLContextRegistry::GetInstance().IsInitialized();
}

bool
GlfGLContext::IsCurrent() const
{
    return IsValid() && _IsEqual(GetCurrentGLContext());
}

void
GlfGLContext::MakeCurrent()
{
    if (IsValid()) {
        _MakeCurrent();
    }
}

void
GlfGLContext::DoneCurrent()
{
    GarchGLPlatformContextState::DoneCurrent();
}

bool
GlfGLContext::IsSharing(GlfGLContextSharedPtr const & otherContext)
{
    return otherContext && IsValid() &&
           otherContext->IsValid() && _IsSharing(otherContext);
}

//
// GlfGLContextScopeHolder
//

GlfGLContextScopeHolder::GlfGLContextScopeHolder(
    const GlfGLContextSharedPtr& newContext) :
    _newContext(newContext)
{
    if (_newContext) {
        _oldContext = GlfGLContext::GetCurrentGLContext();
    }
    _MakeNewContextCurrent();
}

GlfGLContextScopeHolder::~GlfGLContextScopeHolder()
{
    _RestoreOldContext();
}

void
GlfGLContextScopeHolder::_MakeNewContextCurrent()
{
    if (_newContext) {
        GlfGLContext::MakeCurrent(_newContext);
    }
}

void
GlfGLContextScopeHolder::_RestoreOldContext()
{
    if (_newContext) {
        GlfGLContext::MakeCurrent(_oldContext);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

