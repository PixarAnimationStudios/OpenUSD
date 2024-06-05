//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GLF_GL_CONTEXT_REGISTRY_H
#define PXR_IMAGING_GLF_GL_CONTEXT_REGISTRY_H

/// \file glf/glContextRegistry.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/base/tf/singleton.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


struct GlfGLContextRegistry_Data;

typedef std::shared_ptr<class GlfGLContext> GlfGLContextSharedPtr;

/// \class GlfGLContextRegistry
///
/// Registry of GlfGLContexts.
///
class GlfGLContextRegistry
{
public:
    static GlfGLContextRegistry& GetInstance()
    {
        return TfSingleton<GlfGLContextRegistry>::GetInstance();
    }

    /// Returns whether the registry has any registered interfaces.
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

    // Non-copyable
    GlfGLContextRegistry(const GlfGLContextRegistry &) = delete;
    GlfGLContextRegistry &operator=(const GlfGLContextRegistry &) = delete;

    friend class TfSingleton<GlfGLContextRegistry>;

private:
    std::vector<std::unique_ptr<GlfGLContextRegistrationInterface>> _interfaces;
    bool _sharedContextInitialized;
    GlfGLContextSharedPtr _shared;
    std::unique_ptr<GlfGLContextRegistry_Data> _data;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_GLF_GL_CONTEXT_REGISTRY_H
