//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/garch/glPlatformContextWindows.h"
#include "pxr/base/tf/hash.h"

#include <Windows.h>

PXR_NAMESPACE_OPEN_SCOPE


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
    return TfHash::Combine(
        _detail->hdc,
        _detail->hglrc
    );
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


PXR_NAMESPACE_CLOSE_SCOPE
