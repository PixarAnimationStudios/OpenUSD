//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GARCH_GL_PLATFORM_DEBUG_CONTEXT_H
#define PXR_IMAGING_GARCH_GL_PLATFORM_DEBUG_CONTEXT_H

#include "pxr/pxr.h"
#include "pxr/imaging/garch/api.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/weakBase.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


class GarchGLPlatformDebugContextPrivate;

TF_DECLARE_WEAK_AND_REF_PTRS(GarchGLPlatformDebugContext);

/// \class GarchGLPlatformDebugContext
///
/// Platform specific context (e.g. X11/GLX) which supports debug output.
///
class GarchGLPlatformDebugContext : public TfRefBase, public TfWeakBase {
public:
    
    static GarchGLPlatformDebugContextRefPtr
    New(int majorVersion, int minorVersion, bool coreProfile,
        bool directRenderering) {
        return TfCreateRefPtr(
            new GarchGLPlatformDebugContext(
                majorVersion, minorVersion, coreProfile, directRenderering));
    }

    virtual ~GarchGLPlatformDebugContext();

    GARCH_API
    static bool IsEnabledDebugOutput();

    GARCH_API
    static bool IsEnabledCoreProfile();

    GARCH_API
    void makeCurrent();

    GARCH_API
    void *chooseMacVisual();

public:
    std::unique_ptr<GarchGLPlatformDebugContextPrivate> _private;
    bool _coreProfile;

protected:
    GARCH_API
    GarchGLPlatformDebugContext(int majorVersion,
                               int minorVersion,
                               bool coreProfile,
                               bool directRenderering);

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_GARCH_GL_PLATFORM_DEBUG_CONTEXT_H
