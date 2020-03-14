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
#ifndef PXR_IMAGING_GLF_TEST_GLCONTEXT_H
#define PXR_IMAGING_GLF_TEST_GLCONTEXT_H

/// \file glf/testGLContext.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/glf/glContext.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class Glf_TestGLContextPrivate;

typedef std::shared_ptr<class GlfTestGLContext> GlfTestGLContextSharedPtr;

/// \class GlfTestGLContext
///
/// Testing support class for GlfGLContext.
///
class GlfTestGLContext : public GlfGLContext {
public:
    GLF_API
    static void RegisterGLContextCallbacks();

    // GlfGLContext overrides
    GLF_API
    virtual bool IsValid() const;

    GLF_API
    static GlfTestGLContextSharedPtr Create( GlfTestGLContextSharedPtr const & share );

protected:
    // GlfGLContext overrides
    GLF_API
    virtual void _MakeCurrent();
    GLF_API
    virtual bool _IsSharing(const GlfGLContextSharedPtr& rhs) const;
    GLF_API
    virtual bool _IsEqual(const GlfGLContextSharedPtr& rhs) const;

private:
    GlfTestGLContext(Glf_TestGLContextPrivate const * context);

    friend class GlfTestGLContextRegistrationInterface;

private:
    Glf_TestGLContextPrivate * _context;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_GLF_TEST_GLCONTEXT_H
