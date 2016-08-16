/// \file glPlatformContextWindows.cpp
// Copyright 2013, Pixar Animation Studios.  All rights reserved.

#include <boost/functional/hash.hpp>
#include "pxr/imaging/garch/glPlatformContext.h"
#include "pxr/base/arch/errno.h"
#include "pxr/base/arch/error.h"

//
// GarchGLWContextState
//

static void checkError()
{
    DWORD errorCode = GetLastError();
    if(errorCode > 0)
    {
        std::string message = ArchStrSysError(errorCode);
        OutputDebugString(message.c_str());
    }
}
GarchGLWContextState::GarchGLWContextState() :
   _defaultCtor(true)
{
    context = wglGetCurrentContext();
    checkError();

    device = wglGetCurrentDC();
    checkError();
}

GarchGLWContextState::GarchGLWContextState(HDC device_, HGLRC context_):
    context(context_), device(device_), _defaultCtor(false)
{
    // Do nothing
}

bool
GarchGLWContextState::operator==(const GarchGLWContextState& rhs) const
{
    return context == rhs.context;
}

size_t
GarchGLWContextState::GetHash() const
{
    size_t result = 0;
    boost::hash_combine(result, device);
    boost::hash_combine(result, context);
    return result;
}

bool
GarchGLWContextState::IsValid() const
{
    return context && device;
}

void
GarchGLWContextState::MakeCurrent()
{
    if (IsValid()) 
    {
        wglMakeCurrent(device, context);
        checkError();
    }
    else if (_defaultCtor)
    {
        DoneCurrent();
    }
}

void
GarchGLWContextState::DoneCurrent()
{
    wglMakeCurrent(NULL, NULL);
    checkError();
}

GarchGLPlatformContextState
GarchGetNullGLPlatformContextState()
{
    return GarchGLWContextState(nullptr, nullptr);
}