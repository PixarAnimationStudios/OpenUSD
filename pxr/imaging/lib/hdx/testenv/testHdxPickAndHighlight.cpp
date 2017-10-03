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
#include "pxr/pxr.h"

#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/garch/glDebugWindow.h"
#include "pxr/imaging/glf/drawTarget.h"

#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hdSt/renderDelegate.h"

#include "pxr/imaging/hdx/intersector.h"
#include "pxr/imaging/hdx/selectionTask.h"
#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"
#include "pxr/imaging/hdx/unitTestGLDrawing.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <unordered_set>
#include <boost/scoped_ptr.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

class My_TestGLDrawing : public Hdx_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        SetCameraRotate(0, 0);
        SetCameraTranslate(GfVec3f(0));
        _reprName = HdTokens->hull;
        _refineLevel = 0;
    }
    ~My_TestGLDrawing();

    void DrawScene();
    void DrawMarquee();
    void Pick(GfVec2i const &startPos, 
              GfVec2i const &endPos,
              HdxSelectionHighlightMode const& mode);

    // Hdx_UnitTestGLDrawing overrides
    virtual void InitTest();
    virtual void UninitTest();
    virtual void DrawTest();
    virtual void OffscreenTest();

    virtual void MousePress(int button, int x, int y, int modKeys);
    virtual void MouseRelease(int button, int x, int y, int modKeys);
    virtual void MouseMove(int x, int y, int modKeys);

protected:
    virtual void ParseArgs(int argc, char *argv[]);

private:
    HdEngine _engine;
    HdStRenderDelegate _renderDelegate;
    HdRenderIndex *_renderIndex;
    boost::scoped_ptr<HdxIntersector> _intersector;
    boost::scoped_ptr<Hdx_UnitTestDelegate> _delegate;
    HdxSelectionTrackerSharedPtr _selectionTracker;

    TfToken _reprName;
    int _refineLevel;
    GfVec2i _startPos, _endPos;
    GLuint _vbo;
    GLuint _program;
};

////////////////////////////////////////////////////////////

GLuint vao;

static GfMatrix4d
_GetTranslate(float tx, float ty, float tz)
{
    GfMatrix4d m(1.0f);
    m.SetRow(3, GfVec4f(tx, ty, tz, 1.0));
    return m;
}

namespace {
    struct HitHash {
        // make a partial hash excluding elementId, ndcDepth, wsHitPoint,
        // allowing us to group hits to different elements of the same object
        // instance
        size_t operator()(HdxIntersector::Hit const& hit) const {
            size_t hash = 0;
            boost::hash_combine(hash, hit.delegateId.GetHash());
            boost::hash_combine(hash, hit.objectId.GetHash());
            boost::hash_combine(hash, hit.instancerId.GetHash());
            boost::hash_combine(hash, hit.instanceIndex);
            return hash;
        }
    };

    struct HitEq {
        bool operator()(HdxIntersector::Hit const& a, 
                        HdxIntersector::Hit const& b) const {
            return a.delegateId    == b.delegateId    &&
                   a.objectId      == b.objectId      &&
                   a.instancerId   == b.instancerId   &&
                   a.instanceIndex == b.instanceIndex;
        }
    };
}


My_TestGLDrawing::~My_TestGLDrawing()
{
    _intersector.reset();
    _selectionTracker.reset();
    _delegate.reset();
    delete _renderIndex;
}

