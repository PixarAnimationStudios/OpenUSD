//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hgiPresent/testenv/testWindow.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

PXR_NAMESPACE_OPEN_SCOPE

class HgiPresentTestWindowX11 final : public HgiPresentTestWindow
{
public:
    HgiPresentTestWindowX11(const GfVec2i &size)
    {
        _display = XOpenDisplay(nullptr);
        if (!_display) {
            TF_RUNTIME_ERROR("Failed to open X11 display");
            return;
        }

        _window = XCreateSimpleWindow(_display, XDefaultRootWindow(_display), 0,
            0, size[0], size[1], 0, 0, 0);
        if (!_window) {
            TF_RUNTIME_ERROR("Failed to create X11 window");
            return;
        }

        XStoreName(_display, _window, "testHgiPresent X11");

        _deleteWindowMessage = XInternAtom(_display, "WM_DELETE_WINDOW", false);
        XSetWMProtocols(_display, _window, &_deleteWindowMessage, 1);

        XMapWindow(_display, _window);
        XFlush(_display);
    }

    ~HgiPresentTestWindowX11() override
    {
        if (_display) {
            if (_window) {
                XDestroyWindow(_display, _window);
            }
            XCloseDisplay(_display);
        }
    }

    HgiPresentWindowHandle GetHandle() const override
    {
        if (!_display || !_window) {
            return HgiPresentNullWindowHandle{};
        }

        return HgiPresentXlibWindowHandle{_display, _window};
    }

    bool Update() override
    {
        if (!_display || !_window) {
            return false;
        }

        while (XPending(_display)) {
            XEvent event;
            XNextEvent(_display, &event);
            if (event.type == ClientMessage &&
                static_cast<Atom>(event.xclient.data.l[0]) ==
                    _deleteWindowMessage) {
                return false;
            }
        }

        return true;
    }

    bool CaptureImage(HioImage::StorageSpec &storage,
        std::vector<uint8_t> &buffer) const override
    {
        if (!_display || !_window) {
            return false;
        }

        XWindowAttributes attributes;
        if (!XGetWindowAttributes(_display, _window, &attributes)) {
            return false;
        }

        XImage *image = XGetImage(_display, _window, 0, 0, attributes.width,
            attributes.height, AllPlanes, ZPixmap);
        if (!image) {
            return false;
        }

        // This assumes a full color display.
        buffer.resize(attributes.width * attributes.height * 3);
        for (int row = 0; row < attributes.height; row++) {
            for (int col = 0; col < attributes.width; col++) {
                const auto pixel = XGetPixel(image, col, row);
                const int i = (row * attributes.width + col) * 3;
                buffer[i + 0] = (pixel & image->red_mask) >> 16;
                buffer[i + 1] = (pixel & image->green_mask) >> 8;
                buffer[i + 2] = (pixel & image->blue_mask) >> 0;
            }
        }

        XFree(image);

        storage.format = HioFormatUNorm8Vec3srgb;
        storage.width = attributes.width;
        storage.height = attributes.height;
        storage.data = buffer.data();

        return true;
    }

private:
    Display *_display{};
    Window _window{};
    Atom _deleteWindowMessage;
};

std::unique_ptr<HgiPresentTestWindow>
HgiPresentTestCreateX11Window(const GfVec2i &size)
{
    return std::make_unique<HgiPresentTestWindowX11>(size);
}

PXR_NAMESPACE_CLOSE_SCOPE
