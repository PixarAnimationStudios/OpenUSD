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
#ifndef GLF_GLCONTEXT_H
#define GLF_GLCONTEXT_H

#include "pxr/base/arch/threads.h"
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<class GlfGLContext> GlfGLContextSharedPtr;

/// \class GlfGLContext
///
/// Provides window system independent access to GL contexts.
///
/// All OpenGL operation occurs within a current GL Context.  The GL
/// contexts used by an application are allocated and managed by the window
/// system interface layer, i.e. Qt, GLUT, GLX, etc.
///
/// This class provides a way for lower-level OpenGL framework code to
/// get useful information about the GL contexts in use by the application.
///
/// This mechanism depends on the application code registering callbacks to
/// provide access to its GL contexts.
///
class GlfGLContext : public boost::noncopyable {
public:
    virtual ~GlfGLContext();

    /// Returns an instance for the current GL context.
    static GlfGLContextSharedPtr GetCurrentGLContext();

    /// Returns an instance for the shared GL context.
    static GlfGLContextSharedPtr GetSharedGLContext();

    /// Makes \p context current if valid, otherwise makes no context current.
    static void MakeCurrent(const GlfGLContextSharedPtr& context);

    /// Returns \c true if \a context1 and \a context2 are sharing.
    static bool AreSharing(GlfGLContextSharedPtr const & context1,
                           GlfGLContextSharedPtr const & context2);

    /// Returns whether this interface has been initialized.
    static bool IsInitialized();

    /// Returns \c true if this context is current.
    bool IsCurrent() const;

private:
    /// Makes this context current.
    ///
    /// If the context is not valid this does nothing.
    void MakeCurrent();

public:
    /// Makes no context current.
    static void DoneCurrent();

    /// Returns \c true if this context is sharing with \a otherContext.
    bool IsSharing(GlfGLContextSharedPtr const & otherContext);

    /// Returns \c true if this context is valid.
    virtual bool IsValid() const = 0;

protected:
    GlfGLContext();

    /// Makes this context current.
    virtual void _MakeCurrent() = 0;

    /// Returns \c true if this context is sharing with \a rhs.
    virtual bool _IsSharing(const GlfGLContextSharedPtr& rhs) const = 0;

    /// Returns \c true if this context is equal to \p rhs.
    virtual bool _IsEqual(const GlfGLContextSharedPtr& rhs) const = 0;
};

/// \class GlfGLContextScopeHolder
///
/// Helper class to make a GL context current.
///
/// It is often useful to wrap a dynamic GL resource with a class interface.
///
/// In addition to providing API to make it more convenient to use the
/// underlying GL resource, the lifetime of the underlying resource can
/// be tied to the lifetime of a wrapper object instance, e.g. allocate
/// the GL resource during construction, delete the GL resource during
/// destruction.
///
/// While the construction and use of these kinds of wrapper objects is
/// usually pretty safe and straightforward, it can be more difficult to
/// manage destruction.  Specifically, it can be hard to guarantee that
/// a suitable GL context is current at the time that a wrapper object
/// instance is destroyed.  If a suitable context is not current, then
/// it will not be possible to delete the underlying resource, which
/// may cause the resource to remain allocated, which will then result
/// in a resource leak.
///
/// Typically, these GL resources are allocated from contexts which are
/// sharing the GL resources.  In which case it is sufficient for any one
/// one of the sharing contexts to be current in order to be able to safely
/// delete the GL resource from the destructor of a wrapper object.
///
/// GlfGLContext and GlfGLContextScopeHolder can help.
///
/// When GlfGLContext has been initialized, i.e. when suitable context
/// callbacks have been registered, we can use GlfGLContext to make
/// a GL context current.
///
/// GlfGLContextScopeHolder does this automatically for the duration
/// of a code block.
///
/// The underlying calls to make GL contexts current can be moderately
/// expensive.  So, this mechanism should be used carefully.
///
class GlfGLContextScopeHolder : boost::noncopyable {
public:
    /// Make the given context current and restore the current context
    /// when this object is destroyed.
    explicit GlfGLContextScopeHolder(const GlfGLContextSharedPtr& newContext);

    ~GlfGLContextScopeHolder();

protected:
    void _MakeNewContextCurrent();
    void _RestoreOldContext();

private:
    GlfGLContextSharedPtr _newContext;
    GlfGLContextSharedPtr _oldContext;
};

/// \class GlfSharedGLContextScopeHolder
///
/// Helper class to make the shared GL context current.
///
/// Example:
///
/// \code
///     class MyTexture {
///     public:
///         MyTexture() : _textureId(0) {
///             // allocate from the shared context pool.
///             GlfSharedGLContextScopeHolder sharedContextScopeHolder;
///             glGenTextures(1, &_textureId);
///         }
///
///         ~MyTexture() {
///             // delete from the shared context pool.
///             GlfSharedGLContextScopeHolder sharedContextScopeHolder;
///             glDeleteTextures(1, &_texureId);
///             _textureId = 0;
///         }
///
///         // The caller is responsible for making sure that a suitable
///         // GL context is current before calling other methods.
///
///         void Bind() {
///             glBindTexture(GL_TEXTURE_2D, _textureId);
///         }
///
///         void Unbind() {
///             glBindTexture(GL_TEXTURE_2D, 0);
///         }
///
///         ...
///
///     private:
///         GLuint _textureId;
///
///     };
/// \endcode
///
class GlfSharedGLContextScopeHolder : private GlfGLContextScopeHolder {
public:
    GlfSharedGLContextScopeHolder() :
        GlfGLContextScopeHolder(_GetSharedContext())
    {
        // Do nothing
    }

private:
    static GlfGLContextSharedPtr _GetSharedContext()
    {
        if (GlfGLContext::IsInitialized()
#ifdef MENV30
            // XXX skip this test for globaltrees until shared_code lands
            and ArchIsMainThread()
#endif
            ) {
            return GlfGLContext::GetSharedGLContext();
        }
        return GlfGLContextSharedPtr();
    }
};

/// \class GlfGLContextRegistrationInterface
///
/// Interface for registering a GlfGLContext system.
///
/// If you subclass GlfGLContext you should subclass this type and
/// instantiate an instance on the heap.  It will be cleaned up
/// automatically.
class GlfGLContextRegistrationInterface : boost::noncopyable {
public:
    virtual ~GlfGLContextRegistrationInterface();

    /// If this GLContext system supports a shared context this should
    /// return it.  This will be called at most once.
    virtual GlfGLContextSharedPtr GetShared() = 0;

    /// Whatever your GLContext system thinks is the current GL context
    /// may not really be the current context if another system has since
    /// changed the context.  This method should return what it thinks is
    /// the current context.  If it thinks there is no current context it
    /// should return \c NULL.
    virtual GlfGLContextSharedPtr GetCurrent() = 0;

protected:
    GlfGLContextRegistrationInterface();
};

#endif
