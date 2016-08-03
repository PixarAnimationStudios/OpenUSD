#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/hd/basisCurvesShaderKey.h"
#include "pxr/imaging/hd/codeGen.h"
#include "pxr/imaging/hd/defaultLightingShader.h"
#include "pxr/imaging/hd/drawingCoord.h"
#include "pxr/imaging/hd/drawItem.h"
#include "pxr/imaging/hd/geometricShader.h"
#include "pxr/imaging/hd/glslfxShader.h"
#include "pxr/imaging/hd/meshShaderKey.h"
#include "pxr/imaging/hd/package.h"
#include "pxr/imaging/hd/pointsShaderKey.h"
#include "pxr/imaging/hd/renderPassShader.h"
#include "pxr/imaging/hd/resourceBinder.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/rprimSharedData.h"
#include "pxr/imaging/hd/surfaceShader.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/glslfx.h"
#include "pxr/imaging/glf/testGLContext.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/staticTokens.h"

#include <iostream>

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (color)
    (faceVisibility)
    (normals)
    (points)
    (primID)
    (translate)
    (transform)
    (transformInverse)
    (widths)
    ((_float, "float"))
    (vec2)
    (vec3)
    (vec4)
    (mat4)
    ((_double, "double"))
    (dvec2)
    (dvec3)
    (dvec4)
    (dmat4)
);

template <typename KEY>
static bool
CodeGenTest(KEY const &key, bool useIndirect, bool useBindlessBuffer, bool instance)
{
    TfErrorMark mark;

    // create drawItem
    HdRprimSharedData sharedData(HdDrawingCoord::DefaultNumSlots);
    HdDrawItem drawItem(&sharedData);
    HdResourceRegistry *registry = &HdResourceRegistry::GetInstance();

    HdDrawingCoord *drawingCoord = drawItem.GetDrawingCoord();

    // constant primvars
    {
        HdBufferSpecVector bufferSpecs;
        bufferSpecs.push_back(HdBufferSpec(
                                  _tokens->transform,
                                  GL_FLOAT,
                                  16));
        bufferSpecs.push_back(HdBufferSpec(
                                  _tokens->transformInverse,
                                  GL_FLOAT,
                                  16));
        bufferSpecs.push_back(HdBufferSpec(
                                  _tokens->color,
                                  GL_FLOAT,
                                  4));
        bufferSpecs.push_back(HdBufferSpec(
                                  _tokens->primID,
                                  GL_FLOAT,
                                  4));
        bufferSpecs.push_back(HdBufferSpec(
                                  _tokens->widths,
                                  GL_FLOAT,
                                  1));
        HdBufferArrayRangeSharedPtr range =
            registry->AllocateShaderStorageBufferArrayRange(
                HdTokens->primVar, bufferSpecs);

        sharedData.barContainer.Set(
            drawingCoord->GetConstantPrimVarIndex(), range);
    }

    // element primvars
    {
        HdBufferSpecVector bufferSpecs;
        bufferSpecs.push_back(HdBufferSpec(
                                  _tokens->faceVisibility,
                                  GL_FLOAT,
                                  1));
        HdBufferArrayRangeSharedPtr range =
            registry->AllocateNonUniformBufferArrayRange(
                HdTokens->primVar, bufferSpecs);

        sharedData.barContainer.Set(
            drawingCoord->GetElementPrimVarIndex(), range);
    }

    // vertex primvars
    {
        HdBufferSpecVector bufferSpecs;
        bufferSpecs.push_back(HdBufferSpec(
                                  _tokens->points,
                                  GL_FLOAT,
                                  3));
        // XXX: The order of emitting multiple attribute is arbitrary
        // since HdBufferResourceMap uses hash_map of TfToken.
        // The resulting code becomes unstable if we have more than 1
        // primvars in the same category. We need to fix it.
        bufferSpecs.push_back(HdBufferSpec(
                                  _tokens->normals,
                                  GL_FLOAT,
                                  3));

        HdBufferArrayRangeSharedPtr range =
            registry->AllocateNonUniformBufferArrayRange(
                HdTokens->primVar, bufferSpecs);

        sharedData.barContainer.Set(
            drawingCoord->GetVertexPrimVarIndex(), range);
    }

    if (instance) {
        // instance primvars
        {
            HdBufferSpecVector bufferSpecs;
            bufferSpecs.push_back(HdBufferSpec(_tokens->translate,
                                               GL_FLOAT, 3));
            HdBufferArrayRangeSharedPtr range =
                registry->AllocateNonUniformBufferArrayRange(
                    HdTokens->primVar, bufferSpecs);

            drawingCoord->SetInstancePrimVarIndex(0, /*hard-coded*/8);

            sharedData.barContainer.Set(
                drawingCoord->GetInstancePrimVarIndex(0), range);
        }
        // instance index
        {
            HdBufferSpecVector bufferSpecs;
            bufferSpecs.push_back(HdBufferSpec(HdTokens->instanceIndices,
                                               GL_INT, 1));
            bufferSpecs.push_back(HdBufferSpec(HdTokens->culledInstanceIndices,
                                               GL_INT, 1));
            HdBufferArrayRangeSharedPtr range =
                registry->AllocateNonUniformBufferArrayRange(
                    HdTokens->primVar, bufferSpecs);
            sharedData.barContainer.Set(
                drawingCoord->GetInstanceIndexIndex(), range);
        }
    }

    Hd_ResourceBinder binder;
    HdBindingRequestVector empty;

    HdSurfaceShaderSharedPtr _surfaceFallback;
    GlfGLSLFXSharedPtr glslfx = GlfGLSLFXSharedPtr(new GlfGLSLFX(
                HdPackageFallbackSurfaceShader()));
    _surfaceFallback = HdSurfaceShaderSharedPtr(new HdGLSLFXShader(glslfx));
    _surfaceFallback->Sync();

    Hd_GeometricShaderSharedPtr geometricShader = Hd_GeometricShader::Create(key);
    HdShaderSharedPtrVector shaders(3);
    shaders[0].reset(new HdRenderPassShader());
    shaders[1].reset(new Hd_DefaultLightingShader());
    shaders[2] = _surfaceFallback;

    Hd_CodeGen codeGen(geometricShader, shaders);
    binder.ResolveBindings(&drawItem,
                           shaders,
                           codeGen.GetMetaData(),
                           useIndirect, /*instanced=*/true, empty);

    codeGen.Compile();

    std::cout <<
        "-------------------------------------------------------\n"
              << HdShaderKey::GetGLSLFXString(key)
              <<
        "-------------------------------------------------------\n";
    std::cout <<
        "=======================================================\n"
        "  VERTEX SHADER                                        \n"
        "=======================================================\n"
              << codeGen.GetVertexShaderSource();
    std::cout <<
        "=======================================================\n"
        "  TESS CONTROL SHADER                                  \n"
        "=======================================================\n"
              << codeGen.GetTessControlShaderSource();
    std::cout <<
        "=======================================================\n"
        "  TESS EVAL SHADER                                     \n"
        "=======================================================\n"
              << codeGen.GetTessEvalShaderSource();
    std::cout <<
        "=======================================================\n"
        "  GEOMETRY SHADER                                      \n"
        "=======================================================\n"
              << codeGen.GetGeometryShaderSource();
    std::cout <<
        "=======================================================\n"
        "  FRAGMENT SHADER                                      \n"
        "=======================================================\n"
              << codeGen.GetFragmentShaderSource();

    return TF_VERIFY(mark.IsClean());
}

