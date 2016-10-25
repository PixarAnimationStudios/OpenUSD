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

#include "pxr/imaging/garch/glDebugWindow.h"
#include "pxr/imaging/garch/glPlatformDebugWindowGLX.h"
#include "pxr/imaging/garch/glPlatformDebugContext.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/tf/diagnostic.h"

typedef GLXContext (*GLXCREATECONTEXTATTRIBSARBPROC)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

// ---------------------------------------------------------------------------

Garch_GLPlatformDebugWindow::Garch_GLPlatformDebugWindow(GarchGLDebugWindow *w)
    : _running(false)
    , _callback(w)
    , _display(NULL)
{
}

void
Garch_GLPlatformDebugWindow::Init(const char *title,
                                  int width, int height, int nSamples)
{
    int attrib[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, True,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_SAMPLE_BUFFERS, (nSamples > 1 ? 1 : 0),
        GLX_SAMPLES, nSamples,
        None
    };

    _display = XOpenDisplay(NULL);
    int screen = DefaultScreen(_display);
    Window root = RootWindow(_display, screen);

    // X window
    int fbcount;
    GLXFBConfig *fbc = glXChooseFBConfig(_display, screen, attrib, &fbcount);
    if (not fbc) {
        TF_FATAL_ERROR("glXChooseFBConfig failed");
        exit(1);
    }

    XVisualInfo *visinfo = glXGetVisualFromFBConfig(_display, fbc[0]);
    if (not visinfo) {
        TF_FATAL_ERROR("glXGetVisualFromFBConfig failed");
        exit(1);
    }

    XSetWindowAttributes attr;
    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = XCreateColormap(_display, root, visinfo->visual, AllocNone);
    attr.event_mask =
        StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask |
        PointerMotionMask | ButtonPressMask | ButtonReleaseMask;

    _window = XCreateWindow(_display, root, 0, 0,
                            width, height, 0,
                            visinfo->depth,
                            InputOutput,
                            visinfo->visual,
                            CWBackPixel|CWBorderPixel|CWColormap|CWEventMask,
                            &attr);
    XStoreName(_display, _window, title);

    // GL context
    GLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB
        = (GLXCREATECONTEXTATTRIBSARBPROC) glXGetProcAddress(
            (const GLubyte*)"glXCreateContextAttribsARB");

    int attribs[] = { 0 };
    GLXContext tmpCtx = glXCreateContextAttribsARB(_display, fbc[0], 0, true, attribs);
    glXMakeCurrent(_display, _window, tmpCtx);

    // switch to the debug context
    _glContext.reset(new GarchGLPlatformDebugContext(4, 5, true, true));
    _glContext->makeCurrent();
    glXDestroyContext(_display, tmpCtx);

    _callback->OnInitializeGL();
}

static int
Garch_GetButton(unsigned int button)
{
    if (button == Button1) return 0;
    else if (button == Button2) return 1;
    else if (button == Button3) return 2;
    return 0;
}

static int
Garch_GetModifierKeys(int state)
{
    int keys = 0;
    if (state & ShiftMask)   keys |= GarchGLDebugWindow::Shift;
    if (state & ControlMask) keys |= GarchGLDebugWindow::Ctrl;
    if (state & Mod1Mask)    keys |= GarchGLDebugWindow::Alt;
    return keys;
}

void
Garch_GLPlatformDebugWindow::Run()
{
    if (not _display) return;

    XMapWindow(_display, _window);

    _running = true;
    XEvent event;

    while (_running) {
        while (XPending(_display)) {

            XNextEvent(_display, &event);

            switch(event.type) {
            case Expose:
                break;
            case ConfigureNotify:
                _callback->OnResize(event.xconfigure.width,
                                    event.xconfigure.height);
                break;
            case ButtonPress:
                _callback->OnMousePress(
                    Garch_GetButton(event.xbutton.button),
                    event.xbutton.x,
                    event.xbutton.y,
                    Garch_GetModifierKeys(event.xbutton.state));
                break;
            case ButtonRelease:
                _callback->OnMouseRelease(
                    Garch_GetButton(event.xbutton.button),
                    event.xbutton.x,
                    event.xbutton.y,
                    Garch_GetModifierKeys(event.xbutton.state));
                break;
            case MotionNotify:
                _callback->OnMouseMove(
                    event.xmotion.x, event.xmotion.y,
                    Garch_GetModifierKeys(event.xbutton.state));
                break;
            case KeyRelease:
            {
                char key;
                XLookupString(&event.xkey, &key, 1, NULL, NULL);
                _callback->OnKeyRelease(key);
                break;
            }
            }
        }
        _glContext->makeCurrent();

        // XXX: this should be constant interval
        _callback->OnIdle();

        _callback->OnPaintGL();

        glFinish();
        glXSwapBuffers(_display, _window);
    }

    _callback->OnUninitializeGL();

    glXMakeCurrent(_display, 0, 0);
    _glContext.reset();
    XDestroyWindow(_display, _window);
    XCloseDisplay(_display);
}

void
Garch_GLPlatformDebugWindow::ExitApp()
{
    _running = false;
}
