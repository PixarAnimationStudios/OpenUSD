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
