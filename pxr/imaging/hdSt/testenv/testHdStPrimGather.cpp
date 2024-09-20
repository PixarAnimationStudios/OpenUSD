//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3d.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

// The id of each delegate.
// First one must be root.
// This list is expected to be sorted.
static const SdfPath NESTED_DELEGATE_PATHS[] =
{
  SdfPath("/"),
  SdfPath("/i"),
  SdfPath("/i/j/k"),
};
static const size_t NUM_NESTED_DELEAGATES = sizeof(NESTED_DELEGATE_PATHS) /
                                            sizeof(NESTED_DELEGATE_PATHS[0]);

// The paths for prim on each row of the drawn output
// this path is prefixed on the front of the coloumn path
static const SdfPath PRIM_ROW_PREFIX_PATHS[] =
{
  SdfPath("/"),
  SdfPath("/i"),
  SdfPath("/i/j"),
  SdfPath("/i/j/k"),
};
static const size_t NUM_PRIM_ROWS = sizeof(PRIM_ROW_PREFIX_PATHS) /
                                    sizeof(PRIM_ROW_PREFIX_PATHS[0]);


// The paths for prim on each col of the drawn output
// this path is appended to the row path
static const SdfPath PRIM_COL_SUFFIX_PATHS[] =
{
  SdfPath("p"),
  SdfPath("a"),
  SdfPath("a/p"),
  SdfPath("z/p"),
};
static const size_t NUM_PRIM_COLS = sizeof(PRIM_COL_SUFFIX_PATHS) /
                                    sizeof(PRIM_COL_SUFFIX_PATHS[0]);


static const SdfPath ROOT_PATHS[] =
{
  SdfPath("/"),
  SdfPath("/a"),
  SdfPath("/i"),
  SdfPath("/i/a"),
  SdfPath("/i/j"),
  SdfPath("/i/j/a"),
  SdfPath("/i/j/k"),
  SdfPath("/i/j/k/a"),
  SdfPath("/i/j/k/l"),
  SdfPath("/i/j/k/z"),
  SdfPath("/i/j/z"),
  SdfPath("/i/z"),
  SdfPath("/z"),
};
static const size_t NUM_ROOT_PATHS = sizeof(ROOT_PATHS) /
                                     sizeof(ROOT_PATHS[0]);


// Positioning control
static const double PRIM_SPACING = 3.0;
static const double X_OFFSET = -(static_cast<double>(NUM_PRIM_COLS) * 0.5)
                                 * PRIM_SPACING + (0.5 * PRIM_SPACING);
static const double Y_OFFSET = -(static_cast<double>(NUM_PRIM_ROWS) * 0.5)
                                 * PRIM_SPACING + (0.5 * PRIM_SPACING);

// Color control
static const float COLOR_COL_DELTA =  1.0f / (NUM_PRIM_COLS - 1);
static const float COLOR_ROW_DELTA =  1.0f / (NUM_PRIM_ROWS - 1);


class My_TestGLDrawing final : public HdSt_UnitTestGLDrawing {
public:
    My_TestGLDrawing();
    ~My_TestGLDrawing();

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void KeyRelease(int key) override;
    void Present(uint32_t framebuffer) override;

private:
    void AddTriangle(HdUnitTestDelegate *delegate,
                     SdfPath const &id,
                     GfMatrix4f const &transform,
                     GfVec3f const &color, float opacity);
    void AddPrim(size_t col, size_t row);
    void UpdateCollection();

    HdSt_TestDriverUniquePtr _driver;
    HdUnitTestDelegate *_sceneDelegates[NUM_NESTED_DELEAGATES];
    HdRprimCollection     _collection;
    HdRenderPassSharedPtr _renderPass;
    size_t                _desiredRootPathNum;
    size_t                _currentRootPathNum;
};

////////////////////////////////////////////////////////////




My_TestGLDrawing::My_TestGLDrawing()
 : HdSt_UnitTestGLDrawing()
 , _driver(nullptr)
 , _sceneDelegates()
 , _collection()
 , _renderPass()
 , _desiredRootPathNum(0)
 , _currentRootPathNum(-1)
{
    SetCameraRotate(90.0f, 0.0f);
    SetCameraTranslate(GfVec3f(0, 0, -20.0f));
}

My_TestGLDrawing::~My_TestGLDrawing()
{
    // Skip Root delegate as that is managed by test driver.
    for (size_t delegateNum = 1;
                delegateNum < NUM_NESTED_DELEAGATES;
              ++delegateNum) {
        delete _sceneDelegates[delegateNum];
    }
}

void
My_TestGLDrawing::InitTest()
{
    std::cout << "My_TestGLDrawing::InitTest()\n";

    _driver = std::make_unique<HdSt_TestDriver>(HdReprSelector(HdReprTokens->hull));

    // Create delegates
    _sceneDelegates[0] = &_driver->GetDelegate();
    HdRenderIndex *renderIndex = &_sceneDelegates[0]->GetRenderIndex();

    for (size_t delegateNum = 1;
                delegateNum < NUM_NESTED_DELEAGATES;
              ++delegateNum) {
        _sceneDelegates[delegateNum] =
                    new HdUnitTestDelegate(renderIndex,
                                            NESTED_DELEGATE_PATHS[delegateNum]);
    }

    // Now Add prims

    for (size_t row = 0; row < NUM_PRIM_ROWS; ++row) {
        for (size_t col = 0; col < NUM_PRIM_COLS; ++col) {
            AddPrim(col, row);
        }
    }

    // Create Render Pass

    _collection = HdRprimCollection(HdTokens->geometry, 
        HdReprSelector(HdReprTokens->hull));

    HdRenderDelegate *renderDelegate = renderIndex->GetRenderDelegate();
    _renderPass = renderDelegate->CreateRenderPass(renderIndex, _collection);

    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());
}

