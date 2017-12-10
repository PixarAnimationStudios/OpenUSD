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
#ifndef HDST_UNIT_TEST_DRAWING_GL
#define HDST_UNIT_TEST_DRAWING_GL

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

    virtual void InitTest() = 0;
    virtual void DrawTest() = 0;        // interactive mode
    virtual void OffscreenTest() = 0;   // offscreen mode (automated test)

    HDST_API
    virtual void MousePress(int button, int x, int y);
    HDST_API
    virtual void MouseRelease(int button, int x, int y);
    HDST_API
    virtual void MouseMove(int x, int y);
    HDST_API
    virtual void KeyRelease(int key);

    HDST_API
    virtual void Idle();

    HDST_API
    bool WriteToFile(std::string const & attachment,
                     std::string const & filename) const;

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

private:
    HdSt_UnitTestWindow *_widget;
    float _rotate[2];
    GfVec3f _translate;

    int _mousePos[2];
    bool _mouseButton[3];
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDST_UNIT_TEST_DRAWING_GL