void
My_TestGLDrawing::InitTest()
{
    // init hud
    glGenBuffers(1, &_vbo);
    _program = glCreateProgram();
    const char *sources[1];
    sources[0] =
        "#version 430                                         \n"
        "in vec2 position;                                    \n"
        "void main() {                                        \n"
        "  gl_Position = vec4(position.x, position.y, 0, 1);  \n"
        "}                                                    \n";

    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, sources, NULL);
    glCompileShader(vShader);

    sources[0] =
        "#version 430                                         \n"
        "out vec4 outColor;                                   \n"
        "void main() {                                        \n"
        "  outColor = vec4(1);                                \n"
        "}                                                    \n";

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, sources, NULL);
    glCompileShader(fShader);

    glAttachShader(_program, vShader);
    glAttachShader(_program, fShader);

    glLinkProgram(_program);

    glDeleteShader(vShader);
    glDeleteShader(fShader);

    _renderIndex = HdRenderIndex::New(&_renderDelegate);
    TF_VERIFY(_renderIndex != nullptr);
    _intersector.reset(new HdxIntersector(_renderIndex));
    _delegate.reset(new Hdx_UnitTestDelegate(_renderIndex));
    _delegate->SetRefineLevel(_refineLevel);
    _selectionTracker.reset(new HdxSelectionTracker());

    // prepare render task
    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");
    SdfPath selectionTask("/selectionTask");
    _delegate->AddRenderSetupTask(renderSetupTask);
    _delegate->AddRenderTask(renderTask);
    _delegate->AddSelectionTask(selectionTask);

    // render task parameters.
    VtValue vParam = _delegate->GetTaskParam(renderSetupTask, HdTokens->params);
    HdxRenderTaskParams param = vParam.Get<HdxRenderTaskParams>();
    param.enableLighting = true; // use default lighting
    _delegate->SetTaskParam(renderSetupTask, HdTokens->params,
                            VtValue(param));
    _delegate->SetTaskParam(renderTask, HdTokens->collection,
                            VtValue(HdRprimCollection(HdTokens->geometry, _reprName)));
    HdxSelectionTaskParams selParam;
    selParam.enableSelection = true;
    selParam.selectionColor = GfVec4f(1, 1, 0, 1);
    selParam.locateColor = GfVec4f(1, 0, 1, 1);
    selParam.maskColor = GfVec4f(0, 1, 1, 1);
    _delegate->SetTaskParam(selectionTask, HdTokens->params,
                            VtValue(selParam));

    // prepare scene
    _delegate->AddCube(SdfPath("/cube0"), _GetTranslate( 5, 0, 5));
    _delegate->AddCube(SdfPath("/cube1"), _GetTranslate(-5, 0, 5));
    _delegate->AddCube(SdfPath("/cube2"), _GetTranslate(-5, 0,-5));
    _delegate->AddCube(SdfPath("/cube3"), _GetTranslate( 5, 0,-5));

    {
        _delegate->AddInstancer(SdfPath("/instancerTop"));
        _delegate->AddCube(SdfPath("/protoTop"),
                         GfMatrix4d(1), false, SdfPath("/instancerTop"));

        std::vector<SdfPath> prototypes;
        prototypes.push_back(SdfPath("/protoTop"));

        VtVec3fArray scale(3);
        VtVec4fArray rotate(3);
        VtVec3fArray translate(3);
        VtIntArray prototypeIndex(3);

        scale[0] = GfVec3f(1);
        rotate[0] = GfVec4f(0);
        translate[0] = GfVec3f(3, 0, 2);
        prototypeIndex[0] = 0;

        scale[1] = GfVec3f(1);
        rotate[1] = GfVec4f(0);
        translate[1] = GfVec3f(0, 0, 2);
        prototypeIndex[1] = 0;

        scale[2] = GfVec3f(1);
        rotate[2] = GfVec4f(0);
        translate[2] = GfVec3f(-3, 0, 2);
        prototypeIndex[2] = 0;

        _delegate->SetInstancerProperties(SdfPath("/instancerTop"),
                                        prototypeIndex,
                                        scale, rotate, translate);
    }

    {
        _delegate->AddInstancer(SdfPath("/instancerBottom"));
        _delegate->AddTet(SdfPath("/protoBottom"),
                         GfMatrix4d(1), false, SdfPath("/instancerBottom"));
        _delegate->SetRefineLevel(SdfPath("/protoBottom"), 2);

        std::vector<SdfPath> prototypes;
        prototypes.push_back(SdfPath("/protoBottom"));

        VtVec3fArray scale(3);
        VtVec4fArray rotate(3);
        VtVec3fArray translate(3);
        VtIntArray prototypeIndex(3);

        scale[0] = GfVec3f(1);
        rotate[0] = GfVec4f(0);
        translate[0] = GfVec3f(3, 0, -2);
        prototypeIndex[0] = 0;

        scale[1] = GfVec3f(1);
        rotate[1] = GfVec4f(0);
        translate[1] = GfVec3f(0, 0, -2);
        prototypeIndex[1] = 0;

        scale[2] = GfVec3f(1);
        rotate[2] = GfVec4f(0);
        translate[2] = GfVec3f(-3, 0, -2);
        prototypeIndex[2] = 0;

        _delegate->SetInstancerProperties(SdfPath("/instancerBottom"),
                                        prototypeIndex,
                                        scale, rotate, translate);
    }


    SetCameraTranslate(GfVec3f(0, 0, -20));

    // XXX: Setup a VAO, the current drawing engine will not yet do this.
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindVertexArray(0);
}