void
My_TestGLDrawing::DrawTest()
{
    UpdateCollection();

    int width = GetWidth(), height = GetHeight();
    GfMatrix4d viewMatrix = GetViewMatrix();
    GfMatrix4d projMatrix = GetProjectionMatrix();

    _driver->SetCamera(
        viewMatrix,
        projMatrix,
        CameraUtilFraming(
            GfRect2i(GfVec2i(0, 0), width, height)));
    
    _driver->UpdateAovDimensions(width, height);

    _driver->Draw(_renderPass, false);

}

void
My_TestGLDrawing::OffscreenTest()
{
    static const std::string FILENAME_PREFIX("testHdStNestedDelegate");
    static const std::string FILENAME_EXT(".png");

    for (size_t rootPathNum = 0; rootPathNum < NUM_ROOT_PATHS; ++rootPathNum) {
        _desiredRootPathNum = rootPathNum;

        DrawTest();

        std::string rootPath = ROOT_PATHS[_desiredRootPathNum].GetString();
        std::replace(rootPath.begin(), rootPath.end(), '/', '_');
        std::string filePath =  FILENAME_PREFIX + rootPath + FILENAME_EXT;


        printf("Writing File %s\n", filePath.c_str());
        _driver->WriteToFile("color", filePath);
    }
}

void
My_TestGLDrawing::KeyRelease(int key)
{
    if (key == ' ') {
        ++_desiredRootPathNum;
        _desiredRootPathNum %= NUM_ROOT_PATHS;
    }
}

void
My_TestGLDrawing::Present(uint32_t framebuffer)
{
    _driver->Present(GetWidth(), GetHeight(), framebuffer);
}

template <typename T>
static VtArray<T>
_BuildArray(T values[], int numValues)
{
    VtArray<T> result(numValues);
    std::copy(values, values+numValues, result.begin());
    return result;
}

void
My_TestGLDrawing::AddTriangle(HdUnitTestDelegate *delegate,
                              SdfPath const &id,
                              GfMatrix4f const &transform,
                              GfVec3f const &color, float opacity)
{
    static GfVec3f points[] = {
        GfVec3f(-1.0f, -1.0f, 0.0f ),
        GfVec3f( 1.0f, -1.0f, 0.0f ),
        GfVec3f( 0.0f,  1.0f, 0.0f ),
    };
    static int numVerts[] = {3};
    static int verts[] = {0, 1, 2};

    VtIntArray holes;
    PxOsdSubdivTags subdivTags;

    delegate->AddMesh(
            id,
            transform,
            _BuildArray(points, sizeof(points)/sizeof(points[0])),
            _BuildArray(numVerts, sizeof(numVerts)/sizeof(numVerts[0])),
            _BuildArray(verts, sizeof(verts)/sizeof(verts[0])),
            holes,
            subdivTags,
            VtValue(color),
            HdInterpolationConstant,
            VtValue(opacity),
            HdInterpolationConstant);
}


void
My_TestGLDrawing::AddPrim(size_t col, size_t row)
{
    SdfPath primId = PRIM_ROW_PREFIX_PATHS[row].AppendPath(
                                                    PRIM_COL_SUFFIX_PATHS[col]);

    double xPos =  X_OFFSET + col * PRIM_SPACING;
    double yPos =  Y_OFFSET + row * PRIM_SPACING;

    // Invert y, so 1st row is top of screen
    yPos = -yPos;

    GfMatrix4d dmat;
    dmat.SetTranslate(GfVec3d(xPos, yPos, 0.0));

    GfVec3f color(static_cast<float>(col) * COLOR_COL_DELTA,
                  static_cast<float>(row) * COLOR_ROW_DELTA,
                  1.0f);

    // Walk delagate list backwards to find first delegate that has
    // path that prefixes the prim path.
    size_t delegateNum = NUM_NESTED_DELEAGATES - 1;
    while ((delegateNum > 0) &&
           (!primId.HasPrefix(_sceneDelegates[delegateNum]->GetDelegateID())))
    {
        --delegateNum;
    }

    printf("Adding prim: %s @ (%f, %f) using delegate %s\n",
            primId.GetText(),
            xPos,
            yPos,
            _sceneDelegates[delegateNum]->GetDelegateID().GetText());

    AddTriangle(_sceneDelegates[delegateNum], primId, GfMatrix4f(dmat), color, 1.0f);
}

void
My_TestGLDrawing::UpdateCollection()
{
    if (_currentRootPathNum != _desiredRootPathNum)
    {
        printf("Setting Collection to %s\n", ROOT_PATHS[_desiredRootPathNum].GetText());
        _collection.SetRootPath(ROOT_PATHS[_desiredRootPathNum]);
        _renderPass->SetRprimCollection(_collection);
        _currentRootPathNum = _desiredRootPathNum;
    }
}

int main(int argc, char *argv[])
{
    TfErrorMark mark;

    My_TestGLDrawing driver;
    driver.RunTest(argc, argv);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

