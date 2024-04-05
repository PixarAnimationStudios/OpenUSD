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


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX,                  "mesh.glslfx"))

    // normal mixins
    ((normalsScene,                "MeshNormal.Scene"))
    ((normalsScenePatches,         "MeshNormal.Scene.Patches"))
    ((normalsSmooth,               "MeshNormal.Smooth"))
    ((normalsFlat,                 "MeshNormal.Flat"))
    ((normalsPass,                 "MeshNormal.Pass"))
    ((normalsScreenSpaceFS,        "MeshNormal.Fragment.ScreenSpace"))

    ((normalsGeometryFlat,         "MeshNormal.Geometry.Flat"))
    ((normalsGeometryNoFlat,       "MeshNormal.Geometry.NoFlat"))

    ((normalsDoubleSidedFS,        "MeshNormal.Fragment.DoubleSided"))
    ((normalsSingleSidedFS,        "MeshNormal.Fragment.SingleSided"))

    ((faceCullNoneFS,              "MeshFaceCull.Fragment.None"))
    ((faceCullFrontFacingFS,       "MeshFaceCull.Fragment.FrontFacing"))
    ((faceCullBackFacingFS,        "MeshFaceCull.Fragment.BackFacing"))

    // wireframe mixins
    ((edgeNoneFS,                  "MeshWire.Fragment.NoEdge"))

    ((edgeOpacityNoForceFS,        "MeshWire.Fragment.FinalEdgeOpacityNoForce"))
    ((edgeOpacityForceFS,          "MeshWire.Fragment.FinalEdgeOpacityForce"))

    ((edgeMaskTriangleFS,          "MeshWire.Fragment.EdgeMaskTriangle"))
    ((edgeMaskQuadFS,              "MeshWire.Fragment.EdgeMaskQuad"))
    ((edgeMaskRefinedQuadFS,       "MeshWire.Fragment.EdgeMaskRefinedQuad"))
    ((edgeMaskTriQuadFS,           "MeshWire.Fragment.EdgeMaskTriQuad"))
    ((edgeMaskNoneFS,              "MeshWire.Fragment.EdgeMaskNone"))

    ((edgeCommonFS,                "MeshWire.Fragment.EdgeCommon"))
    ((edgeTriQuadPTVSFS,           "MeshWire.Fragment.EdgeTriQuadPTVS"))
    ((edgeParamFS,                 "MeshWire.Fragment.EdgeParam"))

    ((edgeOnlyBlendFS,             "MeshWire.Fragment.EdgeOnlyBlendColor"))
    ((edgeOnlyNoBlendFS,           "MeshWire.Fragment.EdgeOnlyNoBlend"))
                         
    ((edgeCoordBarycentricCoordFS, "MeshWire.Fragment.EdgeCoord.Barycentric"))
    ((edgeCoordTessCoordFS,        "MeshWire.Fragment.EdgeCoord.Tess"))
    ((edgeCoordTessCoordTriangleFS,"MeshWire.Fragment.EdgeCoord.TessTriangle"))

    ((edgeOnSurfFS,                "MeshWire.Fragment.EdgeOnSurface"))
    ((patchEdgeTriangleFS,         "MeshPatchWire.Fragment.PatchEdgeTriangle"))
    ((patchEdgeQuadFS,             "MeshPatchWire.Fragment.PatchEdgeQuad"))
    ((patchEdgeOnlyFS,             "MeshPatchWire.Fragment.EdgeOnly"))
    ((patchEdgeOnSurfFS,           "MeshPatchWire.Fragment.EdgeOnSurface"))

    ((selWireOffsetGS,             "Selection.Geometry.WireSelOffset"))
    ((selWireNoOffsetGS,           "Selection.Geometry.WireSelNoOffset"))
    
    // selection decoding
    ((selDecodeUtils,              "Selection.DecodeUtils"))
    ((selPointSelVS,               "Selection.Vertex.PointSel"))
    ((selElementSelGS,             "Selection.Geometry.ElementSel"))

    // edge id mixins (for edge picking & selection)
    ((edgeIdCommonFS,              "EdgeId.Fragment.Common"))
    ((edgeIdTriangleSurfFS,        "EdgeId.Fragment.TriangleSurface"))
    ((edgeIdTriangleLineFS,        "EdgeId.Fragment.TriangleLines"))
    ((edgeIdTriangleParamFS,       "EdgeId.Fragment.TriangleParam"))
    ((edgeIdQuadSurfFS,            "EdgeId.Fragment.QuadSurface"))
    ((edgeIdQuadLineFS,            "EdgeId.Fragment.QuadLines"))
    ((edgeIdQuadParamFS,           "EdgeId.Fragment.QuadParam"))

    // point id mixins (for point picking & selection)
    ((pointIdNoneVS,               "PointId.Vertex.None"))
    ((pointIdVS,                   "PointId.Vertex.PointParam"))
    ((pointIdFallbackFS,           "PointId.Fragment.Fallback"))
    ((pointIdFS,                   "PointId.Fragment.PointParam"))

    // visibility mixin (for face and point visibility)
    ((topVisFallbackFS,            "Visibility.Fragment.Fallback"))
    ((topVisFS,                    "Visibility.Fragment.Topology"))

    // main for all the shader stages
    ((mainVS,                      "Mesh.Vertex"))
    ((mainPatchCommonTCS,          "Mesh.TessControl.PatchCommon"))
    ((mainBSplineQuadTCS,          "Mesh.TessControl.BSplineQuad"))
    ((mainBezierQuadTES,           "Mesh.TessEval.BezierQuad"))
    ((mainBoxSplineTriangleTCS,    "Mesh.TessControl.BoxSplineTriangle"))
    ((mainBezierTriangleTES,       "Mesh.TessEval.BezierTriangle"))
    ((mainVaryingInterpTES,        "Mesh.TessEval.VaryingInterpolation"))
    ((mainTrianglePTVS,            "Mesh.PostTessVertex.Triangle"))
    ((mainQuadPTVS,                "Mesh.PostTessVertex.Quad"))
    ((mainTriQuadPTVS,             "Mesh.PostTessVertex.TriQuad"))
    ((mainPatchCommonPTCS,         "Mesh.PostTessControl.PatchCommon"))
    ((mainBSplineQuadPTCS,         "Mesh.PostTessControl.BSplineQuad"))
    ((mainBSplineQuadPTVS,         "Mesh.PostTessVertex.BSplineQuad"))
    ((mainBoxSplineTrianglePTCS,   "Mesh.PostTessControl.BoxSplineTriangle"))
    ((mainBoxSplineTrianglePTVS,   "Mesh.PostTessVertex.BoxSplineTriangle"))
    ((mainTriangleTessGS,          "Mesh.Geometry.TriangleTess"))
    ((mainTriangleGS,              "Mesh.Geometry.Triangle"))
    ((mainTriQuadGS,               "Mesh.Geometry.TriQuad"))
    ((mainQuadGS,                  "Mesh.Geometry.Quad"))
    ((mainPatchCoordFS,            "Mesh.Fragment.PatchCoord"))
    ((mainPatchCoordNoGSFS,        "Mesh.Fragment.PatchCoord.NoGS"))
    ((mainPatchCoordTessFS,        "Mesh.Fragment.PatchCoord.Tess"))
    ((mainPatchCoordTriangleFS,    "Mesh.Fragment.PatchCoord.Triangle"))
    ((mainPatchCoordQuadFS,        "Mesh.Fragment.PatchCoord.Quad"))
    ((mainPatchCoordTriQuadFS,     "Mesh.Fragment.PatchCoord.TriQuad"))
    ((mainPatchCoordTrianglePTVSFS,"Mesh.Fragment.PatchCoord.TrianglePTVS"))
    ((mainPatchCoordQuadPTVSFS,    "Mesh.Fragment.PatchCoord.QuadPTVS"))
    ((mainPatchCoordTriQuadPTVSFS, "Mesh.Fragment.PatchCoord.TriQuadPTVS"))
    ((mainFS,                      "Mesh.Fragment"))

    // instancing related mixins
    ((instancing,                  "Instancing.Transform"))

    // terminals
    ((customDisplacementGS,        "Geometry.CustomDisplacement"))
    ((noCustomDisplacementGS,      "Geometry.NoCustomDisplacement"))
    ((commonFS,                    "Fragment.CommonTerminals"))
    ((surfaceFS,                   "Fragment.Surface"))
    ((surfaceUnlitFS,              "Fragment.SurfaceUnlit"))
    ((surfaceSheerFS,              "Fragment.SurfaceSheer"))
    ((surfaceOutlineFS,            "Fragment.SurfaceOutline"))
    ((constantColorFS,             "Fragment.ConstantColor"))
    ((hullColorFS,                 "Fragment.HullColor"))
    ((pointColorFS,                "Fragment.PointColor"))
    ((pointShadedFS,               "Fragment.PointShaded"))
    ((scalarOverrideFS,            "Fragment.ScalarOverride"))
    ((noScalarOverrideFS,          "Fragment.NoScalarOverride"))
);

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
    bool hasMetalTessellation,
    bool hasCustomDisplacement,
    bool hasPerFaceInterpolation,
    bool hasTopologicalVisibility,
    bool blendWireframeColor,
    bool hasMirroredTransform,
    bool hasInstancer,
    bool enableScalarOverride,
    bool pointsShadingEnabled,
    bool forceOpaqueEdges)
    : primType(primitiveType)
    , cullStyle(cullStyle)
    , hasMirroredTransform(hasMirroredTransform)
    , doubleSided(doubleSided)
    , useMetalTessellation(false)
    , polygonMode(HdPolygonModeFill)
    , lineWidth(lineWidth)
    , fvarPatchType(fvarPatchType)
    , glslfx(_tokens->baseGLSLFX)
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

    // Selected edges can be highlighted even if not otherwise displayed
    const bool renderSelectedEdges = geomStyle == HdMeshGeomStyleSurf ||
                                     geomStyle == HdMeshGeomStyleHull;

    /* Normals configurations:
     * Smooth normals:
     *   [VS] .Smooth, [PTVS] .Smooth, ([GS] .NoFlat, .Pass), [FS] .Pass
     *   (geometry shader, ptvs optional)
     * Scene normals:
     *   [VS] .Scene, [PTVS] .Scene, ([GS] .NoFlat, .Pass), [FS] .Pass
     *   --or-- [VS] .Pass, [PTVS] .Scene, [GS] .NoFlat, .Scene, [FS] .Pass
     *   --or-- [VS] .Pass, [PTVS] .Pass, [FS] .Scene
     *   (depending on interpolation)
     *   (ptvs optional)
     * Limit normals:
     *   [VS] .Pass, [PTVS] .Pass, [GS] .NoFlat, .Pass, [FS] .Pass
     *   (ptvs optional)
     * Flat normals:
     *   [VS] .Pass, [PTVS] .Flat, [GS] .Flat, .Pass, [FS] .Pass
     *   (ptvs optional)
    * Screen Space Flat normals:
     *   [VS] .Pass, [PTVS] .Pass, [GS] .Pass, [FS] .ScreenSpace
     *   (geometry shader, ptvs optional)
     * Geometric Flat normals:
     *   [VS] .Pass, [PTVS] .Pass, [GS] .Flat, .Pass, [FS] .Pass
     *   (ptvs optional)
     */
    bool const vsSceneNormals =
        (normalsSource == NormalSourceScene &&
        normalsInterpolation != HdInterpolationUniform &&
        normalsInterpolation != HdInterpolationFaceVarying);
    bool const gsSceneNormals =
        (normalsSource == NormalSourceScene && !vsSceneNormals);
    bool const ptvsSceneNormals =
        (normalsSource == NormalSourceScene && !vsSceneNormals);
    bool const ptvsGeometricNormals =
        (normalsSource == NormalSourceFlatGeometric);

    // vertex shader
    uint8_t vsIndex = 0;
    VS[vsIndex++] = _tokens->instancing;

    VS[vsIndex++] = (normalsSource == NormalSourceSmooth) ?
        _tokens->normalsSmooth :
        (vsSceneNormals ? _tokens->normalsScene : _tokens->normalsPass);

    if (isPrimTypePoints) {
        // Add mixins that allow for picking and sel highlighting of points.
        // Even though these are more "render pass-ish", we do this here to
        // reduce the shader code generated when the points repr isn't used.
        VS[vsIndex++] = _tokens->pointIdVS;
        VS[vsIndex++] = _tokens->selDecodeUtils;
        VS[vsIndex++] = _tokens->selPointSelVS;
    } else {
        VS[vsIndex++] =  _tokens->pointIdNoneVS;
    }
    VS[vsIndex++] = _tokens->mainVS;
    VS[vsIndex] = TfToken();

    // Determine if PTVS should be used for Metal.
    bool const usePTVSTechniques =
            isPrimTypePatches ||
            hasCustomDisplacement ||
            ptvsSceneNormals ||
            ptvsGeometricNormals ||
            !hasBuiltinBarycentrics;

    // Determine if using actually using Metal PTVS.
    useMetalTessellation =
        hasMetalTessellation && !isPrimTypePoints && usePTVSTechniques;

    // PTVS shaders can provide barycentric coords w/o GS.
    bool const hasFragmentShaderBarycentrics =
        hasBuiltinBarycentrics || useMetalTessellation;

    // post tess vertex shader vertex steps
    uint8_t ptvsIndex = 0;
    uint8_t ptcsIndex = 0;
    if (useMetalTessellation) {
        PTVS[ptvsIndex++] = _tokens->instancing;

        // PTVS handles both usual "vs" normals and then later it may also 
        // handle a separate fallback calculation.
        // Here we handle the fallback possibility.
        if (ptvsGeometricNormals) {
            PTVS[ptvsIndex++] = _tokens->normalsGeometryFlat;
        } else {
            PTVS[ptvsIndex++] = _tokens->normalsGeometryNoFlat;
        }

        // Now handle the vs style normals
        if (normalsSource == NormalSourceFlat) {
            PTVS[ptvsIndex++] = _tokens->normalsFlat;
        }
        else if (normalsSource == NormalSourceSmooth) {
            PTVS[ptvsIndex++] = _tokens->normalsSmooth;
        } else if (vsSceneNormals) {
            PTVS[ptvsIndex++] = _tokens->normalsScene;
        } else if (gsSceneNormals && isPrimTypePatches) {
            PTVS[ptvsIndex++] = _tokens->normalsScenePatches;
        } else {
            PTVS[ptvsIndex++] = _tokens->normalsPass;
        }

        if (isPrimTypePoints) {
            // Add mixins that allow for picking and sel highlighting of points.
            // Even though these are more "render pass-ish", we do this here to
            // reduce the shader code generated when the points repr isn't used.
            PTVS[ptvsIndex++] = _tokens->pointIdVS;
            PTVS[ptvsIndex++] = _tokens->selDecodeUtils;
            PTVS[ptvsIndex++] = _tokens->selPointSelVS;
        } else {
            PTVS[ptvsIndex++] =  _tokens->pointIdNoneVS;
        }

        if (hasCustomDisplacement) {
            PTVS[ptvsIndex++] = _tokens->customDisplacementGS;
        } else {
            PTVS[ptvsIndex++] = _tokens->noCustomDisplacementGS;
        }

        if (isPrimTypeTris) {
            PTVS[ptvsIndex++] = _tokens->mainTrianglePTVS;
        } else if (isPrimTypeQuads) {
            PTVS[ptvsIndex++] = _tokens->mainQuadPTVS;
        } else if (isPrimTypeTriQuads) {
            PTVS[ptvsIndex++] = _tokens->mainTriQuadPTVS;
        } else if (isPrimTypePatchesBSpline) {
            PTCS[ptcsIndex++] = _tokens->instancing;
            PTCS[ptcsIndex++] = _tokens->mainPatchCommonPTCS;
            PTCS[ptcsIndex++] = _tokens->mainBSplineQuadPTCS;
            PTVS[ptvsIndex++] = _tokens->mainBSplineQuadPTVS;
        } else if (isPrimTypePatchesBoxSplineTriangle) {
            PTCS[ptcsIndex++] = _tokens->instancing;
            PTCS[ptcsIndex++] = _tokens->mainPatchCommonPTCS;
            PTCS[ptcsIndex++] = _tokens->mainBoxSplineTrianglePTCS;
            PTVS[ptvsIndex++] = _tokens->mainBoxSplineTrianglePTVS;
        }
        
        PTVS[ptvsIndex] = TfToken();
        PTCS[ptcsIndex] = TfToken();
    }

    bool const ptvsStageEnabled = !PTVS[0].IsEmpty();

    // tessellation control shader
    if (isPrimTypePatches && !ptvsStageEnabled) {
        TCS[0] = _tokens->instancing;
        TCS[1] = _tokens->mainPatchCommonTCS;
        TCS[2] = isPrimTypePatchesBSpline
                     ? _tokens->mainBSplineQuadTCS
                     : _tokens->mainBoxSplineTriangleTCS;
        TCS[3] = TfToken();

        // tessellation evaluation shader
        TES[0] = _tokens->instancing;
        TES[1] = isPrimTypePatchesBSpline
                     ? _tokens->mainBezierQuadTES
                     : _tokens->mainBezierTriangleTES;
        TES[2] = _tokens->mainVaryingInterpTES;
        TES[3] = TfToken();
    } else {
        TCS[0] = TfToken();
        TES[0] = TfToken();
    }

    // geometry shader
    uint8_t gsIndex = 0;
    GS[gsIndex++] = _tokens->instancing;

    GS[gsIndex++] = (normalsSource == NormalSourceFlat) ?
        _tokens->normalsFlat :
        (gsSceneNormals ? (isPrimTypePatches ? _tokens->normalsScenePatches : 
            _tokens->normalsScene) : 
        _tokens->normalsPass);
   
    GS[gsIndex++] = (normalsSource == NormalSourceFlatGeometric) ?
            _tokens->normalsGeometryFlat : _tokens->normalsGeometryNoFlat;

    // emit "ComputeSelectionOffset" GS function.
    if (renderWireframe) {
        // emit necessary selection decoding and helper mixins
        GS[gsIndex++] = _tokens->selDecodeUtils;
        GS[gsIndex++] = _tokens->selElementSelGS;
        GS[gsIndex++] = _tokens->selWireOffsetGS;
    } else {
        GS[gsIndex++] = _tokens->selWireNoOffsetGS;
    }

    // Displacement shading can be disabled explicitly, or if the entrypoint
    // doesn't exist (resolved in HdStMesh).
    GS[gsIndex++] = (!hasCustomDisplacement) ?
        _tokens->noCustomDisplacementGS :
        _tokens->customDisplacementGS;

    GS[gsIndex++] = isPrimTypeQuads
                        ? _tokens->mainQuadGS
                        : isPrimTypeTriQuads
                            ? _tokens->mainTriQuadGS
                            : isPrimTypePatches
                                ? _tokens->mainTriangleTessGS
                                    : _tokens->mainTriangleGS;
    GS[gsIndex] = TfToken();

    // Optimization : See if we can skip the geometry shader.
    bool const canSkipGS =
            ptvsStageEnabled ||
            // Whether we can skip executing the displacement shading terminal
            (!hasCustomDisplacement
            && (normalsSource != NormalSourceLimit)
            && (normalsSource != NormalSourceFlatGeometric)
            // whether we can skip splitting quads to triangles
            && (isPrimTypeTris ||
                isPrimTypeTriQuads)
            // whether we can skip generating coords for edges
            && ((!renderWireframe && !renderEdges && !renderSelectedEdges) ||
                hasFragmentShaderBarycentrics)
            // whether we can skip generating coords for per-face interpolation
            && (!hasPerFaceInterpolation ||
                hasFragmentShaderBarycentrics))
            ;
            
    if (canSkipGS) {
        GS[0] = TfToken();
    }

    // Optimization : Points don't need any sort of geometry shader so
    //                we ignore it here.
    if (isPrimTypePoints) {
        GS[0] = TfToken();
    }

    bool const gsStageEnabled = !GS[0].IsEmpty();

    // fragment shader
    uint8_t fsIndex = 0;
    FS[fsIndex++] = _tokens->instancing;

    FS[fsIndex++] =
        (normalsSource == NormalSourceFlatScreenSpace)
            ? _tokens->normalsScreenSpaceFS
            : (!gsStageEnabled && normalsSource == NormalSourceFlat)
                ? _tokens->normalsFlat
                : ((!gsStageEnabled && gsSceneNormals)
                    ? _tokens->normalsScene
                    : _tokens->normalsPass);

    FS[fsIndex++] = doubleSided ?
        _tokens->normalsDoubleSidedFS : _tokens->normalsSingleSidedFS;

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
            ? _tokens->faceCullFrontFacingFS
            : faceCullBackFacing
                ? _tokens->faceCullBackFacingFS
                : _tokens->faceCullNoneFS; // DontCare, Nothing, HW

    // Wire (edge) related mixins
    if (renderWireframe || renderEdges) {
        if (isPrimTypeTris && ptvsStageEnabled) {
            FS[fsIndex++] = _tokens->edgeCoordTessCoordTriangleFS;
        } else if ((isPrimTypeQuads||isPrimTypeTriQuads) && ptvsStageEnabled) {
            FS[fsIndex++] = _tokens->edgeCoordTessCoordFS;
        } else {
            FS[fsIndex++] = _tokens->edgeCoordBarycentricCoordFS;
        }

        if (isPrimTypeRefinedMesh) {
            if (isPrimTypeQuads || isPrimTypeTriQuads) {
                FS[fsIndex++] = _tokens->edgeMaskRefinedQuadFS;
            } else {
                FS[fsIndex++] = _tokens->edgeMaskNoneFS;
            }
        } else if (isPrimTypeTris) {
            FS[fsIndex++] = _tokens->edgeMaskTriangleFS;
        } else if (isPrimTypeTriQuads) {
            FS[fsIndex++] = _tokens->edgeMaskTriQuadFS;
        } else {
            FS[fsIndex++] = _tokens->edgeMaskQuadFS;
        }

        if (isPrimTypeTriQuads && ptvsStageEnabled) {
            FS[fsIndex++] = _tokens->edgeTriQuadPTVSFS;
        } else {
            FS[fsIndex++] = _tokens->edgeCommonFS;
        }
        FS[fsIndex++] = _tokens->edgeParamFS;

        if (forceOpaqueEdges) {
            FS[fsIndex++] = _tokens->edgeOpacityForceFS;
        } else {
            FS[fsIndex++] = _tokens->edgeOpacityNoForceFS;
        }

        if (renderWireframe) {
            if (isPrimTypePatches) {
                FS[fsIndex++] = _tokens->patchEdgeOnlyFS;
            } else {
                FS[fsIndex++] = blendWireframeColor
                                    ? _tokens->edgeOnlyBlendFS
                                    : _tokens->edgeOnlyNoBlendFS;
            }
        } else {
            if (isPrimTypeTris || isPrimTypePatchesBoxSplineTriangle) {
                FS[fsIndex++] = _tokens->patchEdgeTriangleFS;
            } else {
                FS[fsIndex++] = _tokens->patchEdgeQuadFS;
            }
            if (isPrimTypeRefinedMesh) {
                FS[fsIndex++] = _tokens->patchEdgeOnSurfFS;
            } else {
                FS[fsIndex++] = _tokens->edgeOnSurfFS;
            }
        }
    } else {
        FS[fsIndex++] = _tokens->edgeNoneFS;
        FS[fsIndex++] = _tokens->edgeParamFS;
    }

    // Shading terminal mixin
    TfToken terminalFS;
    if (shadingTerminal == HdMeshReprDescTokens->surfaceShader) {
        terminalFS = _tokens->surfaceFS;
    } else if (shadingTerminal == HdMeshReprDescTokens->surfaceShaderUnlit) {
        terminalFS = _tokens->surfaceUnlitFS;
    } else if (shadingTerminal == HdMeshReprDescTokens->surfaceShaderSheer) {
        terminalFS = _tokens->surfaceSheerFS;
    } else if (shadingTerminal == HdMeshReprDescTokens->surfaceShaderOutline) {
        terminalFS = _tokens->surfaceOutlineFS;
    } else if (shadingTerminal == HdMeshReprDescTokens->constantColor) {
        terminalFS = _tokens->constantColorFS;
    } else if (shadingTerminal == HdMeshReprDescTokens->hullColor) {
        terminalFS = _tokens->hullColorFS;
    } else if (shadingTerminal == HdMeshReprDescTokens->pointColor) {
        if (pointsShadingEnabled) {
            // Let points be affected by the associated material so as to appear
            // coherent with the other shaded surfaces that may be part of this
            // mesh.
            terminalFS = _tokens->pointShadedFS;
        } else {
            terminalFS = _tokens->pointColorFS;
        }
    } else if (!shadingTerminal.IsEmpty()) {
        terminalFS = shadingTerminal;
    } else {
        terminalFS = _tokens->surfaceFS;
    }

    // Common must be first as it defines terminal interfaces
    FS[fsIndex++] = _tokens->commonFS;
    FS[fsIndex++] = terminalFS;

    FS[fsIndex++] =  enableScalarOverride ? _tokens->scalarOverrideFS :
                                            _tokens->noScalarOverrideFS;

    // EdgeId mixin(s) for edge picking and selection
    // Note: When rendering a mesh as points, we handle this in code gen.
    if (!isPrimTypePoints) {
        FS[fsIndex++] = _tokens->edgeIdCommonFS;
        if (isPrimTypeTris || isPrimTypePatchesBoxSplineTriangle) {
            if (polygonMode == HdPolygonModeLine) {
                FS[fsIndex++] = _tokens->edgeIdTriangleLineFS;
            } else {
                FS[fsIndex++] = _tokens->edgeIdTriangleSurfFS;
            }
            FS[fsIndex++] = _tokens->edgeIdTriangleParamFS;
        } else {
            if (polygonMode == HdPolygonModeLine) {
                FS[fsIndex++] = _tokens->edgeIdQuadLineFS;
            } else {
                FS[fsIndex++] = _tokens->edgeIdQuadSurfFS;
            }
            FS[fsIndex++] = _tokens->edgeIdQuadParamFS;
        }
    }

    // PointId mixin for point picking and selection
    FS[fsIndex++] = isPrimTypePoints? _tokens->pointIdFS :
                                      _tokens->pointIdFallbackFS;
    FS[fsIndex++] = hasTopologicalVisibility? _tokens->topVisFS :
                                              _tokens->topVisFallbackFS;

    // Triangles
    if (isPrimTypeTris && ptvsStageEnabled) {
        FS[fsIndex++] = _tokens->mainPatchCoordTrianglePTVSFS;
    } else if (isPrimTypeTris && !gsStageEnabled) {
        FS[fsIndex++] = _tokens->mainPatchCoordTriangleFS;

    // Quads
    } else if (isPrimTypeQuads && ptvsStageEnabled) {
        FS[fsIndex++] = _tokens->mainPatchCoordQuadPTVSFS;
    } else if (isPrimTypeQuads && !gsStageEnabled) {
        FS[fsIndex++] = _tokens->mainPatchCoordQuadFS;

    // TriQuads
    } else if (isPrimTypeTriQuads && ptvsStageEnabled) {
        FS[fsIndex++] = _tokens->mainPatchCoordTriQuadPTVSFS;
    } else if (isPrimTypeTriQuads) {
        FS[fsIndex++] = _tokens->mainPatchCoordTriQuadFS;

    // Patches
    } else if (isPrimTypePatches && ptvsStageEnabled) {
        FS[fsIndex++] = _tokens->mainPatchCoordTessFS;
    // Points/No GS
    } else if (isPrimTypePoints || canSkipGS) {
        FS[fsIndex++] = _tokens->mainPatchCoordNoGSFS;
    } else {
        FS[fsIndex++] = _tokens->mainPatchCoordFS;
    }

    FS[fsIndex++] = _tokens->mainFS;
    FS[fsIndex] = TfToken();
}

HdSt_MeshShaderKey::~HdSt_MeshShaderKey()
{
}

PXR_NAMESPACE_CLOSE_SCOPE

