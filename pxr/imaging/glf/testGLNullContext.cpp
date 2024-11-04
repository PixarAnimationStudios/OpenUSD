//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "testGLNullContext.h"

PXR_NAMESPACE_OPEN_SCOPE

Glf_TestGLContextPrivate const * Glf_TestGLContextPrivate::_currenGLContext=nullptr;

Glf_TestGLContextPrivate::Glf_TestGLContextPrivate( Glf_TestGLContextPrivate const *)
{
}

void
Glf_TestGLContextPrivate::makeCurrent( ) const
{
    _currenGLContext=this;
}

bool
Glf_TestGLContextPrivate::isValid()
{
    return true;
}

bool
Glf_TestGLContextPrivate::operator==(const Glf_TestGLContextPrivate& rhs) const
{
    return true;
}

const Glf_TestGLContextPrivate *
Glf_TestGLContextPrivate::currentContext()
{
    return _currenGLContext;
}

bool
Glf_TestGLContextPrivate::areSharing(
    const Glf_TestGLContextPrivate * context1,
    const Glf_TestGLContextPrivate * context2)
{
    if (!context1 || !context2)
        return false;

    return context1->_sharedContext==context2->_sharedContext;
}

PXR_NAMESPACE_CLOSE_SCOPE