void
My_TestGLDrawing::UninitTest()
{
    _intersector.reset();
    glDeleteProgram(_program);
    glDeleteBuffers(1, &_vbo);
}

void
My_TestGLDrawing::DrawTest()
{
    GLfloat clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, clearColor);

    GLfloat clearDepth[1] = { 1.0f };
    glClearBufferfv(GL_DEPTH, 0, clearDepth);

    DrawScene();

    DrawMarquee();
}

void
My_TestGLDrawing::OffscreenTest()
{
    GLfloat clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, clearColor);

    GLfloat clearDepth[1] = { 1.0f };
    glClearBufferfv(GL_DEPTH, 0, clearDepth);

    DrawScene();
    WriteToFile("color", "color1_unselected.png");

    // --------------------- (active) selection high ---------------------------
    // select cube2
    HdxSelectionHighlightMode mode = HdxSelectionHighlightModeSelect;
    Pick(GfVec2i(180, 390), GfVec2i(181, 391), mode);
    DrawScene();
    WriteToFile("color", "color2_select.png");
    HdxSelectionSharedPtr selection = _selectionTracker->GetSelectionMap();
    TF_VERIFY(selection->GetSelectedPrims(mode).size() == 1);
    TF_VERIFY(selection->GetSelectedPrims(mode)[0] == SdfPath("/cube2"));

    // select cube1, /protoTop:1, /protoTop:2, /protoBottom:1, /protoBottom:2
    Pick(GfVec2i(105,62), GfVec2i(328,288), mode);
    DrawScene();
    WriteToFile("color", "color3_select.png");
    selection = _selectionTracker->GetSelectionMap();

    TF_VERIFY(selection->GetSelectedPrims(mode).size() == 5);
    TF_VERIFY(selection->GetSelectedInstances(mode).size() == 2);
    {
        std::vector<VtIntArray> const& indices
            = *TfMapLookupPtr(selection->GetSelectedInstances(mode),
                             SdfPath("/protoTop"));
        TF_VERIFY(indices.size() == 2);
        TF_VERIFY(indices[0][0] == 1 || indices[0][0] == 2);
        TF_VERIFY(indices[1][0] == 1 || indices[1][0] == 2);
    }
    {
        std::vector<VtIntArray> const& indices
            = *TfMapLookupPtr(selection->GetSelectedInstances(mode),
                              SdfPath("/protoBottom"));
        TF_VERIFY(indices.size() == 2);
        TF_VERIFY(indices[0][0] == 1 || indices[0][0] == 2);
        TF_VERIFY(indices[1][0] == 1 || indices[1][0] == 2);
    }

    // --------------------- locate (rollover) selection -----------------------
    mode = HdxSelectionHighlightModeLocate;
    // select cube0
    Pick(GfVec2i(472, 97), GfVec2i(473, 98), mode);
    DrawScene();
    WriteToFile("color", "color4_locate.png");
    selection = _selectionTracker->GetSelectionMap();
    TF_VERIFY(selection->GetSelectedPrims(mode).size() == 1);
    TF_VERIFY(selection->GetSelectedPrims(mode)[0] == SdfPath("/cube0"));

    // select cube3, /protoBottom:0
    Pick(GfVec2i(408,246), GfVec2i(546,420), mode);
    DrawScene();
    WriteToFile("color", "color5_locate.png");
    selection = _selectionTracker->GetSelectionMap();

    TF_VERIFY(selection->GetSelectedPrims(mode).size() == 2);
    TF_VERIFY(selection->GetSelectedInstances(mode).size() == 1);
    {
        std::vector<VtIntArray> const& indices
            = *TfMapLookupPtr(selection->GetSelectedInstances(mode),
                             SdfPath("/protoBottom"));
        TF_VERIFY(indices.size() == 1);
        TF_VERIFY(indices[0][0] == 0);
    }

    // ------------------------- mask  selection -------------------------------
    mode = HdxSelectionHighlightModeMask;
    // select cube2
    Pick(GfVec2i(180, 390), GfVec2i(181, 391), mode);
    DrawScene();
    WriteToFile("color", "color6_mask.png");

     // select cube3, /protoBottom:0
    Pick(GfVec2i(408,246), GfVec2i(546,420), mode);
    DrawScene();
    WriteToFile("color", "color7_mask.png");
    selection = _selectionTracker->GetSelectionMap();

    TF_VERIFY(selection->GetSelectedPrims(mode).size() == 2);
    TF_VERIFY(selection->GetSelectedInstances(mode).size() == 1);
    {
        std::vector<VtIntArray> const& indices
            = *TfMapLookupPtr(selection->GetSelectedInstances(mode),
                             SdfPath("/protoBottom"));
        TF_VERIFY(indices.size() == 1);
        TF_VERIFY(indices[0][0] == 0);
    }

    // deselect    
    Pick(GfVec2i(0,0), GfVec2i(0,0), mode);
    DrawScene();
    WriteToFile("color", "color8_unselected.png");
}

