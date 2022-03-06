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

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hdSt/meshShaderKey.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


namespace pxrImagingHdStMeshShaderKey {

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX,              "mesh.glslfx"))

    // normal mixins
    ((normalsScene,            "MeshNormal.Scene"))
    ((normalsScenePatches,     "MeshNormal.Scene.Patches"))
    ((normalsSmooth,           "MeshNormal.Smooth"))
    ((normalsFlat,             "MeshNormal.Flat"))
    ((normalsPass,             "MeshNormal.Pass"))
    ((normalsScreenSpaceFS,    "MeshNormal.Fragment.ScreenSpace"))

    ((normalsGeometryFlat,     "MeshNormal.Geometry.Flat"))
    ((normalsGeometryNoFlat,   "MeshNormal.Geometry.NoFlat"))

    ((normalsDoubleSidedFS,    "MeshNormal.Fragment.DoubleSided"))
    ((normalsSingleSidedFS,    "MeshNormal.Fragment.SingleSided"))

    ((faceCullNoneFS,          "MeshFaceCull.Fragment.None"))
    ((faceCullFrontFacingFS,   "MeshFaceCull.Fragment.FrontFacing"))
    ((faceCullBackFacingFS,    "MeshFaceCull.Fragment.BackFacing"))

    // wireframe mixins
    ((edgeNoneFS,              "MeshWire.Fragment.NoEdge"))

    ((edgeMaskTriangleFS,      "MeshWire.Fragment.EdgeMaskTriangle"))
    ((edgeMaskQuadFS,          "MeshWire.Fragment.EdgeMaskQuad"))
    ((edgeMaskRefinedQuadFS,   "MeshWire.Fragment.EdgeMaskRefinedQuad"))
    ((edgeMaskTriQuadFS,       "MeshWire.Fragment.EdgeMaskTriQuad"))
    ((edgeMaskRefinedTriQuadFS,"MeshWire.Fragment.EdgeMaskRefinedTriQuad"))
    ((edgeMaskNoneFS,          "MeshWire.Fragment.EdgeMaskNone"))

    ((edgeCommonFS,            "MeshWire.Fragment.EdgeCommon"))
    ((edgeParamFS,             "MeshWire.Fragment.EdgeParam"))

    ((edgeOnlyBlendFS,         "MeshWire.Fragment.EdgeOnlyBlendColor"))
    ((edgeOnlyNoBlendFS,       "MeshWire.Fragment.EdgeOnlyNoBlend"))

    ((edgeOnSurfFS,            "MeshWire.Fragment.EdgeOnSurface"))
    ((patchEdgeTriangleFS,     "MeshPatchWire.Fragment.PatchEdgeTriangle"))
    ((patchEdgeQuadFS,         "MeshPatchWire.Fragment.PatchEdgeQuad"))
    ((patchEdgeOnlyFS,         "MeshPatchWire.Fragment.EdgeOnly"))
    ((patchEdgeOnSurfFS,       "MeshPatchWire.Fragment.EdgeOnSurface"))

    ((selWireOffsetGS,         "Selection.Geometry.WireSelOffset"))
    ((selWireNoOffsetGS,       "Selection.Geometry.WireSelNoOffset"))
    
    // selection decoding
    ((selDecodeUtils,          "Selection.DecodeUtils"))
    ((selPointSelVS,           "Selection.Vertex.PointSel"))
    ((selElementSelGS,         "Selection.Geometry.ElementSel"))

    // edge id mixins (for edge picking & selection)
    ((edgeIdCommonFS,          "EdgeId.Fragment.Common"))
    ((edgeIdTriangleSurfFS,    "EdgeId.Fragment.TriangleSurface"))
    ((edgeIdTriangleLineFS,    "EdgeId.Fragment.TriangleLines"))
    ((edgeIdTriangleParamFS,   "EdgeId.Fragment.TriangleParam"))
    ((edgeIdQuadSurfFS,        "EdgeId.Fragment.QuadSurface"))
    ((edgeIdQuadLineFS,        "EdgeId.Fragment.QuadLines"))
    ((edgeIdQuadParamFS,       "EdgeId.Fragment.QuadParam"))

    // point id mixins (for point picking & selection)
    ((pointIdNoneVS,           "PointId.Vertex.None"))
    ((pointIdVS,               "PointId.Vertex.PointParam"))
    ((pointIdFallbackFS,       "PointId.Fragment.Fallback"))
    ((pointIdFS,               "PointId.Fragment.PointParam"))

    // visibility mixin (for face and point visibility)
    ((topVisFallbackFS,        "Visibility.Fragment.Fallback"))
    ((topVisFS,                "Visibility.Fragment.Topology"))

    // main for all the shader stages
    ((mainVS,                  "Mesh.Vertex"))
    ((mainBSplineQuadTCS,      "Mesh.TessControl.BSplineQuad"))
    ((mainBezierQuadTES,       "Mesh.TessEval.BezierQuad"))
    ((mainBoxSplineTriangleTCS,"Mesh.TessControl.BoxSplineTriangle"))
    ((mainBezierTriangleTES,   "Mesh.TessEval.BezierTriangle"))
    ((mainVaryingInterpTES,    "Mesh.TessEval.VaryingInterpolation"))
    ((mainTriangleTessGS,      "Mesh.Geometry.TriangleTess"))
    ((mainTriangleGS,          "Mesh.Geometry.Triangle"))
    ((mainTriQuadGS,           "Mesh.Geometry.TriQuad"))
    ((mainQuadGS,              "Mesh.Geometry.Quad"))
    ((mainPatchCoordFS,        "Mesh.Fragment.PatchCoord"))
    ((mainPatchCoordTriangleFS,"Mesh.Fragment.PatchCoord.Triangle"))
    ((mainPatchCoordQuadFS,    "Mesh.Fragment.PatchCoord.Quad"))
    ((mainPatchCoordTriQuadFS, "Mesh.Fragment.PatchCoord.TriQuad"))
    ((mainFS,                  "Mesh.Fragment"))

    // instancing related mixins
    ((instancing,              "Instancing.Transform"))

    // terminals
    ((customDisplacementGS,    "Geometry.CustomDisplacement"))
    ((noCustomDisplacementGS,  "Geometry.NoCustomDisplacement"))
    ((commonFS,                "Fragment.CommonTerminals"))
    ((surfaceFS,               "Fragment.Surface"))
    ((surfaceUnlitFS,          "Fragment.SurfaceUnlit"))
    ((surfaceSheerFS,          "Fragment.SurfaceSheer"))
    ((surfaceOutlineFS,        "Fragment.SurfaceOutline"))
    ((constantColorFS,         "Fragment.ConstantColor"))
    ((hullColorFS,             "Fragment.HullColor"))
    ((pointColorFS,            "Fragment.PointColor"))
    ((scalarOverrideFS,        "Fragment.ScalarOverride"))
    ((noScalarOverrideFS,      "Fragment.NoScalarOverride"))
);

} // pxrImagingHdStMeshShaderKey

