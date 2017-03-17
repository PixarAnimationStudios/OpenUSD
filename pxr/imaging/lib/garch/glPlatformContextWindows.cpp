//
// Copyright 2017 Pixar
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
#include "pxr/imaging/garch/glPlatformContextWindows.h"

#include <boost/functional/hash.hpp>
#include <Windows.h>

class GarchWGLContextState::_Detail {
public:
    _Detail(HDC hdc, HGLRC hglrc) : hdc(hdc), hglrc(hglrc) { }

    HDC hdc;
    HGLRC hglrc;
};

//
// GarchWGLContextState
//

GarchWGLContextState::GarchWGLContextState() :
    _detail(std::make_shared<_Detail>(wglGetCurrentDC(),wglGetCurrentContext()))
{
    // Do nothing
}

GarchWGLContextState::GarchWGLContextState(NullState) :
    _detail(std::make_shared<_Detail>(HDC(0), HGLRC(0)))
{
    // Do nothing
}

bool
GarchWGLContextState::operator==(const GarchWGLContextState& rhs) const
{
    return _detail->hdc   == rhs._detail->hdc &&
           _detail->hglrc == rhs._detail->hglrc;
}

size_t
GarchWGLContextState::GetHash() const
{
    size_t result = 0;
    boost::hash_combine(result, _detail->hdc);
    boost::hash_combine(result, _detail->hglrc);
    return result;
}

bool
GarchWGLContextState::IsValid() const
{
    return _detail->hdc && _detail->hglrc;
}

void
GarchWGLContextState::MakeCurrent()
{
    wglMakeCurrent(_detail->hdc, _detail->hglrc);
}

void
GarchWGLContextState::DoneCurrent()
{
    wglMakeCurrent(NULL, NULL);
}

GARCH_API
GarchGLPlatformContextState
GarchGetNullGLPlatformContextState()
{
    return GarchWGLContextState(GarchWGLContextState::NullState::nullstate);
}