void
My_TestGLDrawing::DrawScene()
{
    int width = GetWidth(), height = GetHeight();

    GfMatrix4d viewMatrix = GetViewMatrix();
    GfFrustum frustum = GetFrustum();

    GfVec4d viewport(0, 0, width, height);

    GfMatrix4d projMatrix = frustum.ComputeProjectionMatrix();
    _delegate->SetCamera(viewMatrix, projMatrix);

    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");
    SdfPath selectionTask("/selectionTask");

    // viewport
    HdxRenderTaskParams param
        = _delegate->GetTaskParam(
            renderSetupTask, HdTokens->params).Get<HdxRenderTaskParams>();
    param.viewport = viewport;
    _delegate->SetTaskParam(renderSetupTask, HdTokens->params, VtValue(param));

    HdTaskSharedPtrVector tasks;
    tasks.push_back(_delegate->GetRenderIndex().GetTask(renderSetupTask));
    tasks.push_back(_delegate->GetRenderIndex().GetTask(renderTask));
    tasks.push_back(_delegate->GetRenderIndex().GetTask(selectionTask));

    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(vao);

    VtValue v(_selectionTracker);
    _engine.SetTaskContextData(HdxTokens->selectionState, v);

    _engine.Execute(_delegate->GetRenderIndex(), tasks);

    glBindVertexArray(0);
}

void
My_TestGLDrawing::DrawMarquee()
{
    glDisable(GL_DEPTH_TEST);
    glUseProgram(_program);

    float width = GetWidth(), height = GetHeight();
    GfVec2f s(2*_startPos[0]/width-1,
              1-2*_startPos[1]/height);
    GfVec2f e(2*_endPos[0]/width-1,
              1-2*_endPos[1]/height);
    float pos[] = { s[0], s[1], e[0], s[1],
                    e[0], e[1], s[0], e[1],
                    s[0], s[1] };

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pos), pos, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_LINE_STRIP, 0, 5);

    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    glUseProgram(0);
    glEnable(GL_DEPTH_TEST);
}

void
My_TestGLDrawing::MousePress(int button, int x, int y, int modKeys)
{
    Hdx_UnitTestGLDrawing::MousePress(button, x, y, modKeys);
    _startPos = _endPos = GetMousePos();
}

