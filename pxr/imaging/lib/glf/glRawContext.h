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
#ifndef GLF_GLRAWCONTEXT_H
#define GLF_GLRAWCONTEXT_H

/// \file glf/glRawContext.h

#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/garch/glPlatformContext.h"

typedef boost::shared_ptr<class GlfGLRawContext> GlfGLRawContextSharedPtr;

class GlfGLRawContext : public GlfGLContext {
public:
    /// Returns a new object with the current context.
    static GlfGLRawContextSharedPtr New();

    /// Returns a new object with the given state.
    static GlfGLRawContextSharedPtr New(const GarchGLPlatformContextState&);

    virtual ~GlfGLRawContext();

    /// Returns the held state.
    const GarchGLPlatformContextState& GetState() const { return _state; }

    // GlfGLContext overrides
    virtual bool IsValid() const;

protected:
    // GlfGLContext overrides
    virtual void _MakeCurrent();
    virtual bool _IsSharing(const GlfGLContextSharedPtr& rhs) const;
    virtual bool _IsEqual(const GlfGLContextSharedPtr& rhs) const;

private:
    GlfGLRawContext(const GarchGLPlatformContextState&);

private:
    GarchGLPlatformContextState _state;
};

#endif  // GLF_GLRAWCONTEXT_H
