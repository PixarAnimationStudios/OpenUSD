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
#include "pxr/imaging/garch/glPlatformDebugWindowDarwin.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/tf/diagnostic.h"

// ---------------------------------------------------------------------------

Garch_GLPlatformDebugWindow::Garch_GLPlatformDebugWindow(GarchGLDebugWindow *w)
    : _running(false)
    , _callback(w)
{
}

void
Garch_GLPlatformDebugWindow::Init(const char *title,
                                  int width, int height, int nSamples)
{
    // XXX: todo: implement window and context initialization

    _callback->OnInitializeGL();
}

static int
Garch_GetModifierKeys(int state)
{
    int keys = 0;
    // if (state & ShiftMask)   keys |= GarchGLDebugWindow::Shift;
    // if (state & ControlMask) keys |= GarchGLDebugWindow::Ctrl;
    // if (state & Mod1Mask)    keys |= GarchGLDebugWindow::Alt;
    return keys;
}

void
Garch_GLPlatformDebugWindow::Run()
{
    _running = true;

    // XXX: todo: implement event loop
}

void
Garch_GLPlatformDebugWindow::ExitApp()
{
    _running = false;
}