void
My_TestGLDrawing::MouseRelease(int button, int x, int y, int modKeys)
{
    Hdx_UnitTestGLDrawing::MouseRelease(button, x, y, modKeys);

    if (!(modKeys & GarchGLDebugWindow::Alt)) {
        Pick(_startPos, _endPos, HdxSelectionHighlightModeSelect);
    }
    _startPos = _endPos = GfVec2i(0);
}

void
My_TestGLDrawing::Pick(GfVec2i const &startPos, 
                       GfVec2i const &endPos,
                       HdxSelectionHighlightMode const& mode)
{
    int fwidth  = std::max(4, std::abs(startPos[0] - endPos[0]));
    int fheight = std::max(4, std::abs(startPos[1] - endPos[1]));

    _intersector->SetResolution(GfVec2i(fwidth, fheight));

    float width = GetWidth(), height = GetHeight();

    GfFrustum frustum = GetFrustum();
    GfVec2d min(2*startPos[0]/width-1, 1-2*startPos[1]/height);
    GfVec2d max(2*(endPos[0]+1)/width-1, 1-2*(endPos[1]+1)/height);
    // scale window
    GfVec2d origin = frustum.GetWindow().GetMin();
    GfVec2d scale = frustum.GetWindow().GetMax() - frustum.GetWindow().GetMin();
    min = origin + GfCompMult(scale, 0.5 * (GfVec2d(1.0, 1.0) + min));
    max = origin + GfCompMult(scale, 0.5 * (GfVec2d(1.0, 1.0) + max));

    frustum.SetWindow(GfRange2d(min, max));

    HdxIntersector::Params params;
    params.hitMode = HdxIntersector::HitFirst;
    params.projectionMatrix = frustum.ComputeProjectionMatrix();
    params.viewMatrix =  GetViewMatrix();

    std::cout << "Pick " << startPos << " - " << endPos << "\n";

    HdxIntersector::Result result;
    HdRprimCollection col(HdTokens->geometry, _reprName);
    _intersector->Query(params, col, &_engine, &result);

    HdxIntersector::HitSet hits;
    HdxSelectionSharedPtr selection(new HdxSelection);
    if (result.ResolveUnique(&hits)) {
        std::unordered_set<HdxIntersector::Hit, HitHash, HitEq> aggregatedHits;

        // Aggregate hits to the same object instance (see HitHash)
        TF_FOR_ALL(it, hits) {
            aggregatedHits.insert(*it);
            std::cout << "object: " << it->objectId << " "
                      << "instancer: " << it->instancerId << " "
                      << "instanceIndex: " << it->instanceIndex << " "
                      << "elementIndex: " << it->elementIndex << " "
                      << "hit: " << it->worldSpaceHitPoint << " "
                      << "ndcDepth: " << it->ndcDepth << "\n";

        }

        for(const auto& hit : aggregatedHits) {
            if (!hit.instancerId.IsEmpty()) {
                // XXX :this doesn't work for nested instancing.
                VtIntArray instanceIndex;
                instanceIndex.push_back(hit.instanceIndex);
                selection->AddInstance(mode, hit.objectId, instanceIndex);
                // we should use GetPathForInstanceIndex instead of it->objectId
                //SdfPath path = _delegate->GetPathForInstanceIndex(it->objectId, it->instanceIndex);
                // and also need to add some APIs to compute VtIntArray instanceIndex.
            } else {
                selection->AddRprim(mode, hit.objectId);
            }
        }
    }

    _selectionTracker->SetSelection(selection);
}

void
My_TestGLDrawing::MouseMove(int x, int y, int modKeys)
{
    Hdx_UnitTestGLDrawing::MouseMove(x, y, modKeys);

    if (!(modKeys & GarchGLDebugWindow::Alt)) {
        _endPos = GetMousePos();
    }
}

void
My_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
    for (int i=0; i<argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--repr") {
            _reprName = TfToken(argv[++i]);
        } else if (arg == "--refineLevel") {
            _refineLevel = atoi(argv[++i]);
        }
    }
}

void
BasicTest(int argc, char *argv[])
{
    My_TestGLDrawing driver;

    driver.RunTest(argc, argv);
}

int main(int argc, char *argv[])
{
    TfErrorMark mark;

    BasicTest(argc, argv);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