HdSt_MeshShaderKey::HdSt_MeshShaderKey(
    HdSt_GeometricShader::PrimitiveType primitiveType,
    TfToken shadingTerminal,
    NormalSource normalsSource,
    HdInterpolation normalsInterpolation,
    HdCullStyle cullStyle,
    HdMeshGeomStyle geomStyle,
    HdSt_GeometricShader::FvarPatchType fvarPatchType,
    float lineWidth,
    bool doubleSided,
    bool hasBuiltinBarycentrics,
    bool hasCustomDisplacement,
    bool hasPerFaceInterpolation,
    bool hasTopologicalVisibility,
    bool blendWireframeColor,
    bool hasMirroredTransform,
    bool hasInstancer,
    bool enableScalarOverride)
    : primType(primitiveType)
    , cullStyle(cullStyle)
    , hasMirroredTransform(hasMirroredTransform)
    , doubleSided(doubleSided)
    , polygonMode(HdPolygonModeFill)
    , lineWidth(lineWidth)
    , fvarPatchType(fvarPatchType)
    , glslfx(pxrImagingHdStMeshShaderKey::_tokens->baseGLSLFX)
{
    if (geomStyle == HdMeshGeomStyleEdgeOnly ||
        geomStyle == HdMeshGeomStyleHullEdgeOnly) {
        polygonMode = HdPolygonModeLine;
    }

    // XXX: Unfortunately instanced meshes can't use h/w culling. This is due to
    // the possibility that they have instanceTransform/instanceScale primvars.
    useHardwareFaceCulling = !hasInstancer;

    bool isPrimTypePoints = HdSt_GeometricShader::IsPrimTypePoints(primType);
    bool isPrimTypeTris   = HdSt_GeometricShader::IsPrimTypeTriangles(primType);
    bool isPrimTypeQuads  = HdSt_GeometricShader::IsPrimTypeQuads(primType);
    bool isPrimTypeTriQuads =
        HdSt_GeometricShader::IsPrimTypeTriQuads(primType);
    bool isPrimTypeRefinedMesh =
        HdSt_GeometricShader::IsPrimTypeRefinedMesh(primType);
    const bool isPrimTypePatches =
        HdSt_GeometricShader::IsPrimTypePatches(primType);
    const bool isPrimTypePatchesBSpline =
        primType ==
            HdSt_GeometricShader::PrimitiveType::PRIM_MESH_BSPLINE;
    const bool isPrimTypePatchesBoxSplineTriangle =
        primType ==
            HdSt_GeometricShader::PrimitiveType::PRIM_MESH_BOXSPLINETRIANGLE;

    const bool renderWireframe = geomStyle == HdMeshGeomStyleEdgeOnly ||
                                 geomStyle == HdMeshGeomStyleHullEdgeOnly;    

    const bool renderEdges = geomStyle == HdMeshGeomStyleEdgeOnSurf ||
                             geomStyle == HdMeshGeomStyleHullEdgeOnSurf;

    /* Normals configurations:
     * Smooth normals:
     *   [VS] .Smooth, ([GS] .NoFlat, .Pass), [FS] .Pass
     *   (geometry shader optional)
     * Scene normals:
     *   [VS] .Scene, ([GS] .NoFlat, .Pass), [FS] .Pass
     *   --or-- [VS] .Pass, [GS] .NoFlat, .Scene, [FS] .Pass
     *   --or-- [VS] .Pass, [FS] .Scene
     *   (depending on interpolation)
     * Limit normals:
     *   [VS] .Pass, [GS] .NoFlat, .Pass, [FS] .Pass
     * Flat normals:
     *   [VS] .Pass, [GS] .Flat, .Pass, [FS] .Pass
     * Screen Space Flat normals:
     *   [VS] .Pass, [GS] .Pass, [FS] .ScreenSpace
     *   (geometry shader optional)
     */
    bool vsSceneNormals =
        (normalsSource == NormalSourceScene &&
        normalsInterpolation != HdInterpolationUniform &&
        normalsInterpolation != HdInterpolationFaceVarying);
    bool gsSceneNormals =
        (normalsSource == NormalSourceScene && !vsSceneNormals);

    // vertex shader
    uint8_t vsIndex = 0;
    VS[vsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->instancing;

    VS[vsIndex++] = (normalsSource == NormalSourceSmooth) ?
        pxrImagingHdStMeshShaderKey::_tokens->normalsSmooth :
        (vsSceneNormals ? pxrImagingHdStMeshShaderKey::_tokens->normalsScene : pxrImagingHdStMeshShaderKey::_tokens->normalsPass);

    if (isPrimTypePoints) {
        // Add mixins that allow for picking and sel highlighting of points.
        // Even though these are more "render pass-ish", we do this here to
        // reduce the shader code generated when the points repr isn't used.
        VS[vsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->pointIdVS;
        VS[vsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->selDecodeUtils;
        VS[vsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->selPointSelVS;
    } else {
        VS[vsIndex++] =  pxrImagingHdStMeshShaderKey::_tokens->pointIdNoneVS;
    }
    VS[vsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->mainVS;
    VS[vsIndex] = TfToken();

    // tessellation control shader
    TCS[0] = isPrimTypePatches ? pxrImagingHdStMeshShaderKey::_tokens->instancing : TfToken();
    TCS[1] = isPrimTypePatches ? isPrimTypePatchesBSpline
                                   ? pxrImagingHdStMeshShaderKey::_tokens->mainBSplineQuadTCS
                                   : pxrImagingHdStMeshShaderKey::_tokens->mainBoxSplineTriangleTCS
                               : TfToken();
    TCS[2] = TfToken();

    // tessellation evaluation shader
    TES[0] = isPrimTypePatches ? pxrImagingHdStMeshShaderKey::_tokens->instancing : TfToken();
    TES[1] = isPrimTypePatches ? isPrimTypePatchesBSpline
                                   ? pxrImagingHdStMeshShaderKey::_tokens->mainBezierQuadTES
                                   : pxrImagingHdStMeshShaderKey::_tokens->mainBezierTriangleTES
                               : TfToken();
    TES[2] = isPrimTypePatches ? pxrImagingHdStMeshShaderKey::_tokens->mainVaryingInterpTES : TfToken();
    TES[3] = TfToken();

    // geometry shader
    uint8_t gsIndex = 0;
    GS[gsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->instancing;

    GS[gsIndex++] = (normalsSource == NormalSourceFlat) ?
        pxrImagingHdStMeshShaderKey::_tokens->normalsFlat :
        (gsSceneNormals ? (isPrimTypePatches ? pxrImagingHdStMeshShaderKey::_tokens->normalsScenePatches : 
            pxrImagingHdStMeshShaderKey::_tokens->normalsScene) : 
        pxrImagingHdStMeshShaderKey::_tokens->normalsPass);
   
    GS[gsIndex++] = (normalsSource == NormalSourceGeometryShader) ?
            pxrImagingHdStMeshShaderKey::_tokens->normalsGeometryFlat : pxrImagingHdStMeshShaderKey::_tokens->normalsGeometryNoFlat;

    // emit "ComputeSelectionOffset" GS function.
    if (renderWireframe) {
        // emit necessary selection decoding and helper mixins
        GS[gsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->selDecodeUtils;
        GS[gsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->selElementSelGS;
        GS[gsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->selWireOffsetGS;
    } else {
        GS[gsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->selWireNoOffsetGS;
    }

    // Displacement shading can be disabled explicitly, or if the entrypoint
    // doesn't exist (resolved in HdStMesh).
    GS[gsIndex++] = (!hasCustomDisplacement) ?
        pxrImagingHdStMeshShaderKey::_tokens->noCustomDisplacementGS :
        pxrImagingHdStMeshShaderKey::_tokens->customDisplacementGS;

    GS[gsIndex++] = isPrimTypeQuads
                        ? pxrImagingHdStMeshShaderKey::_tokens->mainQuadGS
                        : isPrimTypeTriQuads
                            ? pxrImagingHdStMeshShaderKey::_tokens->mainTriQuadGS
                            : isPrimTypePatches
                                ? pxrImagingHdStMeshShaderKey::_tokens->mainTriangleTessGS
                                    : pxrImagingHdStMeshShaderKey::_tokens->mainTriangleGS;
    GS[gsIndex] = TfToken();

    // Optimization : See if we can skip the geometry shader.
    bool canSkipGS =
            // whether we can skip executing the displacement shading terminal
            !hasCustomDisplacement
            // whether we can skip geometry shader normals plumbing
            && (normalsSource != NormalSourceLimit)
            && (normalsSource != NormalSourceGeometryShader)
            // whether we can skip splitting quads to triangles
            && (isPrimTypeTris ||
                isPrimTypeTriQuads)
            // whether we can skip generating coords for edges
            && ((!renderWireframe && !renderEdges) ||
                hasBuiltinBarycentrics)
            // whether we can skip generating coords for per-face interpolation
            && (!hasPerFaceInterpolation ||
                hasBuiltinBarycentrics)
            ;
            
    if (canSkipGS) {
        GS[0] = TfToken();
    }

    // Optimization : Points don't need any sort of geometry shader so
    //                we ignore it here.
    if (isPrimTypePoints) {
        GS[0] = TfToken();
    }

    bool gsStageEnabled = !GS[0].IsEmpty();

    // fragment shader
    uint8_t fsIndex = 0;
    FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->instancing;

    FS[fsIndex++] =
        (normalsSource == NormalSourceScreenSpace)
            ? pxrImagingHdStMeshShaderKey::_tokens->normalsScreenSpaceFS
            : (!gsStageEnabled && normalsSource == NormalSourceFlat)
                ? pxrImagingHdStMeshShaderKey::_tokens->normalsFlat
                : ((!gsStageEnabled && gsSceneNormals)
                    ? pxrImagingHdStMeshShaderKey::_tokens->normalsScene
                    : pxrImagingHdStMeshShaderKey::_tokens->normalsPass);

    FS[fsIndex++] = doubleSided ?
        pxrImagingHdStMeshShaderKey::_tokens->normalsDoubleSidedFS : pxrImagingHdStMeshShaderKey::_tokens->normalsSingleSidedFS;

    bool const faceCullFrontFacing =
        !useHardwareFaceCulling &&
            (cullStyle == HdCullStyleFront ||
             (cullStyle == HdCullStyleFrontUnlessDoubleSided && !doubleSided));
    bool const faceCullBackFacing =
        !useHardwareFaceCulling &&
            (cullStyle == HdCullStyleBack ||
             (cullStyle == HdCullStyleBackUnlessDoubleSided && !doubleSided));
    FS[fsIndex++] =
        faceCullFrontFacing
            ? pxrImagingHdStMeshShaderKey::_tokens->faceCullFrontFacingFS
            : faceCullBackFacing
                ? pxrImagingHdStMeshShaderKey::_tokens->faceCullBackFacingFS
                : pxrImagingHdStMeshShaderKey::_tokens->faceCullNoneFS; // DontCare, Nothing, HW

    // Wire (edge) related mixins
    if (renderWireframe || renderEdges) {
        if (isPrimTypeRefinedMesh) {
            if (isPrimTypeTriQuads) {
                FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->edgeMaskRefinedTriQuadFS;
            } else if (isPrimTypeQuads) {
                FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->edgeMaskRefinedQuadFS;
            } else {
                FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->edgeMaskNoneFS;
            }
        } else if (isPrimTypeTris) {
            FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->edgeMaskTriangleFS;
        } else if (isPrimTypeTriQuads) {
            FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->edgeMaskTriQuadFS;
        } else {
            FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->edgeMaskQuadFS;
        }

        FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->edgeCommonFS;
        FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->edgeParamFS;

        if (renderWireframe) {
            if (isPrimTypePatches) {
                FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->patchEdgeOnlyFS;
            } else {
                FS[fsIndex++] = blendWireframeColor
                                    ? pxrImagingHdStMeshShaderKey::_tokens->edgeOnlyBlendFS
                                    : pxrImagingHdStMeshShaderKey::_tokens->edgeOnlyNoBlendFS;
            }
        } else {
            if (isPrimTypeTris || isPrimTypePatchesBoxSplineTriangle) {
                FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->patchEdgeTriangleFS;
            } else {
                FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->patchEdgeQuadFS;
            }
            if (isPrimTypeRefinedMesh) {
                FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->patchEdgeOnSurfFS;
            } else {
                FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->edgeOnSurfFS;
            }
        }
    } else {
        FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->edgeNoneFS;
        FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->edgeParamFS;
    }

    // Shading terminal mixin
    TfToken terminalFS;
    if (shadingTerminal == HdMeshReprDescTokens->surfaceShader) {
        terminalFS = pxrImagingHdStMeshShaderKey::_tokens->surfaceFS;
    } else if (shadingTerminal == HdMeshReprDescTokens->surfaceShaderUnlit) {
        terminalFS = pxrImagingHdStMeshShaderKey::_tokens->surfaceUnlitFS;
    } else if (shadingTerminal == HdMeshReprDescTokens->surfaceShaderSheer) {
        terminalFS = pxrImagingHdStMeshShaderKey::_tokens->surfaceSheerFS;
    } else if (shadingTerminal == HdMeshReprDescTokens->surfaceShaderOutline) {
        terminalFS = pxrImagingHdStMeshShaderKey::_tokens->surfaceOutlineFS;
    } else if (shadingTerminal == HdMeshReprDescTokens->constantColor) {
        terminalFS = pxrImagingHdStMeshShaderKey::_tokens->constantColorFS;
    } else if (shadingTerminal == HdMeshReprDescTokens->hullColor) {
        terminalFS = pxrImagingHdStMeshShaderKey::_tokens->hullColorFS;
    } else if (shadingTerminal == HdMeshReprDescTokens->pointColor) {
        terminalFS = pxrImagingHdStMeshShaderKey::_tokens->pointColorFS;
    } else if (!shadingTerminal.IsEmpty()) {
        terminalFS = shadingTerminal;
    } else {
        terminalFS = pxrImagingHdStMeshShaderKey::_tokens->surfaceFS;
    }

    // Common must be first as it defines terminal interfaces
    FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->commonFS;
    FS[fsIndex++] = terminalFS;

    FS[fsIndex++] =  enableScalarOverride ? pxrImagingHdStMeshShaderKey::_tokens->scalarOverrideFS :
                                            pxrImagingHdStMeshShaderKey::_tokens->noScalarOverrideFS;

    // EdgeId mixin(s) for edge picking and selection
    // Note: When rendering a mesh as points, we handle this in code gen.
    if (!isPrimTypePoints) {
        FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->edgeIdCommonFS;
        if (isPrimTypeTris || isPrimTypePatchesBoxSplineTriangle) {
            if (polygonMode == HdPolygonModeLine) {
                FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->edgeIdTriangleLineFS;
            } else {
                FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->edgeIdTriangleSurfFS;
            }
            FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->edgeIdTriangleParamFS;
        } else {
            if (polygonMode == HdPolygonModeLine) {
                FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->edgeIdQuadLineFS;
            } else {
                FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->edgeIdQuadSurfFS;
            }
            FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->edgeIdQuadParamFS;
        }
    }

    // PointId mixin for point picking and selection
    FS[fsIndex++] = isPrimTypePoints? pxrImagingHdStMeshShaderKey::_tokens->pointIdFS :
                                      pxrImagingHdStMeshShaderKey::_tokens->pointIdFallbackFS;
    FS[fsIndex++] = hasTopologicalVisibility? pxrImagingHdStMeshShaderKey::_tokens->topVisFS :
                                              pxrImagingHdStMeshShaderKey::_tokens->topVisFallbackFS;

    if (isPrimTypeTris && !gsStageEnabled) {
        FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->mainPatchCoordTriangleFS;
    } else if (isPrimTypeQuads && !gsStageEnabled) {
        FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->mainPatchCoordQuadFS;
    } else if (isPrimTypeTriQuads) {
        FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->mainPatchCoordTriQuadFS;
    } else {
        FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->mainPatchCoordFS;
    }
    FS[fsIndex++] = pxrImagingHdStMeshShaderKey::_tokens->mainFS;
    FS[fsIndex] = TfToken();
}

HdSt_MeshShaderKey::~HdSt_MeshShaderKey()
{
}

PXR_NAMESPACE_CLOSE_SCOPE

