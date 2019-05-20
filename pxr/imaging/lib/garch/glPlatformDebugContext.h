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
#ifndef GARCH_GLPLATFORM_DEBUG_CONTEXT_H
#define GARCH_GLPLATFORM_DEBUG_CONTEXT_H

#include "pxr/pxr.h"
#include "pxr/imaging/garch/api.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/weakBase.h"

#include <boost/scoped_ptr.hpp>

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
    boost::scoped_ptr<GarchGLPlatformDebugContextPrivate> _private;
    bool _coreProfile;

protected:
    GARCH_API
    GarchGLPlatformDebugContext(int majorVersion,
                               int minorVersion,
                               bool coreProfile,
                               bool directRenderering);

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // GARCH_GLPLATFORM_DEBUG_CONTEXT_H
