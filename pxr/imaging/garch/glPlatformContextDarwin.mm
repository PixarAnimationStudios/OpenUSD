//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#import <Foundation/Foundation.h>

#include "pxr/pxr.h"
#include "glPlatformContextDarwin.h"

#if defined(PXR_GL_SUPPORT_ENABLED)
#ifdef ARCH_OS_OSX
#import <AppKit/NSOpenGL.h>
typedef NSOpenGLContext NSGLContext;
#elif defined ARCH_OS_IPHONE
#import <UIKit/UIKit.h>
typedef EAGLContext NSGLContext;
#endif
#else
typedef void* NSGLContext;
#endif

PXR_NAMESPACE_OPEN_SCOPE

class GarchNSGLContextState::Detail
{
public:
    Detail() {
#if defined(PXR_GL_SUPPORT_ENABLED)
        context = [NSGLContext currentContext];
#else
        context = nil;
#endif
    }
    Detail(NullState) {
        context = nil;
    }
    ~Detail() {
        context = nil; // garbage collect
    }
    NSGLContext * context;
};

/// Construct with the current state.
GarchNSGLContextState::GarchNSGLContextState()
  : _detail(std::make_shared<GarchNSGLContextState::Detail>())
{
}

GarchNSGLContextState::GarchNSGLContextState(NullState)
  : _detail(std::make_shared<GarchNSGLContextState::Detail>(
                NullState::nullstate))
{
}

/// Construct with the given state.
//GarchNSGLContextState(const GarchNSGLContextState& copy);

/// Compare for equality.
bool
GarchNSGLContextState::operator==(const GarchNSGLContextState& rhs) const
{
    return rhs._detail->context == _detail->context;
}

/// Returns a hash value for the state.
size_t
GarchNSGLContextState::GetHash() const
{
    return static_cast<size_t>(reinterpret_cast<uintptr_t>(_detail->context));
}

/// Returns \c true if the context state is valid.
bool
GarchNSGLContextState::IsValid() const
{
    return _detail->context != nil;
}

/// Make the context current.
void
GarchNSGLContextState::MakeCurrent()
{
#if defined(PXR_GL_SUPPORT_ENABLED)
#if defined(ARCH_OS_IPHONE)
    [EAGLContext setCurrentContext:_detail->context];
#else
    [_detail->context makeCurrentContext];
#endif
#endif
}

/// Make no context current.
void
GarchNSGLContextState::DoneCurrent()
{
#if defined(PXR_GL_SUPPORT_ENABLED)
#if defined(ARCH_OS_IPHONE)
    [EAGLContext setCurrentContext:nil];
#else
    [NSGLContext clearCurrentContext];
#endif
#endif
}

GarchGLPlatformContextState
GarchGetNullGLPlatformContextState()
{
    return GarchNSGLContextState(GarchNSGLContextState::NullState::nullstate);
}

void *
GarchSelectCoreProfileMacVisual()
{
#if defined(ARCH_OS_OSX)
    NSOpenGLPixelFormatAttribute attribs[10];
    int c = 0;

    attribs[c++] = NSOpenGLPFAOpenGLProfile;
    attribs[c++] = NSOpenGLProfileVersion3_2Core;
    attribs[c++] = NSOpenGLPFADoubleBuffer;
    attribs[c++] = 0;

    return [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
#else // ARCH_OS_MACOS
    return NULL;
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE
