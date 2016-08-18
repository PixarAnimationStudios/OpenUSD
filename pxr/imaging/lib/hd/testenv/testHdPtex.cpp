#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/unitTestGLDrawing.h"
#include "pxr/imaging/hd/unitTestHelper.h"

#include "pxr/imaging/glf/ptexTexture.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <mutex>

#include <Ptexture.h>
#include <PtexUtils.h>

static
std::string
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

class My_TestGLDrawing : public Hd_UnitTestGLDrawing {
public:
    My_TestGLDrawing() {
        // this rotation is to make non-quad faces of the sphere asset
        // visible. we should generalize it (to commandline args) later.
        SetCameraRotate(90.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0));

        _reprName = HdTokens->hull;
        _refineLevel = 0;
        _cullStyle = HdCullStyleNothing;
    }

    // Hd_UnitTestGLDrawing overrides
    virtual void InitTest();
    virtual void DrawTest();
    virtual void OffscreenTest();

protected:
    virtual void ParseArgs(int argc, char *argv[]);

private:
    Hd_TestDriver* _driver;

    TfToken _reprName;
    int _refineLevel;
    HdCullStyle _cullStyle;

    std::string _textureFilePath;
    std::string _outputFilePath;
};

////////////////////////////////////////////////////////////

GLuint vao;

void
My_TestGLDrawing::InitTest()
{
    _driver = new Hd_TestDriver(_reprName);
    Hd_UnitTestDelegate &delegate = _driver->GetDelegate();
    delegate.SetRefineLevel(_refineLevel);

    SdfPath shader("/shader");
    std::string shaderSource(
        "vec4 surfaceShader(vec4 Peye, vec3 Neye, vec4 color, vec4 patchCoord) {\n"
        "    color.rgb = HdGet_ptexColor().xyz;\n"
        "    return color;\n"
        "}\n"
    );
    HdShaderParamVector shaderParams;
    shaderParams.push_back(HdShaderParam(TfToken("ptexColor"), VtValue(GfVec3f(1,0,0)),
                                         SdfPath("/tex0"),
                                         TfTokenVector(),
                                         /*isPtex=*/true));
    delegate.AddSurfaceShader(shader, shaderSource, shaderParams);

    std::string ptexfile = _FindDataFile(_textureFilePath);

    GlfPtexTextureRefPtr ptex = GlfPtexTexture::New(TfToken(ptexfile));
    ptex->SetMemoryRequested(10000000);

    delegate.AddTexture(SdfPath("/tex0"), ptex);

    delegate.BindSurfaceShader(SdfPath("/mesh"), SdfPath("/shader"));

    // read a mesh from ptex's metadata
    GfRange3f range;
    {
        Ptex::String ptexError;
        PtexTexture *ptx = PtexTexture::open(ptexfile.c_str(),
                                             ptexError, true);
        if (ptx == NULL) {
            printf("Error in reading ptex\n");
            exit(1);
        }
        PtexMetaData* meta = ptx->getMetaData();

        const float * vp;
        const int *vi, *vc;
        int nvp = 0, nvi = 0, nvc = 0;
        meta->getValue("PtexVertPositions", vp, nvp);
        meta->getValue("PtexFaceVertCounts", vc, nvc);
        meta->getValue("PtexFaceVertIndices", vi, nvi);
        if (nvc == 0 || nvp == 0 || nvi == 0) {
            exit(1);
        }

        GfMatrix4f transform(1);
        VtArray<GfVec3f> points(nvp/3);
        VtArray<int> numVerts(nvc);
        VtArray<int> verts(nvi);
        std::copy((GfVec3f*)vp, (GfVec3f*)(vp+nvp), points.begin());
        std::copy(vc, vc+nvc, numVerts.begin());
        std::copy(vi, vi+nvi, verts.begin());

        for (int i = 0; i < points.size(); ++i) {
            range.UnionWith(points[i]);
        }

        delegate.AddMesh(
            SdfPath("/mesh"),
            transform,
            points,
            numVerts,
            verts,
            false,
            SdfPath(),
            PxOsdOpenSubdivTokens->catmark,
            HdTokens->rightHanded,
            false);

        ptx->release();
    }
    // frame the object
    GfVec3f center = (range.GetMin() + range.GetMax()) * 0.5f;
    center[2] += range.GetSize().GetLength();
    SetCameraTranslate(-center);

    // XXX: Setup a VAO, the current drawing engine will not yet do this.
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindVertexArray(0);
}

void
My_TestGLDrawing::DrawTest()
{
    GLfloat clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, clearColor);

    GLfloat clearDepth[1] = { 1.0f };
    glClearBufferfv(GL_DEPTH, 0, clearDepth);

    int width = GetWidth(), height = GetHeight();
    GfMatrix4d viewMatrix = GetViewMatrix();
    GfMatrix4d projMatrix = GetProjectionMatrix();

    // cull style
    _driver->SetCullStyle(_cullStyle);

    // camera
    _driver->SetCamera(viewMatrix, projMatrix, GfVec4d(0, 0, width, height));

    glViewport(0, 0, width, height);

    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(vao);

    _driver->Draw();

    glBindVertexArray(0);
}

void
My_TestGLDrawing::OffscreenTest()
{
    DrawTest();

    if (not _outputFilePath.empty()) {
        WriteToFile("color", _outputFilePath);
    }
}

/* virtual */
void
My_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
    for (int i=0; i<argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--repr") {
            _reprName = TfToken(argv[++i]);
        } else if (arg == "--refineLevel") {
            _refineLevel = atoi(argv[++i]);
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
        } else if (arg == "--ptex") {
            _textureFilePath = argv[++i];
        } else if (std::string(argv[i]) == "--write") {
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

