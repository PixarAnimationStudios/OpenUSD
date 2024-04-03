//
// Copyright 2021 Pixar
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

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/errorMark.h"

#include <algorithm>
#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

template <typename T>
static VtArray<T>
_BuildArray(T values[], int numValues)
{
    VtArray<T> result(numValues);
    std::copy(values, values+numValues, result.begin());
    return result;
}

template <typename T, size_t N>
static void
_CopyArray(T (*dst)[N], T const (&src)[N])
{
    std::copy(src, src+N, *dst);
}

static void
_TransformPoints(GfVec3f *pointsOut, GfVec3f *pointsIn, size_t numPoints, 
                 GfMatrix4d mat) {
    for (size_t i = 0; i < numPoints; ++ i) {
        GfVec4f point = GfVec4f(pointsIn[i][0], pointsIn[i][1],
                                pointsIn[i][2], 1.0f);
        point = point *  mat;
        pointsOut[i] = GfVec3f(point[0], point[1], point[2]);
    }
}

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
    void Present(uint32_t framebuffer) override;

protected:
    void ParseArgs(int argc, char *argv[]) override;

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

    GfMatrix4d dmat;

    double xPos = 0.0;
    double yPos = 0.0;
    double zPos = 6.0;
    double dy = -1.75;

    GfVec3f points3[3] = { GfVec3f(-1,0,0), 
                           GfVec3f(0,0,0), 
                           GfVec3f(1,0,0) };
    GfVec3f points3Padded[6] = { GfVec3f(-1,1,0), // extra
                                 GfVec3f(-1,0,0), 
                                 GfVec3f(0,0,0),
                                 GfVec3f(0,1,0), // extra
                                 GfVec3f(1,0,0),
                                 GfVec3f(1,1,0) }; // extra
    int indices3[3] = {1,2,4};
    VtIntArray indices3Vt = _BuildArray(indices3, 3);

    GfVec3f points5[5] = { GfVec3f(-2,0,0),
                           GfVec3f(-1,0,0),
                           GfVec3f(0,0,0), 
                           GfVec3f(1,0,0),
                           GfVec3f(2,0,0) };
    GfVec3f points5Padded[8] = { GfVec3f(-2,1,0), // extra
                                 GfVec3f(-2,0,0),
                                 GfVec3f(-1,0,0),
                                 GfVec3f(0,0,0),
                                 GfVec3f(0,1,0), // extra
                                 GfVec3f(1,0,0),
                                 GfVec3f(2,0,0),
                                 GfVec3f(2,1,0) }; // extra
    int indices5[5] = {1,2,3,5,6};
    VtIntArray indices5Vt = _BuildArray(indices5, 5);

    GfVec3f points7[7] = { GfVec3f(-1,0,0),
                           GfVec3f(-.666,0,0), 
                           GfVec3f(-.333,0,0),
                           GfVec3f(0,0,0), 
                           GfVec3f(.333,0,0),
                           GfVec3f(.666,0,0), 
                           GfVec3f(1,0,0) };
    GfVec3f points7Padded[10] = { GfVec3f(-1,1,0), // extra
                                  GfVec3f(-1,0,0),
                                  GfVec3f(-.666,0,0), 
                                  GfVec3f(-.333,0,0),
                                  GfVec3f(0,0,0), 
                                  GfVec3f(0,1,0), // extra
                                  GfVec3f(.333,0,0),
                                  GfVec3f(.666,0,0), 
                                  GfVec3f(1,0,0),
                                  GfVec3f(1,1,0) }; // extra
    int indices7[7] = {1,2,3,4,6,7,8};
    VtIntArray indices7Vt = _BuildArray(indices7, 7);
    
    float widths3[3] = { 0.1, 0.2, 0.3 };
    float widths3Padded[6] = { 1.0, 0.1, 0.2, 1.0, 0.3, 1.0 };
    float widths5[5] = { 0, 0.1, 0.2, 0.3, 0.4 };
    float widths5Padded[8] = { 1.0, 0, 0.1, 0.2, 1.0, 0.3, 0.4, 1.0 };
    float widths7[7] = { 0.1, 0.1333, 0.1666, 0.2, 0.2333, 0.2666, 0.3 };
    float widths7Padded[10] = { 1.0, 0.1, 0.1333, 0.1666, 0.2, 1.0, 0.2333,
        0.2666, 0.3, 1.0 };
    VtValue widths3Vt = VtValue(_BuildArray(widths3, 3));
    VtValue widths3PaddedVt = VtValue(_BuildArray(widths3Padded, 6));
    VtValue widths5Vt = VtValue(_BuildArray(widths5, 5));
    VtValue widths5PaddedVt = VtValue(_BuildArray(widths5Padded, 8));
    VtValue widths7Vt = VtValue(_BuildArray(widths7, 7));
    VtValue widths7PaddedVt = VtValue(_BuildArray(widths7Padded, 10));

    GfVec3f displayColor3[3] = { GfVec3f(1,0,0), GfVec3f(0,1,0), 
                                 GfVec3f(0,0,1) };
    GfVec3f displayColor3Padded[6] = { GfVec3f(1,1,1), GfVec3f(1,0,0),
                                       GfVec3f(0,1,0), GfVec3f(1,1,1),
                                       GfVec3f(0,0,1), GfVec3f(1,1,1)};
    GfVec3f displayColor5[5] = { GfVec3f(2,-1,0), GfVec3f(1,0,0), 
                                 GfVec3f(0,1,0), GfVec3f(0,0,1), 
                                 GfVec3f(0,-1,2) };
    GfVec3f displayColor5Padded[8] = { GfVec3f(1,1,1), GfVec3f(2,-1,0),
                                       GfVec3f(1,0,0), GfVec3f(0,1,0),
                                       GfVec3f(1,1,1), GfVec3f(0,0,1),
                                       GfVec3f(0,-1,2), GfVec3f(1,1,1) };
    GfVec3f displayColor7[7] = { GfVec3f(1,0,0), GfVec3f(.666,.333,0), 
                                 GfVec3f(.333,.666,0), GfVec3f(0,1,0), 
                                 GfVec3f(0,.666,.333), GfVec3f(0, .333, .666), 
                                 GfVec3f(0,0,1) };
    GfVec3f displayColor7Padded[10] = { GfVec3f(1,1,1), GfVec3f(1,0,0),
                                 GfVec3f(.666,.333,0),  GfVec3f(.333,.666,0),
                                 GfVec3f(0,1,0), GfVec3f(1,1,1),
                                 GfVec3f(0,.666,.333), GfVec3f(0, .333, .666), 
                                 GfVec3f(0,0,1), GfVec3f(1,1,1) };

    VtValue displayColor3Vt = VtValue(_BuildArray(displayColor3, 3));
    VtValue displayColor3PaddedVt =
        VtValue(_BuildArray(displayColor3Padded, 6));
    VtValue displayColor5Vt = VtValue(_BuildArray(displayColor5, 5));
    VtValue displayColor5PaddedVt =
        VtValue(_BuildArray(displayColor5Padded, 8));
    VtValue displayColor7Vt = VtValue(_BuildArray(displayColor7, 7));
    VtValue displayColor7PaddedVt =
        VtValue(_BuildArray(displayColor7Padded, 10));

    float displayOpacity3[3] = { 0.5, 0.75, 1.0 };
    VtValue displayOpacity3Vt = VtValue(_BuildArray(displayOpacity3, 3));


    VtVec3fArray normals = VtVec3fArray();

    // First column: linear curves
    {
        GfVec3f points3Copy[3];
        _CopyArray(&points3Copy, points3);

        int curveVertexCounts3[1] = {3};
        VtIntArray curveVertexCounts3Vt = _BuildArray(curveVertexCounts3, 1);
        
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points3Copy, points3, 3, dmat);
        VtVec3fArray points3Vt = _BuildArray(points3Copy, 3);
        delegate.AddBasisCurves(SdfPath("/curve1l"), points3Vt, 
            curveVertexCounts3Vt, VtIntArray(), normals, HdTokens->linear,
            TfToken(), VtValue(GfVec3f(1.0, 0.0, 0.0)), HdInterpolationConstant,
            VtValue(1.0f), HdInterpolationConstant, VtValue(0.2f), 
            HdInterpolationConstant);
        yPos += dy;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points3Copy, points3, 3, dmat);
        points3Vt = _BuildArray(points3Copy, 3);
        delegate.AddBasisCurves(SdfPath("/curve2l"), points3Vt, 
            curveVertexCounts3Vt, VtIntArray(), normals, HdTokens->linear,
            TfToken(), VtValue(GfVec3f(0.0, 0.0, 1.0)), HdInterpolationConstant,
            displayOpacity3Vt, HdInterpolationVarying, VtValue(0.2f), 
            HdInterpolationConstant);
        yPos += dy;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points3Copy, points3, 3, dmat);
        points3Vt = _BuildArray(points3Copy, 3);
        delegate.AddBasisCurves(SdfPath("/curve3l"), points3Vt, 
            curveVertexCounts3Vt, VtIntArray(), normals, HdTokens->linear,
            TfToken(), displayColor3Vt, HdInterpolationVertex, VtValue(1.0f), 
            HdInterpolationConstant, widths3Vt, HdInterpolationVertex);
        yPos += dy;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points3Copy, points3, 3, dmat);
        points3Vt = _BuildArray(points3Copy, 3);
        delegate.AddBasisCurves(SdfPath("/curve4l"), points3Vt, 
            curveVertexCounts3Vt, VtIntArray(), normals, HdTokens->linear,
            TfToken(), displayColor3Vt, HdInterpolationVarying, VtValue(1.0f), 
            HdInterpolationConstant, widths3Vt, HdInterpolationVarying);
        yPos += dy;

        // padded with 3 unused extra entries for points vertex primvar,
        // with indices avoiding the extra entries, which is valid.
        GfVec3f points3PaddedCopy[6];
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points3PaddedCopy, points3Padded, 6, dmat);
        points3Vt = _BuildArray(points3PaddedCopy, 6);
        delegate.AddBasisCurves(SdfPath("/curve5l"), points3Vt, 
            curveVertexCounts3Vt, indices3Vt, normals, HdTokens->linear,
            TfToken(), displayColor3PaddedVt, HdInterpolationVertex, VtValue(1.0f), 
            HdInterpolationConstant, widths3PaddedVt, HdInterpolationVertex);
        yPos += dy;

        // padded with 3 unused extra entries for points vertex primvar.
        // This is an invalid case and results in no visible curves.
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points3PaddedCopy, points3Padded, 6, dmat);
        points3Vt = _BuildArray(points3PaddedCopy, 6);
        delegate.AddBasisCurves(SdfPath("/curve6l"), points3Vt, 
            curveVertexCounts3Vt, VtIntArray(), normals, HdTokens->linear,
            TfToken(), displayColor3Vt, HdInterpolationVarying, VtValue(1.0f), 
            HdInterpolationConstant, widths3Vt, HdInterpolationVarying);
        yPos += dy;

        // padded with 3 extra entries for widths varying primvar.
        // This is an invalid case and results in fallback width 1 being 
        // used instead.
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points3Copy, points3, 3, dmat);
        points3Vt = _BuildArray(points3Copy, 3);
        delegate.AddBasisCurves(SdfPath("/curve7l"), points3Vt, 
            curveVertexCounts3Vt, VtIntArray(), normals, HdTokens->linear,
            TfToken(), displayColor3Vt, HdInterpolationVarying, VtValue(1.0f), 
            HdInterpolationConstant, widths3PaddedVt, HdInterpolationVarying);
        yPos += dy;
    }

    xPos = 3.0;
    yPos = 0.0;

    // Second column: bezier curves
    {   
        GfVec3f points7Copy[7];
        _CopyArray(&points7Copy, points7);

        int curveVertexCounts7[1] = {7};
        VtIntArray curveVertexCounts7Vt = _BuildArray(curveVertexCounts7, 1);

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points7Copy, points7, 7, dmat);
        VtVec3fArray points7Vt = _BuildArray(points7Copy, 7);
        delegate.AddBasisCurves(SdfPath("/curve1b"), points7Vt, 
            curveVertexCounts7Vt, VtIntArray(), normals, HdTokens->cubic,
            HdTokens->bezier, VtValue(GfVec3f(1.0, 0.0, 0.0)),
            HdInterpolationConstant, VtValue(1.0f), HdInterpolationConstant,
            VtValue(0.2f), HdInterpolationConstant);
        yPos += dy;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points7Copy, points7, 7, dmat);
        points7Vt = _BuildArray(points7Copy, 7);
        delegate.AddBasisCurves(SdfPath("/curve2b"), points7Vt, 
            curveVertexCounts7Vt, VtIntArray(), normals, HdTokens->cubic,
            HdTokens->bezier, VtValue(GfVec3f(0.0, 0.0, 1.0)),
            HdInterpolationConstant, displayOpacity3Vt, HdInterpolationVarying,
            VtValue(0.2f), HdInterpolationConstant);
        yPos += dy;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points7Copy, points7, 7, dmat);
        points7Vt = _BuildArray(points7Copy, 7);
        delegate.AddBasisCurves(SdfPath("/curve3b"), points7Vt, 
            curveVertexCounts7Vt, VtIntArray(), normals, HdTokens->cubic, 
            HdTokens->bezier, displayColor7Vt, HdInterpolationVertex,
            VtValue(1.0f), HdInterpolationConstant, widths7Vt,
            HdInterpolationVertex);
        yPos += dy;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points7Copy, points7, 7, dmat);
        points7Vt = _BuildArray(points7Copy, 7);
        delegate.AddBasisCurves(SdfPath("/curve4b"), points7Vt, 
            curveVertexCounts7Vt, VtIntArray(),normals, HdTokens->cubic, 
            HdTokens->bezier,  displayColor3Vt, HdInterpolationVarying,
            VtValue(1.0f),  HdInterpolationConstant, widths3Vt,
            HdInterpolationVarying);
        yPos += dy;

        // xxx we are drawing the right points, yay, but the colors are
        // entries 1, 1, 1, 1, 2200 maybe??

        // padded with 3 unused extra entries for points vertex primvar,
        // with indices avoiding the extra entries, which is valid.
        GfVec3f points7PaddedCopy[10];
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points7PaddedCopy, points7Padded, 10, dmat);
        points7Vt = _BuildArray(points7PaddedCopy, 10);
        delegate.AddBasisCurves(SdfPath("/curve5b"), points7Vt, 
            curveVertexCounts7Vt, indices7Vt, normals, HdTokens->cubic, 
            HdTokens->bezier,  displayColor7PaddedVt, HdInterpolationVertex,
            VtValue(1.0f),  HdInterpolationConstant, widths7PaddedVt,
            HdInterpolationVertex);
        yPos += dy;

        // padded with 3 unused extra entries for points vertex primvar.
        // This is an invalid case and results in no visible curves.
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points7PaddedCopy, points7Padded, 10, dmat);
        points7Vt = _BuildArray(points7PaddedCopy, 10);
        delegate.AddBasisCurves(SdfPath("/curve6b"), points7Vt, 
            curveVertexCounts7Vt, VtIntArray(), normals, HdTokens->cubic, 
            HdTokens->bezier,  displayColor3Vt, HdInterpolationVarying,
            VtValue(1.0f),  HdInterpolationConstant, widths3Vt,
            HdInterpolationVarying);
        yPos += dy;

        // padded with 3 extra entries for widths varying primvar.
        // This is an invalid case and results in fallback width 1 being 
        // used instead.
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points7Copy, points7, 7, dmat);
        points7Vt = _BuildArray(points7Copy, 7);
        delegate.AddBasisCurves(SdfPath("/curve7b"), points7Vt, 
            curveVertexCounts7Vt, VtIntArray(), normals, HdTokens->cubic, 
            HdTokens->bezier,  displayColor3Vt, HdInterpolationVarying,
            VtValue(1.0f),  HdInterpolationConstant, widths3PaddedVt,
            HdInterpolationVarying);
        yPos += dy;
    }

    xPos = 7.0;
    yPos = 0.0;

    // // Third column: b-spline curves
    {   
        GfVec3f points5Copy[5];
        _CopyArray(&points5Copy, points5);

        int curveVertexCounts5[1] = {5};
        VtIntArray curveVertexCounts5Vt = _BuildArray(curveVertexCounts5, 1);

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points5Copy, points5, 5, dmat);
        VtVec3fArray points5Vt = _BuildArray(points5Copy, 5);
        delegate.AddBasisCurves(SdfPath("/curve1bs"), points5Vt, 
            curveVertexCounts5Vt, VtIntArray(), normals, HdTokens->cubic, 
            HdTokens->bspline,  VtValue(GfVec3f(1.0, 0.0, 0.0)),
            HdInterpolationConstant, VtValue(1.0f), HdInterpolationConstant,
            VtValue(0.2f),  HdInterpolationConstant);
        yPos += dy;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points5Copy, points5, 5, dmat);
        points5Vt = _BuildArray(points5Copy, 5);
        delegate.AddBasisCurves(SdfPath("/curve2bs"), points5Vt, 
            curveVertexCounts5Vt, VtIntArray(), normals, HdTokens->cubic, 
            HdTokens->bspline,  VtValue(GfVec3f(0.0, 0.0, 1.0)),
            HdInterpolationConstant, displayOpacity3Vt, HdInterpolationVarying,
            VtValue(0.2f), HdInterpolationConstant);
        yPos += dy;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points5Copy, points5, 5, dmat);
        points5Vt = _BuildArray(points5Copy, 5);
        delegate.AddBasisCurves(SdfPath("/curve3bs"), points5Vt, 
            curveVertexCounts5Vt, VtIntArray(), normals, HdTokens->cubic, 
            HdTokens->bspline,  displayColor5Vt, HdInterpolationVertex,
            VtValue(1.0f),  HdInterpolationConstant, widths5Vt,
            HdInterpolationVertex);
        yPos += dy;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points5Copy, points5, 5, dmat);
        points5Vt = _BuildArray(points5Copy, 5);
        delegate.AddBasisCurves(SdfPath("/curve4bs"), points5Vt, 
            curveVertexCounts5Vt, VtIntArray(), normals, HdTokens->cubic, 
            HdTokens->bspline,  displayColor3Vt, HdInterpolationVarying, 
            VtValue(1.0f), HdInterpolationConstant,  widths3Vt,
            HdInterpolationVarying);
        yPos += dy;

        // padded with 3 unused extra entries for points vertex primvar,
        // with indices avoiding the extra entries, which is valid.
        GfVec3f points5PaddedCopy[8];
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points5PaddedCopy, points5Padded, 8, dmat);
        points5Vt = _BuildArray(points5PaddedCopy, 8);
        delegate.AddBasisCurves(SdfPath("/curve5bs"), points5Vt, 
            curveVertexCounts5Vt, indices5Vt,normals, HdTokens->cubic, 
            HdTokens->bspline,  displayColor5PaddedVt, HdInterpolationVertex, 
            VtValue(1.0f), HdInterpolationConstant,  widths5PaddedVt,
            HdInterpolationVertex);
        yPos += dy;

        // padded with 3 unused extra entries for points vertex primvar.
        // This is an invalid case and results in no visible curves.
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points5PaddedCopy, points5Padded, 8, dmat);
        points5Vt = _BuildArray(points5PaddedCopy, 8);
        delegate.AddBasisCurves(SdfPath("/curve6bs"), points5Vt, 
            curveVertexCounts5Vt, VtIntArray(),normals, HdTokens->cubic, 
            HdTokens->bspline,  displayColor3Vt, HdInterpolationVarying, 
            VtValue(1.0f), HdInterpolationConstant,  widths3Vt,
            HdInterpolationVarying);
        yPos += dy;

        // padded with 3 extra entries for widths varying primvar.
        // This is an invalid case and results in fallback width 1 being 
        // used instead.
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points5Copy, points5, 5, dmat);
        points5Vt = _BuildArray(points5Copy, 5);
        delegate.AddBasisCurves(SdfPath("/curve7bs"), points5Vt, 
            curveVertexCounts5Vt, VtIntArray(), normals, HdTokens->cubic, 
            HdTokens->bspline,  displayColor3Vt, HdInterpolationVarying, 
            VtValue(1.0f), HdInterpolationConstant,  widths3PaddedVt,
            HdInterpolationVarying);
        yPos += dy;
    }

    xPos = 11.0;
    yPos = 0.0;

    // // Fourth column: catmull-rom curves
    {   
        GfVec3f points5Copy[5];
        _CopyArray(&points5Copy, points5);

        int curveVertexCounts5[1] = {5};
        VtIntArray curveVertexCounts5Vt = _BuildArray(curveVertexCounts5, 1);

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points5Copy, points5, 5, dmat);
        VtVec3fArray points5Vt = _BuildArray(points5Copy, 5);
        delegate.AddBasisCurves(SdfPath("/curve1cr"), points5Vt, 
            curveVertexCounts5Vt, VtIntArray(), normals, HdTokens->cubic,  
            HdTokens->catmullRom, VtValue(GfVec3f(1.0, 0.0, 0.0)), 
            HdInterpolationConstant, VtValue(1.0f), HdInterpolationConstant, 
            VtValue(0.2f), HdInterpolationConstant);
        yPos += dy;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points5Copy, points5, 5, dmat);
        points5Vt = _BuildArray(points5Copy, 5);
        delegate.AddBasisCurves(SdfPath("/curve2cr"), points5Vt, 
            curveVertexCounts5Vt, VtIntArray(), normals, HdTokens->cubic,  
            HdTokens->catmullRom, VtValue(GfVec3f(0.0, 0.0, 1.0)), 
            HdInterpolationConstant, displayOpacity3Vt, HdInterpolationVarying, 
            VtValue(0.2f), HdInterpolationConstant);
        yPos += dy;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points5Copy, points5, 5, dmat);
        points5Vt = _BuildArray(points5Copy, 5);
        delegate.AddBasisCurves(SdfPath("/curve3cr"), points5Vt, 
            curveVertexCounts5Vt, VtIntArray(), normals, HdTokens->cubic, 
            HdTokens->catmullRom, displayColor5Vt, HdInterpolationVertex,
            VtValue(1.0f), HdInterpolationConstant, widths5Vt, 
            HdInterpolationVertex);
        yPos += dy;

        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points5Copy, points5, 5, dmat);
        points5Vt = _BuildArray(points5Copy, 5);
        delegate.AddBasisCurves(SdfPath("/curve4cr"), points5Vt, 
            curveVertexCounts5Vt, VtIntArray(), normals, HdTokens->cubic,  
            HdTokens->catmullRom, displayColor3Vt, HdInterpolationVarying,
            VtValue(1.0f), HdInterpolationConstant, widths3Vt, 
            HdInterpolationVarying);
        yPos += dy;

        // padded with 3 unused extra entries for points vertex primvar,
        // with indices avoiding the extra entries, which is valid.
        GfVec3f points5PaddedCopy[8];
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points5PaddedCopy, points5Padded, 8, dmat);
        points5Vt = _BuildArray(points5PaddedCopy, 8);
        delegate.AddBasisCurves(SdfPath("/curve5cr"), points5Vt, 
            curveVertexCounts5Vt, indices5Vt, normals, HdTokens->cubic,  
            HdTokens->catmullRom, displayColor5PaddedVt, HdInterpolationVertex,
            VtValue(1.0f), HdInterpolationConstant, widths5PaddedVt, 
            HdInterpolationVertex);
        yPos += dy;

        // padded with 3 unused extra entries for points vertex primvar.
        // This is an invalid case and results in no visible curves.
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points5PaddedCopy, points5Padded, 8, dmat);
        points5Vt = _BuildArray(points5PaddedCopy, 8);
        delegate.AddBasisCurves(SdfPath("/curve6cr"), points5Vt, 
            curveVertexCounts5Vt, VtIntArray(), normals, HdTokens->cubic,  
            HdTokens->catmullRom, displayColor3Vt, HdInterpolationVarying,
            VtValue(1.0f), HdInterpolationConstant, widths3Vt, 
            HdInterpolationVarying);
        yPos += dy;

        // padded with 3 extra entries for widths varying primvar.
        // This is an invalid case and results in fallback width 1 being 
        // used instead.
        dmat.SetTranslate(GfVec3d(xPos, yPos, zPos));
        _TransformPoints(points5Copy, points5, 5, dmat);
        points5Vt = _BuildArray(points5Copy, 5);
        delegate.AddBasisCurves(SdfPath("/curve7cr"), points5Vt, 
            curveVertexCounts5Vt, VtIntArray(), normals, HdTokens->cubic,  
            HdTokens->catmullRom, displayColor3Vt, HdInterpolationVarying,
            VtValue(1.0f), HdInterpolationConstant, widths3PaddedVt, 
            HdInterpolationVarying);
        yPos += dy;


    }

    // center camera
    SetCameraTranslate(GetCameraTranslate() + GfVec3f(-xPos/2, 2.0, -5.0));

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

void
My_TestGLDrawing::Present(uint32_t framebuffer)
{
    _driver->Present(GetWidth(), GetHeight(), framebuffer);
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

