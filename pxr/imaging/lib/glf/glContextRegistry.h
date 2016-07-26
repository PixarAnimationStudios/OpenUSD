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
/// \file glContextRegistry.h

#ifndef GLF_GLCONTEXTREGISTRY_H
#define GLF_GLCONTEXTREGISTRY_H

#include "pxr/imaging/glf/glContext.h"
#include "pxr/base/tf/singleton.h"
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

class GlfGLContextRegistry_Data;

typedef boost::shared_ptr<class GlfGLContext> GlfGLContextSharedPtr;

/// \class GlfGLContextRegistry
/// \brief Registry of GlfGLContexts.
class GlfGLContextRegistry : boost::noncopyable {
public:
    static GlfGLContextRegistry& GetInstance()
    {
        return TfSingleton<GlfGLContextRegistry>::GetInstance();
    }

    /// \brief Returns whether the registry has any registered interfaces.
    bool IsInitialized() const;

    /// Add a registration object to the registry.  This takes ownership
    /// of the object.
    void Add(GlfGLContextRegistrationInterface*);

    /// Returns the shared context, if any.
    GlfGLContextSharedPtr GetShared();

    /// Returns the context that matches the raw context, if any.
    GlfGLContextSharedPtr GetCurrent();

    /// Registers this context.  It must be current.
    void DidMakeCurrent(const GlfGLContextSharedPtr& context);

    /// Removes the context.
    void Remove(const GlfGLContext* context);

private:
    GlfGLContextRegistry();
    ~GlfGLContextRegistry();

    friend class TfSingleton<GlfGLContextRegistry>;

private:
    boost::ptr_vector<GlfGLContextRegistrationInterface> _interfaces;
    boost::optional<GlfGLContextSharedPtr> _shared;
    boost::scoped_ptr<GlfGLContextRegistry_Data> _data;
};

#endif  // GLF_GLCONTEXTREGISTRY_H
