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
#ifndef USDIMAGINGGL_UNIT_TEST_GL_DRAWING_H
#define USDIMAGINGGL_UNIT_TEST_GL_DRAWING_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec4d.h"

#include "pxr/base/tf/declarePtrs.h"

#include "pxr/usdImaging/usdImagingGL/engine.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class UsdImagingGL_UnitTestWindow;

/// \class UsdImagingGL_UnitTestGLDrawing
///
/// A helper class for unit tests which need to perform GL drawing.
///
class UsdImagingGL_UnitTestGLDrawing {
public:
    UsdImagingGL_UnitTestGLDrawing();
    virtual ~UsdImagingGL_UnitTestGLDrawing();

    int GetWidth() const;
    int GetHeight() const;

    bool IsEnabledTestLighting() const { return _testLighting; }
    bool IsEnabledCameraLight() const { return _cameraLight; }
    bool IsEnabledCullBackfaces() const { return _cullBackfaces; }
    bool IsEnabledIdRender() const { return _testIdRender; }

    UsdImagingGLEngine::DrawMode GetDrawMode() const { return _drawMode; }

    std::string const & GetStageFilePath() const { return _stageFilePath; }
    std::string const & GetOutputFilePath() const { return _outputFilePath; }

    std::vector<GfVec4d> const & GetClipPlanes() const { return _clipPlanes; }
    std::vector<double> const& GetTimes() const { return _times; }
    GfVec4f const & GetClearColor() const { return _clearColor; }
    GfVec3f const & GetTranslate() const { return _translate; }

    void RunTest(int argc, char *argv[]);

    virtual void InitTest() = 0;
    virtual void DrawTest(bool offscreen) = 0;
    virtual void ShutdownTest() { }

    virtual void MousePress(int button, int x, int y, int modKeys);
    virtual void MouseRelease(int button, int x, int y, int modKeys);
    virtual void MouseMove(int x, int y, int modKeys);
    virtual void KeyRelease(int key);

    bool WriteToFile(std::string const & attachment, std::string const & filename) const;

protected:
    float _GetComplexity() const { return _complexity; }
    bool _ShouldFrameAll() const { return _shouldFrameAll; }

private:
    struct _Args;
    void _Parse(int argc, char *argv[], _Args* args);

private:
    UsdImagingGL_UnitTestWindow *_widget;
    bool _testLighting;
    bool _cameraLight;
    bool _testIdRender;

    std::string _stageFilePath;
    std::string _outputFilePath;

    float _complexity;
    std::vector<double> _times;

    std::vector<GfVec4d> _clipPlanes;

    UsdImagingGLEngine::DrawMode _drawMode;
    bool _shouldFrameAll;
    bool _cullBackfaces;
    GfVec4f _clearColor;
    GfVec3f _translate;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDIMAGINGGL_UNIT_TEST_GL_DRAWING_H