template <typename KEY>
bool
TestShader(KEY const &key, bool indirect, bool bindless, bool instance)
{
    bool success = true;
    success &= CodeGenTest(key, indirect, bindless, instance);
    return success;
}

int main(int argc, char *argv[])
{
    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfGlewInit();
    GlfSharedGLContextScopeHolder sharedContext;

    bool success = true;

    bool lit = true;
    bool smoothNormals = false;
    bool doubleSided = false;
    bool faceVarying = false;
    bool authoredNormals = false;
    bool refine = false;
    bool instance = false;
    bool mesh = false;
    bool curves = false;
    bool points = false;
    bool indirect = false;
    bool bindless = false;

    for (int i=0; i<argc; ++i) {
        std::string arg(argv[i]);

        if (arg == "--smoothNormals") {
            smoothNormals = true;
        } else if (arg == "--doubleSided") {
            doubleSided = true;
        } else if (arg == "--faceVarying") {
            faceVarying = true;
        } else if (arg == "--instance") {
            instance = true;
        } else if (arg == "--indirect") {
            indirect = true;
        } else if (arg == "--bindless") {
            bindless = true;
        } else if (arg == "--mesh") {
            mesh = true;
        } else if (arg == "--curves") {
            curves = true;
        } else if (arg == "--points") {
            points = true;
        }
    }

    // mesh
    if (mesh) {
        success &= TestShader(
            Hd_MeshShaderKey(GL_TRIANGLES, lit, smoothNormals,
                             doubleSided, faceVarying,
                             HdCullStyleNothing,
                             HdMeshGeomStyleSurf),
            indirect, bindless, instance);
        success &= TestShader(
            Hd_MeshShaderKey(GL_LINES_ADJACENCY, lit, smoothNormals,
                             doubleSided, faceVarying,
                             HdCullStyleNothing,
                             HdMeshGeomStyleSurf),
            indirect, bindless, instance);
    }

    // curves
    if (curves) {
        success &= TestShader(Hd_BasisCurvesShaderKey(HdTokens->bezier,
                                                      authoredNormals, refine),
                              indirect, bindless, instance);
    }

    // points
    if (points) {
        success &= TestShader(Hd_PointsShaderKey(),
                              indirect, bindless, instance);
    }

    if (success) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
