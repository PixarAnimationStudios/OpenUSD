//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// This test harness is mostly a clone of testHdStBasicDrawing.cpp with fewer
// options and a custom test scene.
//

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3d.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        _reprName = HdReprTokens->hull;
        _cullStyle = HdCullStyleNothing;

        SetCameraRotate(60.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0, 0, -20.0f-1.7320508f*2.0f));
    }

    // HdSt_UnitTestGLDrawing overrides
    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

protected:
    void ParseArgs(int argc, char *argv[]) override;

private:
    GfVec3f _PopulateCullingTestSet(HdUnitTestDelegate * const delegate);

    HdSt_TestDriverUniquePtr _driver;
    HdSt_TestLightingShaderSharedPtr _lightingShader;
    std::vector<GfVec4d> _clipPlanes;

    TfToken _reprName;
    HdCullStyle _cullStyle;
    std::string _outputFilePath;
};

////////////////////////////////////////////////////////////

GfVec3f
My_TestGLDrawing::_PopulateCullingTestSet(
    HdUnitTestDelegate * const delegate)
{

    /* The test set below consists of grids that exercise the matrix of
     opinions that affect culling:
     - single/double sidedness : {SS, DS}
     - orientation of the topology (handedness) : {LH, RH}
     - regular/mirrored xform : {RT, MT}
     - prim cullstyle: {Nothing, DontCare, Back, Front, BackUnlessDS, FrontUnlessDS}

       The generated grid has 8 columns with the following opinions constant
       per-column (read columnwise):
       
       SS  SS  SS  SS  DS  DS  DS  DS
       RH  RH  LH  LH  RH  RH  LH  LH
       RT  MT  RT  MT  RT  MT  RT  MT
        
       The prim cullstyle opinions are exercised bottom-to-top.

       Our expectation is that:
       The bottom row (Nothing) should never be culled in any of the baselines.

       The row above it (DontCare) is influenced by the render pass cullstyle
       opinion and should differ for each baseline.

       The combinations of RH x {RT, MT} should not be culled when the prim's
       cullstyle opinion is Back. The flipside (LH x {RT, MT}) would be culled.
       The combinations of LH x {RT, MT} should not be culled when the prim's
       cullstyle opinion is Front. The flipside (RH x {RT, MT}) would be culled.

       A double sided prim with the cullstyle opinion *UnlessDS shouldn't be
       culled (i.e., the 2x4 set of prims on the top-right shouldn't ever be
       culled).
    */

    enum class Sidedness {
        SingleSided,
        DoubleSided
    };

    enum class Orientation {
        LeftHanded,
        RightHanded
    };

    enum class Transform {
        Regular,
        Mirrored
    };

    GfMatrix4d dmat(1.0);
    double xPos = 0.0;
    size_t i = 0;

    for (Sidedness s : {Sidedness::SingleSided, Sidedness::DoubleSided}) {

        const bool doubleSided = (s == Sidedness::DoubleSided);

        for (Orientation o : {Orientation::RightHanded,
                              Orientation::LeftHanded}) {
            
            const bool rightHanded = (o == Orientation::RightHanded);

            for (Transform t : {Transform::Regular, Transform::Mirrored}) {

                double yPos = -3.0;

                // Generate a column of grids that exercises all the authored
                // cullstyle opinions.
                for (HdCullStyle c : {HdCullStyleNothing, HdCullStyleDontCare,
                                      HdCullStyleBack, HdCullStyleFront,
                                      HdCullStyleBackUnlessDoubleSided,
                                      HdCullStyleFrontUnlessDoubleSided}) {

                    const std::string id("/grid" + std::to_string(i++));

                    if (t == Transform::Mirrored) {
                        dmat.SetScale(GfVec3d(-1, 1, 1));
                    } else {
                        dmat.SetScale(GfVec3d(1));
                    }
                    dmat.SetTranslateOnly(GfVec3d(xPos,  yPos, 0.0));

                    delegate->AddGridWithFaceColor(
                        SdfPath(id),
                        /* nx */ 3,
                        /* ny */ 3,
                        GfMatrix4f(dmat),
                        rightHanded,
                        doubleSided);
                    
                    delegate->SetMeshCullStyle(SdfPath(id), c);

                    yPos += 3.0;
                }

                xPos += 3.0;
            }
        }
    }

   return GfVec3f(xPos/2.0, 0, 5);
}


void
My_TestGLDrawing::InitTest()
{
    std::cout << "My_TestGLDrawing::InitTest() " << _reprName << "\n";

    _driver = std::make_unique<HdSt_TestDriver>(_reprName);
    HdUnitTestDelegate &delegate = _driver->GetDelegate();

    GfVec3f center(0);

    center = _PopulateCullingTestSet(&delegate);

    // center camera
    SetCameraTranslate(GetCameraTranslate() - center);

    _driver->SetCameraClipPlanes(_clipPlanes);

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

    _driver->SetCullStyle(_cullStyle);

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
        std::string arg(argv[i]);
        if (arg == "--repr") {
            _reprName = TfToken(argv[++i]);
        } else if (std::string(argv[i]) == "--cullStyle") {
            std::string style = argv[++i];
            if (style == "Nothing") {
                _cullStyle = HdCullStyleNothing;
            } else if (style == "Back") {
                _cullStyle = HdCullStyleBack;
            } else if (style == "Front") {
                _cullStyle = HdCullStyleFront;
            } else if (style == "BackUnlessDoubleSided") {
                _cullStyle = HdCullStyleBackUnlessDoubleSided;
            } else if (style == "FrontUnlessDoubleSided") {
                _cullStyle = HdCullStyleFrontUnlessDoubleSided;
            } else {
                std::cerr << "Error: Unknown cullstyle = " << style << "\n";
                exit(EXIT_FAILURE);
            }
        }  else if (arg == "--write") {
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

