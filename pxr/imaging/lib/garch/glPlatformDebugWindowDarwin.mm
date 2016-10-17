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

#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>

static int
Garch_GetModifierKeys(NSUInteger flags)
{
    int keys = 0;

    if (flags & NSEventModifierFlagShift)   keys |= GarchGLDebugWindow::Shift;
    if (flags & NSEventModifierFlagControl) keys |= GarchGLDebugWindow::Ctrl;
    if (flags & NSEventModifierFlagOption)  keys |= GarchGLDebugWindow::Alt;
    if (flags & NSEventModifierFlagCommand) keys |= GarchGLDebugWindow::Alt;

    return keys;
}

@class  View;

@interface View : NSOpenGLView <NSWindowDelegate>
{
    GarchGLDebugWindow *_callback;
    NSOpenGLContext *_ctx;
}

@end

@implementation View

-(id)initGL:(NSRect)frame callback:(GarchGLDebugWindow*)cb
{
    _callback = cb;

    int attribs[] = {
        NSOpenGLPFAAccelerated,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        NSOpenGLPFAColorSize, 32,
        NSOpenGLPFANoRecovery,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
        0
    };

    NSOpenGLPixelFormat *pf
        = [[NSOpenGLPixelFormat alloc]
           initWithAttributes:(NSOpenGLPixelFormatAttribute*)attribs];
    self = [self initWithFrame:frame pixelFormat:pf];

    _ctx = [[NSOpenGLContext alloc] initWithFormat:pf shareContext:nil];

    [self setOpenGLContext:_ctx];

    [_ctx makeCurrentContext];

    _callback->OnInitializeGL();

    [pf release];

    return self;
}

-(bool)acceptsFirstResponder
{
    return YES;
}

-(void)drawRect:(NSRect)theRect
{
    [_ctx makeCurrentContext];

    _callback->OnPaintGL();

    [[self openGLContext] flushBuffer];
}

-(void)windowWillClose:(NSNotification*)notification
{
    [[NSApplication sharedApplication] terminate:self];
}

-(void)windowDidResize:(NSNotification *)notification
{
    NSRect r = [self frame];
    _callback->OnResize(r.size.width, r.size.height);
}

-(void)mouseDown:(NSEvent*)event
{
    NSPoint p = [event locationInWindow];
    NSRect r = [self frame];
    NSUInteger modflags = [event modifierFlags];
    _callback->OnMousePress(GarchGLDebugWindow::Button1,
                            p.x, r.size.height - 1 - p.y,
                            Garch_GetModifierKeys(modflags));

    [self setNeedsDisplay:YES];
}

-(void)mouseUp:(NSEvent*)event
{
    NSPoint p = [event locationInWindow];
    NSRect r = [self frame];
    NSUInteger modflags = [event modifierFlags];
    _callback->OnMouseRelease(GarchGLDebugWindow::Button1,
                              p.x, r.size.height - 1 - p.y,
                              Garch_GetModifierKeys(modflags));

    [self setNeedsDisplay:YES];
}

-(void)mouseDragged:(NSEvent*)event
{
    NSPoint p = [event locationInWindow];
    NSRect r = [self frame];
    NSUInteger modflags = [event modifierFlags];
    _callback->OnMouseMove(p.x, r.size.height - 1 - p.y,
                           Garch_GetModifierKeys(modflags));

    [self setNeedsDisplay:YES];
}

- (void)keyDown:(NSEvent *)event
{
    int keyCode = [event keyCode];
    int key = 0;

    // XXX shoud call UCKeyTranslate() for non-us keyboard
    const int keyMap[] = { 0x00, 'a', 0x0b, 'b', 0x08, 'c', 0x02, 'd',
                           0x0e, 'e', 0x03, 'f', 0x05, 'g', 0x04, 'h',
                           0x22, 'i', 0x26, 'j', 0x28, 'k', 0x25, 'l',
                           0x2e, 'm', 0x2d, 'n', 0x1f, 'o', 0x23, 'p',
                           0x0c, 'q', 0x0f, 'r', 0x01, 's', 0x11, 't',
                           0x20, 'u', 0x09, 'v', 0x0d, 'w', 0x07, 'x',
                           0x10, 'y', 0x06, 'z', 0x31, ' ', -1, -1};

    for (int i = 0; keyMap[i] >=0; i += 2) {
        if (keyMap[i] == keyCode) {
            key = keyMap[i+1];
            break;
        }
    }
    if (key) {
        _callback->OnKeyRelease(key);
    }

    [self setNeedsDisplay:YES];
}

@end

// ---------------------------------------------------------------------------

Garch_GLPlatformDebugWindow::Garch_GLPlatformDebugWindow(GarchGLDebugWindow *w)
    : _callback(w)
{
}

void
Garch_GLPlatformDebugWindow::Init(const char *title,
                                  int width, int height, int nSamples)
{
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    id applicationName = [[NSProcessInfo processInfo] processName];

    NSRect frame = NSMakeRect(0, 0, width, height);
    NSRect viewBounds = NSMakeRect(0, 0, width, height);

    View *view = [[View alloc] initGL:viewBounds callback:_callback];

    NSWindow *window = [[NSWindow alloc]
                        initWithContentRect:frame
                        styleMask:NSTitledWindowMask
                                 |NSClosableWindowMask
                                 |NSMiniaturizableWindowMask
                                 |NSResizableWindowMask
                        backing:NSBackingStoreBuffered
                        defer:NO];
    [window cascadeTopLeftFromPoint:NSMakePoint(20,20)];
    [window setTitle: applicationName];
    [window makeKeyAndOrderFront:nil];

    [NSApp activateIgnoringOtherApps:YES];

    [window setContentView:view];
    [window setDelegate:view];
}

void
Garch_GLPlatformDebugWindow::Run()
{
    [NSApp run];
}

void
Garch_GLPlatformDebugWindow::ExitApp()
{
    [NSApp stop:nil];
}
