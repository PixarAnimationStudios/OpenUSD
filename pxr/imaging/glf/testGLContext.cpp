//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/base/tf/diagnostic.h"

#ifdef PXR_X11_SUPPORT_ENABLED
#include "pxr/imaging/glf/testGLXContext.h"
#else
#include "pxr/imaging/glf/testGLNullContext.h"
#endif


PXR_NAMESPACE_OPEN_SCOPE


Glf_TestGLContextPrivate *
_GetSharedContext()
{
    static Glf_TestGLContextPrivate* sharedCtx = new Glf_TestGLContextPrivate();
    return sharedCtx;
}

//
// GlfTestGLContextRegistrationInterface
//

class GlfTestGLContextRegistrationInterface :
    public GlfGLContextRegistrationInterface {
public:
    GlfTestGLContextRegistrationInterface();
    virtual ~GlfTestGLContextRegistrationInterface();

    // GlfGLContextRegistrationInterface overrides
    virtual GlfGLContextSharedPtr GetShared();
    virtual GlfGLContextSharedPtr GetCurrent();
};

GlfTestGLContextRegistrationInterface::GlfTestGLContextRegistrationInterface()
{
    // Do nothing
}

GlfTestGLContextRegistrationInterface::~GlfTestGLContextRegistrationInterface()
{
    // Do nothing
}

GlfGLContextSharedPtr
GlfTestGLContextRegistrationInterface::GetShared()
{
    return GlfGLContextSharedPtr(new GlfTestGLContext(_GetSharedContext()));
}

GlfGLContextSharedPtr
GlfTestGLContextRegistrationInterface::GetCurrent()
{
    if (const Glf_TestGLContextPrivate* context =
            Glf_TestGLContextPrivate::currentContext()) {
        return GlfGLContextSharedPtr(new GlfTestGLContext(context));
    }
    return GlfGLContextSharedPtr();
}

//
// GlfTestGLContext
//

GlfTestGLContextSharedPtr 
GlfTestGLContext::Create( GlfTestGLContextSharedPtr const & share )
{
    Glf_TestGLContextPrivate * ctx = new Glf_TestGLContextPrivate( 
        share && share->_context ? share->_context : NULL );
    return GlfTestGLContextSharedPtr( new GlfTestGLContext( ctx ) );
}

void
GlfTestGLContext::RegisterGLContextCallbacks()
{
    new GlfTestGLContextRegistrationInterface;
}

GlfTestGLContext::GlfTestGLContext(Glf_TestGLContextPrivate const * context) :
    _context(const_cast<Glf_TestGLContextPrivate *>(context))
{
}

bool
GlfTestGLContext::IsValid() const
{
    return (_context && _context->isValid());
}

void
GlfTestGLContext::_MakeCurrent()
{
    _context->makeCurrent();
}

bool
GlfTestGLContext::_IsSharing(GlfGLContextSharedPtr const & otherContext)const
{
    GlfTestGLContextSharedPtr otherGlfTestGLContext =
        std::dynamic_pointer_cast<GlfTestGLContext>(otherContext);
    return (otherGlfTestGLContext &&
            Glf_TestGLContextPrivate::areSharing(_context, otherGlfTestGLContext->_context));
}

bool
GlfTestGLContext::_IsEqual(GlfGLContextSharedPtr const &rhs) const
{
    if (const GlfTestGLContext* rhsRaw =
            dynamic_cast<const GlfTestGLContext*>(rhs.get())) {
        return *_context == *rhsRaw->_context;
    }
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
