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
/// \file glPlatformContext.cpp

#include "pxr/imaging/garch/glPlatformContext.h"
#include <boost/functional/hash.hpp>

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
    size_t result = 0;
    boost::hash_combine(result, display);
    boost::hash_combine(result, drawable);
    boost::hash_combine(result, context);
    return result;
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

