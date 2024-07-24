//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/diagnostic.h"

#include <iostream>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        _reprName = HdReprTokens->hull;
        _refineLevel = 0;
        SetCameraRotate(60.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0, 0, -15.0f-1.7320508f*2.0f));
    }

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;

protected:
    void ParseArgs(int argc, char *argv[]) override;
    void AddLargeCurve(HdUnitTestDelegate *delegate);

private:
    HdSt_TestDriver* _driver;

    TfToken _reprName;
    int _refineLevel;
    std::string _outputFilePath;
};

////////////////////////////////////////////////////////////

void
My_TestGLDrawing::InitTest()
{
    _driver = new HdSt_TestDriver(_reprName);
    HdUnitTestDelegate &delegate = _driver->GetDelegate();
    delegate.SetRefineLevel(_refineLevel);
   
    AddLargeCurve(&delegate);
        
    // center camera
    SetCameraTranslate(GetCameraTranslate() + GfVec3f(0.0, 0.0, 0.0));

    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());
}

void
My_TestGLDrawing::DrawTest()
{
    int width = GetWidth(), height = GetHeight();
    GfMatrix4d viewMatrix = GetViewMatrix();
    GfMatrix4d projMatrix = GetProjectionMatrix();

    _driver->SetCamera(
        viewMatrix,
        projMatrix,
        CameraUtilFraming(
            GfRect2i(GfVec2i(0, 0), width, height)));

    _driver->UpdateAovDimensions(width, height);

    _driver->Draw();
}

void
My_TestGLDrawing::OffscreenTest()
{    
    DrawTest();

    if (!_outputFilePath.empty()) {
        _driver->WriteToFile("color", _outputFilePath);
    }
}

/* virtual */
void
My_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
    for (int i=0; i<argc; ++i) {
        if (std::string(argv[i]) == "--repr") {
            _reprName = TfToken(argv[++i]);
        } else if (std::string(argv[i]) == "--refineLevel") {
            _refineLevel = atoi(argv[++i]);
        } else if (std::string(argv[i]) == "--write" && i+1<argc) {
            _outputFilePath = argv[++i];
        }
    }
}

// Add a BasisCurve with points data that exceeds the maximum size of a BAR.
// This case implies that the BufferArray contains only one BAR. which 
// exceeds the current size limitations. The test is to ensure Hydra 
// gracefully handles this scenario, and copies as-much-as-possible, while
// issuing a warning.
void
My_TestGLDrawing::AddLargeCurve(HdUnitTestDelegate *delegate)
{
    HdInterpolation colorInterp, widthInterp, opacityInterp;
    colorInterp = widthInterp = opacityInterp = HdInterpolationConstant;
    
    const size_t vboSizeLimit = 1 << 30; // see HD_MAX_VBO_SIZE
    const size_t maxPointsInVBO = vboSizeLimit / sizeof(GfVec3f);
    const size_t numControlVertsPerCurve = 1 << 2;
    const size_t numCurves = maxPointsInVBO / numControlVertsPerCurve + 1;
    
    std::vector<int> curveVertexCounts(numCurves, numControlVertsPerCurve);
    
    GfVec3f basePoints[] = {
        GfVec3f( 1.0f, 1.0f, 1.0f ),
        GfVec3f(-1.0f, 1.0f, 1.0f ),
        GfVec3f(-1.0f,-1.0f, 1.0f ),
        GfVec3f( 1.0f,-1.0f, 1.0f ),

        GfVec3f(-1.0f,-1.0f,-1.0f ),
        GfVec3f(-1.0f, 1.0f,-1.0f ),
        GfVec3f( 1.0f, 1.0f,-1.0f ),
        GfVec3f( 1.0f,-1.0f,-1.0f ),
    };

    const size_t numVerts = numCurves * numControlVertsPerCurve;
    std::vector<GfVec3f> points;
    points.reserve(numVerts);

    GfMatrix4d transform(1.0f); // set to identity.
    GfVec3d translation(0,0,0); 
    double deltaTrans = 4;
    double rotDegrees = 0, deltaDegrees = 5;
    size_t curveId = 0;
    while (curveId < numCurves) {

        for (size_t vxId = 0; vxId < numControlVertsPerCurve; vxId++) {
            GfVec4f tmpPoint = GfVec4f(basePoints[vxId][0], 
                                       basePoints[vxId][1], 
                                       basePoints[vxId][2], 
                                       1.0f);

            GfRotation orientation(GfVec3d(1, 0, 0), rotDegrees);
            transform.SetTransform(orientation, translation);
            tmpPoint = tmpPoint * transform;
            
            points.emplace_back(GfVec3f( tmpPoint[0],
                                         tmpPoint[1],
                                         tmpPoint[2]));
        }
        curveId++;
        rotDegrees += deltaDegrees;
        if (rotDegrees > 360) {
            rotDegrees = 0;
            translation[1] += deltaTrans;
        }
    }

    VtValue color = VtValue(GfVec3f(0.4, 0.3, 0.5));
    VtValue opacity = VtValue(1.0f);
    VtValue width = VtValue(0.8f);

    VtArray<GfVec3f> vtPoints(numVerts);
    VtArray<int> vtCurveVertexCounts(numCurves);

    std::copy(points.begin(), points.end(), vtPoints.begin());
    std::copy(curveVertexCounts.begin(), curveVertexCounts.end(),
              vtCurveVertexCounts.begin());


    delegate->AddBasisCurves(SdfPath("/largeCurve"),
                            vtPoints,
                            vtCurveVertexCounts,
                            VtIntArray(),
                            VtArray<GfVec3f>(),
                            HdTokens->cubic,
                            HdTokens->bezier,
                            color, colorInterp,
                            opacity, opacityInterp,
                            width, widthInterp);

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

