//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/glContextRegistry.h"
#include "pxr/imaging/garch/glPlatformContext.h"

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

