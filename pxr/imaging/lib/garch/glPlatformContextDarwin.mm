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
#import <Foundation/Foundation.h>
#import <AppKit/NSOpenGL.h>

#include "pxr/pxr.h"
#include "glPlatformContextDarwin.h"

#ifdef ARCH_OS_IOS
typedef EAGLContext NSGLContext;
#else
typedef NSOpenGLContext NSGLContext;
#endif

PXR_NAMESPACE_OPEN_SCOPE

class GarchNSGLContextState::Detail
{
public:
    Detail() {
        context = [NSGLContext currentContext];
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
    return rhs._detail->context = _detail->context;
}

/// Returns a hash value for the state.
size_t
GarchNSGLContextState::GetHash() const
{
    return (size_t) _detail->context;
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
#if ARCH_OS_IOS
    [EAGLContext setCurrentContext:_detail->context];
#else
    [_detail->context makeCurrentContext];
#endif
}

/// Make no context current.
void
GarchNSGLContextState::DoneCurrent()
{
    [NSGLContext clearCurrentContext];
}

GarchGLPlatformContextState
GarchGetNullGLPlatformContextState()
{
    return GarchNSGLContextState(GarchNSGLContextState::NullState::nullstate);
}

void *
GarchSelectCoreProfileMacVisual()
{
    NSOpenGLPixelFormatAttribute attribs[10];
    int c = 0;

    attribs[c++] = NSOpenGLPFAOpenGLProfile;
    attribs[c++] = NSOpenGLProfileVersion3_2Core;
    attribs[c++] = NSOpenGLPFADoubleBuffer;
    attribs[c++] = 0;

    return [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
}

PXR_NAMESPACE_CLOSE_SCOPE
