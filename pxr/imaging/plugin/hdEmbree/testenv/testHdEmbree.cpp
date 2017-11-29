//
// Copyright 2017 Pixar
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
#include "pxr/pxr.h"

#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/glf/drawTarget.h"

#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hd/unitTestDelegate.h"

#include "pxr/imaging/hdx/renderTask.h"

#include "pxr/imaging/hdSt/camera.h"

#include "pxr/imaging/hdEmbree/rendererPlugin.h"
#include "pxr/imaging/hdEmbree/renderDelegate.h"

#include "pxr/base/tf/errorMark.h"

#include <embree2/rtcore.h>
#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

// ------------------------------------------------------
// HdSt_UnitTestGLDrawing is test scaffolding; it can create a window
// and render into it, or do a headless render into a PNG file.
// Extend it to draw a simple embree scene.

class HdEmbree_TestGLDrawing : public HdSt_UnitTestGLDrawing {
public:
    HdEmbree_TestGLDrawing()
        : _smooth(false)
        , _instance(false)
        , _refined(false)
        , _outputName("color1.png")
    {
        SetCameraRotate(0,0);
        SetCameraTranslate(GfVec3f(0));
    }

    // Populate the test scene with objects.
    virtual void InitTest();
    // Execute the render task.
    virtual void DrawTest();
    // Execute the render task, and write the output to a file.
    virtual void OffscreenTest();
    // De-populate the scene.
    virtual void UninitTest();

protected:
    // Give the test class a chance to parse command line arguments.
    virtual void ParseArgs(int argc, char *argv[]);

private:

    // HdEngine is the entry point for generating images with hydra. The
    // data-flow is as follows:
    //
    // HdRenderIndex manages scene state. It creates a "prim" for each scene
    // object, which stores drawing data for that object. The render delegate
    // is responsible for providing an implementation for each prim type
    // (e.g. "mesh"), and storing data in a format useful for that type of
    // renderer (e.g. OpenGL buffers, Embree scene objects). Each render
    // index is bound to a single render delegate.
    //
    // HdSceneDelegate (or derived classes like Hd_UnitTestDelegate) provide
    // accessors for scene data for the prims they are supporting. Each
    // scene delegate is bound to a single render index, but a render index
    // can support many scene delegates (although only one per prim).
    //
    // Hd_UnitTestDelegate implements the scene data accessors for the
    // render index to use, and also exports an application-facing API to
    // create basic objects like a camera or a cube.
    //
    // Image generation is done via HdEngine::Execute(). Execute() will
    // first call HdRenderIndex::SyncAll(), giving each prim a chance to
    // update invalidated data by calling scene delegate accessors.
    // Then it runs the tasks passed in as arguments. The simplest
    // invocation executes a single render task, which draws the scene to
    // the framebuffer.
    //
    // HdxRendererPlugin (or derived classes like HdEmbreeRendererPlugin)
    // are a discoverable way to create render delegates.

    HdEngine _engine;
    HdEmbreeRendererPlugin *_rendererPlugin;
    HdRenderDelegate *_renderDelegate;
    HdRenderIndex *_renderIndex;
    Hd_UnitTestDelegate *_sceneDelegate;

    // Scene options:
    // - Use smooth normals, or flat normals?
    // - Draw a scene with two instanced cubes?
    //   Or two normal cubes and a plane?
    // - Treat the cubes as subdivision surfaces, and refine them to spheres?
    bool _smooth;
    bool _instance;
    bool _refined;

    // For offscreen tests, what file do we write to?
    std::string _outputName;
};

