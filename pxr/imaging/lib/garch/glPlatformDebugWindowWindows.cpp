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
#include "pxr/imaging/garch/glPlatformDebugWindowWindows.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/tf/diagnostic.h"

// ---------------------------------------------------------------------------
Garch_GLPlatformDebugWindow::_className = _T("GarchGLDebugWindow");

Garch_GLPlatformDebugWindow::Garch_GLPlatformDebugWindow(GarchGLDebugWindow *w)
    : _running(false)
    , _hWND(NULL)
    , _hDC(NULL)
    , _hGLRC(NULL)
{
}

void
Garch_GLPlatformDebugWindow::Init(const char *title,
                                  int width, int height, int nSamples)
{
    // platform initialize
    WNDCLASS wc;
    HINSTANCE _hInstnace = GetModuleHandle(NULL);
    if (GetClassInfo(_hInstance, _className, &wc) == 0) {
        ZeroMemory(&wc, sizeof(WNDCLASS));

        wc.lpfnWndProc   = MsgProc; // XXX:
        wc.hInstance     = _hInstance;
        wc.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = _className;

        if (RegisterClass(&wc) == 0) {
            TF_FATAL_ERROR("RegisterClass failed");
            exit(1);
        }
    }

    // XXX: todo: add support multi sampling

    DWORD flags = WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
    DWORD exFlags = 0;

    _hWND = CreateWindowEX(exFlags, _className,
                           title, flags, 100, 100, width, height,
                           (HWND)NULL, (HMENU)NULL, _hInstance,
                           (LPVOID)NULL);
    ShowWidnow(_hWND, SW_SHOW);
    _windows[_hWND] = this;
    _hDC = GetDC(_hWND);

    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));

    pfd.nSize       = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion   = 1;
    pfd.dwFlags    = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cRedBits   = 8;
    pfd.cGreenBits = 8;
    pfd.cBlueBits  = 8;
    pfd.cAlphaBits = 8;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;

    int pixelformat = ChoosePixelFormat(_hDC, &pfd);

    if (SetPixelFormat(_hDC, pixelformat, &pfd) == 0) {
        TF_FATAL_ERROR("SetPixelFormat failed");
        exit(1);
    }

    _hGLRC = wglCreateContext(_hDC);
    if (_hGLRC == 0) {
        TF_FATAL_ERROR("wglCreateContext failed");
        exit(1);
    }

    wglMakeCurrent(_hDC, _hGLRC);
    _callback->OnInitializeGL();
}

static int
Garch_GetModifierKeys(WPARAM wParam)
{
    int keys = 0;
    if (wParam & MK_SHIFT)   keys |= GarchGLDebugWindow::Shift;
    if (wParam & MK_CONTROL) keys |= GarchGLDebugWindow::Ctrl;
    if (HIBYTE(GetKeyState(VK_MENU)) & 0x80)
        keys |= GarchGLDebugWindow::Alt;
    return keys;
}

/* static */
Garch_GLPlatformDebugWindow *
Garch_GLPlatformDebugWindow::_GetWindowByHandle(HWND hWND)
{
    std::map<HWND, Garch_GLPlatformDebugWindow*>::iterator it =
        _windows.find(hWND);
    if (it != _windows.end()) {
        return it->second;
    }
    return NULL;
}

/* static */
LRESULT WINAPI
Garch_GLPlatformDebugWindow::_MsgProc(HWND hWnd, UINT msg,
                                     WPARAM wParam, LPARAM lParam)
{
    Garch_GLPlatformDebugWindow *window
        = Garch_GLPlatformDebugWindow::GetWindowByHandle(hWnd);
    if (not TF_VERIFY(window)) {
        return 0;
    }

    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    switch (msg) {
    case WM_SIZE:
        window->_callback->OnResize(
            HIWORD(lParam), LOWORD(lParam));
        break;
    case WM_LBUTTONDOWN:
        window->_callback->OnMousePressEvent(
            /*button=*/0, x, y, Garch_GetModifierKeys(wParam));
        break;
    case WM_MBUTTONDOWN:
        window->_callback->OnMousePressEvent(
            /*button=*/1, x, y, Garch_GetModifierKeys(wParam));
        break;
    case WM_RBUTTONDOWN:
        window->_callback->OnMousePressEvent(
            /*button=*/2, x, y, Garch_GetModifierKeys(wParam));
        break;
    case WM_LBUTTONUP:
        window->_callback->OnMouseReleaseEvent(
            /*button=*/0, x, y, Garch_GetModifierKeys(wParam));
        break;
    case WM_MBUTTONUP:
        window->_callback->OnMouseReleaseEvent(
            /*button=*/1, x, y, Garch_GetModifierKeys(wParam));
        break;
    case WM_RBUTTONUP:
        window->_callback->OnMouseReleaseEvent(
            /*button=*/2, x, y, Garch_GetModifierKeys(wParam));
        break;
    case WM_MOUSEMOVE:
        window->_callback->OnMouseMoveEvent(
            x, y, Garch_GetModifierKeys(wParam));
        break;
    case WM_KEYUP:
        window->_callback->OnKeyReleaseEvent(key);
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void
Garch_GLPlatformDebugWindow::Run()
{
    if (not _display) return;

    _running = true;

    MSG msg = {0};
    while (_running and message != WM_QUIT) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            // make current
            wglMakeCurrent(_hDC, _hGLRC);

            // XXX: this should be constant interval
            _callback->OnIdle();
            _callback->OnPaintGL();

            glFinish();

            SwapBuffers(_hDC);
        }
    }
    _callback->OnUninitializeGL();

    whlMakeCurrent(0, 0);
    // release GL
    wglDeleteContext(_hGLRC);

    _windows.remove(_hWND);
}

void
Garch_GLPlatformDebugWindow::ExitApp()
{
    _running = false;
}
