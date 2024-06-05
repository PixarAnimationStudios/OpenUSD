//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_UNIT_TEST_GLDRAWING_H
#define PXR_IMAGING_HD_ST_UNIT_TEST_GLDRAWING_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3f.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE


class HdSt_UnitTestWindow;

/// \class HdSt_UnitTestGLDrawing
///
/// A helper class for unit tests which need to perform GL drawing.
///
class HdSt_UnitTestGLDrawing {
public:
    HDST_API
    HdSt_UnitTestGLDrawing();
    HDST_API
    virtual ~HdSt_UnitTestGLDrawing();

    HDST_API
    int GetWidth() const;
    HDST_API
    int GetHeight() const;
    HDST_API
    void RunTest(int argc, char *argv[]);
    HDST_API
    void RunOffscreenTest();

    virtual void InitTest() = 0;
    HDST_API virtual void UninitTest();
    virtual void DrawTest() = 0;        // interactive mode
    virtual void OffscreenTest() = 0;   // offscreen mode (automated test)

    HDST_API
    virtual void MousePress(int button, int x, int y, int modKeys);
    HDST_API
    virtual void MouseRelease(int button, int x, int y, int modKeys);
    HDST_API
    virtual void MouseMove(int x, int y, int modKeys);
    HDST_API
    virtual void KeyRelease(int key);

    HDST_API
    virtual void Idle();

    HDST_API
    virtual void Present(uint32_t framebuffer) {
        // do nothing
    }

protected:
    HDST_API
    virtual void ParseArgs(int argc, char *argv[]);

    void SetCameraRotate(float rx, float ry) {
        _rotate[0] = rx; _rotate[1] = ry;
    }
    void SetCameraTranslate(GfVec3f t) {
        _translate = t;
    }
    GfVec3f GetCameraTranslate() const {
        return _translate;
    }
    HDST_API
    GfMatrix4d GetViewMatrix() const;
    HDST_API
    GfMatrix4d GetProjectionMatrix() const;
    HDST_API
    GfFrustum GetFrustum() const;

    GfVec2i GetMousePos() const { return GfVec2i(_mousePos[0], _mousePos[1]); }

private:
    HdSt_UnitTestWindow *_widget;
    float _rotate[2];
    GfVec3f _translate;

    int _mousePos[2];
    bool _mouseButton[3];
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_UNIT_TEST_GLDRAWING_H