void HdEmbree_TestGLDrawing::InitTest()
{
    // This test bypasses the hydra plugin system; it creates an embree
    // renderer plugin directly, and then a render delegate, and then
    // a render index.
    _rendererPlugin = new HdEmbreeRendererPlugin;
    TF_VERIFY(_rendererPlugin != nullptr);

    _renderDelegate = _rendererPlugin->CreateRenderDelegate();
    TF_VERIFY(_renderDelegate != nullptr);

    _renderIndex = HdRenderIndex::New(_renderDelegate);
    TF_VERIFY(_renderIndex != nullptr);

    // Construct a new scene delegate to populate the render index.
    _sceneDelegate = new Hd_UnitTestDelegate(
            _renderIndex, SdfPath::AbsoluteRootPath());
    TF_VERIFY(_sceneDelegate != nullptr);

    // Create a camera (it's populated later).
    SdfPath camera("/camera");
    _sceneDelegate->AddCamera(camera);

    // Prepare an embree render task.  The render task is responsible for
    // rendering the scene.
    SdfPath renderTask("/renderTask");
    _sceneDelegate->AddTask<HdxRenderTask>(renderTask);

    // Params is a general argument structure to the render task.
    // - Specify the camera to render from.
    // - Specify the viewport size.
    HdxRenderTaskParams params;
    params.camera = camera;
    params.viewport = GfVec4f(0, 0, GetWidth(), GetHeight());
    _sceneDelegate->UpdateTask(renderTask, HdTokens->params, VtValue(params));

    // Collection specifies which HdRprimCollection we want to render,
    // and with what draw style.

    // This test doesn't have multiple collections, so we can use the
    // default collection HdTokens->geometry.  We don't explicitly specify
    // include/exclude paths, so all geometry is included.

    // There are several pre-defined repr tokens. Some that we make use of:
    // - HdTokens->hull is the flat-shaded, unrefined mesh.
    // - HdTokens->smoothHull is the smooth-shaded, unrefined mesh.
    // - HdTokens->refined is the smooth-shaded, refined mesh.

    if (_refined) {
        _sceneDelegate->UpdateTask(renderTask, HdTokens->collection,
                VtValue(HdRprimCollection(HdTokens->geometry, 
                HdTokens->refined)));
    } else {
        _sceneDelegate->UpdateTask(renderTask, HdTokens->collection,
                VtValue(HdRprimCollection(HdTokens->geometry, 
                _smooth ? HdTokens->smoothHull : HdTokens->hull)));
    }

    // Tasks can have child tasks that get scheduled together.
    // We don't use this here.
    _sceneDelegate->UpdateTask(renderTask, HdTokens->children,
            VtValue(SdfPathVector()));

    if (_instance) {
        // Instanced scene. Add test geometry:
        // - Proto cube.
        // - Instancer for cube (prototype 0), with transforms:
        //    (-3, 0, 5),
        //    ( 3, 0, 5)
        _sceneDelegate->AddInstancer(SdfPath("/instancer"));
        _sceneDelegate->AddCube(SdfPath("/protoCube"), GfMatrix4f(1),
                                false, SdfPath("/instancer"));

        VtIntArray index;
        VtVec3fArray translate, scale;
        VtVec4fArray rotate;

        index.push_back(0);
        translate.push_back(GfVec3f(-3, 0, 5));
        rotate.push_back(GfVec4f(1, 0, 0, 0));
        scale.push_back(GfVec3f(1, 1, 1));

        index.push_back(0);
        translate.push_back(GfVec3f(3, 0, 5));
        rotate.push_back(GfVec4f(1, 0, 0, 0));
        scale.push_back(GfVec3f(1, 1, 1));
        
        _sceneDelegate->SetInstancerProperties(SdfPath("/instancer"),
            index, scale, rotate, translate);
    }
    else {
        // Un-instanced scene. Add test geometry:
        // - A grid on the XY plane, uniformly scaled up by 10.
        // - A cube at (-3, 0, 5).
        // - A cube at (3, 0, 5), rotated around the Z axis by 30 degrees.
        GfMatrix4d gridXf(10);
        _sceneDelegate->AddGrid(SdfPath("/grid"), 1, 1, GfMatrix4f(gridXf));

        GfMatrix4d cube1Xf(1);
        cube1Xf.SetTranslateOnly(GfVec3d(-5,0,1));
        _sceneDelegate->AddCube(SdfPath("/cube1"), GfMatrix4f(cube1Xf));

        GfMatrix4d cube2Xf(1);
        cube2Xf.SetRotateOnly(GfRotation(GfVec3d(0,0,1), 30));
        cube2Xf.SetTranslateOnly(GfVec3d(5,0,1));
        _sceneDelegate->AddCube(SdfPath("/cube2"), GfMatrix4f(cube2Xf));
    }

    if (_refined) {
        // If we're supposed to refine, tell the geometry how many
        // times to recursively subdivide.
        _sceneDelegate->SetRefineLevel(4);
    }

    // Configure the camera looking slightly down on the objects.
    GfFrustum frustum;
    frustum.SetNearFar(GfRange1d(0.1, 1000.0));
    frustum.SetPosition(GfVec3d(0, -5, 10));
    frustum.SetRotation(GfRotation(GfVec3d(1, 0, 0), 45));

    _sceneDelegate->UpdateCamera(camera,
        HdStCameraTokens->worldToViewMatrix,
        VtValue(frustum.ComputeViewMatrix()));
    _sceneDelegate->UpdateCamera(camera,
        HdStCameraTokens->projectionMatrix,
        VtValue(frustum.ComputeProjectionMatrix()));
};

void HdEmbree_TestGLDrawing::DrawTest()
{
    // The GL viewport needs to be set before calling execute.
    glViewport(0, 0, GetWidth(), GetHeight());

    // Ask hydra to execute our render task (producing an image).
    HdTaskSharedPtrVector tasks;
    tasks.push_back(_renderIndex->GetTask(SdfPath("/renderTask")));
    _engine.Execute(*_renderIndex, tasks);
}

void HdEmbree_TestGLDrawing::OffscreenTest()
{
    // Render and write out to a file.
    DrawTest();
    WriteToFile("color", _outputName.c_str());
}

void HdEmbree_TestGLDrawing::UninitTest()
{
    // Deconstruct hydra state.
    delete _sceneDelegate;
    delete _renderIndex;
    _rendererPlugin->DeleteRenderDelegate(_renderDelegate);
    delete _rendererPlugin;
}

void HdEmbree_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
    // Parse command line for variant switches:
    // - Flat/smooth shading (default = flat)
    // - Whether to test instancing (default = no)
    // - Whether to refine (default = no)
    // - Where to write offscreen test output.
    for (int i=0; i<argc; ++i) {
        if (std::string(argv[i]) == "--flat") {
            _smooth = false;
        } else if (std::string(argv[i]) == "--smooth") {
            _smooth = true;
        } else if (std::string(argv[i]) == "--instance") {
            _instance = true;
        } else if (std::string(argv[i]) == "--refined") {
            _refined = true;
        } else if (std::string(argv[i]) == "--write" &&
                   (i+1) < argc) {
            _outputName = std::string(argv[i+1]);
        }
    }
}

int main(int argc, char *argv[])
{
    TfErrorMark mark;

    HdEmbree_TestGLDrawing driver;

    // RunTest() is the main loop of the unit test window; it responds to
    // UI events and calls DrawTest() or OffscreenTest() as necessary.
    driver.RunTest(argc, argv);

    // If no error messages were logged, return success.
    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
