#include "pxr/imaging/glf/glew.h"

#include "pxr/usdImaging/usdImaging/delegate.h"

#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/frustum.h"

#include <QApplication>
#include <iostream>

static std::string
_FindDataFile(const std::string& file)
{
    static std::once_flag importOnce;
    std::call_once(importOnce, [](){
        const std::string importFindDataFile = "from Mentor.Runtime import *";
        if (TfPyRunSimpleString(importFindDataFile) != 0) {
            TF_FATAL_ERROR("ERROR: Could not import FindDataFile");
        }
    });

    const std::string findDataFile =
        TfStringPrintf("FindDataFile(\'%s\')", file.c_str());
    using namespace boost::python;
    const object resultObj(TfPyRunString(findDataFile, Py_eval_input));
    const extract<std::string> dataFileObj(resultObj);

    if (not dataFileObj.check()) {
        TF_FATAL_ERROR("ERROR: Could not extract result of FindDataFile");
        return std::string();
    }
    return dataFileObj();
}


class Offscreen {
public:
    Offscreen(std::string const &outPrefix) {
        _count = 0;
        _outPrefix = outPrefix;
        _drawTarget = GlfDrawTarget::New(GfVec2i(512, 512));
        _drawTarget->Bind();
        _drawTarget->AddAttachment("color", GL_RGBA, GL_FLOAT, GL_RGBA);
        _drawTarget->AddAttachment("depth", GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,
                                   GL_DEPTH24_STENCIL8);
        _drawTarget->Unbind();
    }

    void Begin() {
        GLfloat clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        GLfloat clearDepth[1] = { 1.0f };

        _drawTarget->Bind();
        glClearBufferfv(GL_COLOR, 0, clearColor);
        glClearBufferfv(GL_DEPTH, 0, clearDepth);
    }

    void End() {
        _drawTarget->Unbind();

        if (_outPrefix.size() > 0) {
            std::string filename = TfStringPrintf(
                "%s_%d.png", _outPrefix.c_str(), _count);
            _drawTarget->WriteToFile("color", filename);
            std::cerr << "**Write to " << filename << "\n";
        }
        ++_count;
    }

private:
    int _count;
    std::string _outPrefix;
    GlfDrawTargetRefPtr _drawTarget;

};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfGlewInit();
    GlfSharedGLContextScopeHolder sharedContext;

    std::string outPrefix;
    std::string filePath;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--output") {
            outPrefix = argv[++i];
        } else {
            filePath = argv[i];
        }
    }

    if (filePath.empty()) {
        std::cout << "Usage: " << argv[0] << " [--output <filename>] stage.usd\n";
        return EXIT_FAILURE;
    }

    UsdStageRefPtr stage = UsdStage::Open(_FindDataFile(filePath));

    HdEngine engine;
    HdRenderIndexSharedPtr renderIndex(new HdRenderIndex());
    // intentionally specify delegateID to test indexPath-usdPath conversion.
    UsdImagingDelegate delegate(renderIndex, SdfPath("/delegateId"));
    delegate.Populate(stage->GetPseudoRoot());
    delegate.SetTime(1.0);

    // prep draw target
    Offscreen offscreen(outPrefix);

    HdRenderPassSharedPtr renderPass(
        new HdRenderPass(
            &delegate.GetRenderIndex(),
            HdRprimCollection(HdTokens->geometry, HdTokens->smoothHull)));
    HdRenderPassStateSharedPtr state(new HdRenderPassState());

    GfMatrix4d viewMatrix;
    viewMatrix.SetLookAt(GfVec3d(10, 20, 20),
                         GfVec3d(10, 0, 0),
                         GfVec3d(0, 1, 0));
    GfFrustum frustum;
    frustum.SetPerspective(60.0, true, 1.0, 0.1, 100.0);
    state->SetCamera(viewMatrix,
                     frustum.ComputeProjectionMatrix(),
                     GfVec4d(0, 0, 512, 512));

    // initial draw
    glViewport(0, 0, 512, 512);
    glEnable(GL_DEPTH_TEST);

    offscreen.Begin();
    engine.Draw(delegate.GetRenderIndex(), renderPass, state);
    offscreen.End();

    /*  in test.usda

       /Cube
       /Foo/X1/C1     (instance)
       /Foo/X2/C2     (instance)
       /Foo/X3/C3     (instance)
       /Foo/Bar/C
       /Foo/Bar/X4/C4 (instance)
       /Bar/C
       /Bar/X5/C5     (instance)
     */

    // Set root transform
    delegate.SetRootTransform(GfMatrix4d(1,0,0,0,0,1,0,0,0,0,1,0,1,0,0,1));

    offscreen.Begin();
    engine.Draw(delegate.GetRenderIndex(), renderPass, state);
    offscreen.End();

    // Reset root transform
    delegate.SetRootTransform(GfMatrix4d(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1));

    offscreen.Begin();
    engine.Draw(delegate.GetRenderIndex(), renderPass, state);
    offscreen.End();

    // Set rigid xform override
    UsdImagingDelegate::RigidXformOverridesMap overrides;
    overrides[SdfPath("/Foo/X2")] = GfMatrix4d(1,0,0,0,0,1,0,0,0,0,1,0, 1,0, 0,1);
    overrides[SdfPath("/Bar")]    = GfMatrix4d(1,0,0,0,0,1,0,0,0,0,1,0, 0,5,-5,1);

    delegate.SetRigidXformOverrides(overrides);

    offscreen.Begin();
    engine.Draw(delegate.GetRenderIndex(), renderPass, state);
    offscreen.End();

    // Set root transform again (+rigid xform)
    delegate.SetRootTransform(GfMatrix4d(1,0,0,0,0,1,0,0,0,0,1,0,2,0,0,1));

    offscreen.Begin();
    engine.Draw(delegate.GetRenderIndex(), renderPass, state);
    offscreen.End();

    // Invis cube
    SdfPathVector invisedPaths;
    invisedPaths.push_back(SdfPath("/Cube"));
    delegate.SetInvisedPrimPaths(invisedPaths);

    offscreen.Begin();
    engine.Draw(delegate.GetRenderIndex(), renderPass, state);
    offscreen.End();

    // Invis instances
    invisedPaths.push_back(SdfPath("/Foo/X2"));
    invisedPaths.push_back(SdfPath("/Foo/Bar/X4/C4"));
    delegate.SetInvisedPrimPaths(invisedPaths);

    offscreen.Begin();
    engine.Draw(delegate.GetRenderIndex(), renderPass, state);
    offscreen.End();

    // un-invis
    delegate.SetInvisedPrimPaths(SdfPathVector());
    offscreen.Begin();
    engine.Draw(delegate.GetRenderIndex(), renderPass, state);
    offscreen.End();

    // Set rigid xform override, overlapped
    overrides.clear();

    overrides[SdfPath("/Foo")]
        = GfMatrix4d(1,0,0,0,0,1,0,0,0,0,1,0,1,0,0,1);
    overrides[SdfPath("/Foo/Bar")]
        = GfMatrix4d(1,0,0,0,0,1,0,0,0,0,1,0,0,1,0,1);
    overrides[SdfPath("/Foo/Bar/X4")]
        = GfMatrix4d(1,0,0,0,0,1,0,0,0,0,1,0,0,0,6,1);

    delegate.SetRigidXformOverrides(overrides);

    offscreen.Begin();
    engine.Draw(delegate.GetRenderIndex(), renderPass, state);
    offscreen.End();

    std::cout << "OK" << std::endl;
    return EXIT_SUCCESS;
}

