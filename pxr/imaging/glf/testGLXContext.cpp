//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "testGLXContext.h"

PXR_NAMESPACE_OPEN_SCOPE

Glf_TestGLContextPrivate const * Glf_TestGLContextPrivate::_currenGLContext=nullptr;
GLXWindow Glf_TestGLContextPrivate::_win=0;

Glf_TestGLContextPrivate::Glf_TestGLContextPrivate( Glf_TestGLContextPrivate const * other )
{
    static int attribs[] = { GLX_DOUBLEBUFFER, GLX_RGBA_BIT,
            GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8,
            GLX_SAMPLE_BUFFERS, 1, GLX_SAMPLES, 4, None };

    _dpy = XOpenDisplay(0);

    int n;
    GLXFBConfig * fbConfigs = glXChooseFBConfig( _dpy,
        DefaultScreen(_dpy), attribs, &n );

    GLXContext share = other ? other->_context : 0;

    _context = glXCreateNewContext( _dpy,
        fbConfigs[0], GLX_RGBA_TYPE, share, true);

    _sharedContext=other ? other : this;

    if (!_win) {
        XVisualInfo * vi = glXGetVisualFromFBConfig( _dpy, fbConfigs[0] );

        XSetWindowAttributes  swa;
        swa.colormap = XCreateColormap(_dpy, RootWindow(_dpy, vi->screen),
                 vi->visual, AllocNone);
        swa.border_pixel = 0;
        swa.event_mask = StructureNotifyMask;

        Window xwin = XCreateWindow( _dpy, RootWindow(_dpy, vi->screen),
                0, 0, 256, 256, 0, vi->depth, InputOutput, vi->visual,
	        CWBorderPixel|CWColormap|CWEventMask, &swa );

        _win = glXCreateWindow( _dpy, fbConfigs[0], xwin, NULL );
    }
}

void
Glf_TestGLContextPrivate::makeCurrent( ) const
{
    glXMakeContextCurrent(_dpy, _win, _win, _context);
    _currenGLContext=this;
}

bool
Glf_TestGLContextPrivate::isValid()
{
    return _context!=nullptr;
}

bool
Glf_TestGLContextPrivate::operator==(const Glf_TestGLContextPrivate& rhs) const
{
    return _dpy == rhs._dpy && _context == rhs._context;
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
