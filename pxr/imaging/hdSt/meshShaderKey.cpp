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
    ((baseGLSLFX,              "mesh.glslfx"))

    // normal mixins
    ((normalsScene,            "MeshNormal.Scene"))
    ((normalsSmooth,           "MeshNormal.Smooth"))
    ((normalsFlat,             "MeshNormal.Flat"))
    ((normalsPass,             "MeshNormal.Pass"))

    ((normalsGeometryFlat,     "MeshNormal.Geometry.Flat"))
    ((normalsGeometryNoFlat,   "MeshNormal.Geometry.NoFlat"))

    ((normalsDoubleSidedFS,    "MeshNormal.Fragment.DoubleSided"))
    ((normalsSingleSidedFS,    "MeshNormal.Fragment.SingleSided"))

    ((faceCullHardwareFS,      "MeshFaceCull.Fragment.None"))
    ((faceCullSingleSidedFS,   "MeshFaceCull.Fragment.SingleSided"))
    ((faceCullDoubleSidedFS,   "MeshFaceCull.Fragment.DoubleSided"))

    // wireframe mixins
    ((edgeNoneGS,              "MeshWire.Geometry.NoEdge"))
    ((edgeNoneFS,              "MeshWire.Fragment.NoEdge"))
    
    ((edgeCommonFS,            "MeshWire.Fragment.EdgeCommon"))

    ((edgeOnlyGS,              "MeshWire.Geometry.Edge"))
    ((edgeOnlyBlendFS,         "MeshWire.Fragment.EdgeOnlyBlendColor"))
    ((edgeOnlyNoBlendFS,       "MeshWire.Fragment.EdgeOnlyNoBlend"))

    ((edgeOnSurfGS,            "MeshWire.Geometry.Edge"))
    ((edgeOnSurfFS,            "MeshWire.Fragment.EdgeOnSurface"))
    ((patchEdgeOnlyFS,         "MeshPatchWire.Fragment.EdgeOnly"))
    ((patchEdgeOnSurfFS,       "MeshPatchWire.Fragment.EdgeOnSurface"))

    ((selWireOffsetGS,         "Selection.Geometry.WireSelOffset"))
    ((selWireNoOffsetGS,       "Selection.Geometry.WireSelNoOffset"))
    
    // selection decoding
    ((selDecodeUtils,          "Selection.DecodeUtils"))
    ((selPointSelVS,           "Selection.Vertex.PointSel"))
    ((selElementSelGS,         "Selection.Geometry.ElementSel"))

    // edge id mixins (for edge picking & selection)
    ((edgeIdNoneGS,            "EdgeId.Geometry.None"))
    ((edgeIdEdgeParamGS,       "EdgeId.Geometry.EdgeParam"))
    ((edgeIdFallbackFS,        "EdgeId.Fragment.Fallback"))
    ((edgeIdCommonFS,          "EdgeId.Fragment.Common"))
    ((edgeIdTriangleParamFS,   "EdgeId.Fragment.TriangleParam"))
    ((edgeIdRectangleParamFS,  "EdgeId.Fragment.RectangleParam"))

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
    ((mainQuadGS,              "Mesh.Geometry.Quad"))
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

HdSt_MeshShaderKey::HdSt_MeshShaderKey(
    HdSt_GeometricShader::PrimitiveType primitiveType,
    TfToken shadingTerminal,
    bool useCustomDisplacement,
    NormalSource normalsSource,
    HdInterpolation normalsInterpolation,
    bool doubleSided,
    bool forceGeometryShader,
    bool hasTopologicalVisibility,
    bool blendWireframeColor,
    HdCullStyle cullStyle,
    HdMeshGeomStyle geomStyle,
    float lineWidth,
    bool hasMirroredTransform,
    bool hasInstancer,
    bool enableScalarOverride)
    : primType(primitiveType)
    , cullStyle(cullStyle)
    , hasMirroredTransform(hasMirroredTransform)
    , doubleSided(doubleSided)
    , polygonMode(HdPolygonModeFill)
    , lineWidth(lineWidth)
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
    bool isPrimTypeQuads  = HdSt_GeometricShader::IsPrimTypeQuads(primType);
    bool isPrimTypeTris   = HdSt_GeometricShader::IsPrimTypeTriangles(primType);
    const bool isPrimTypePatches = 
        HdSt_GeometricShader::IsPrimTypePatches(primType);
    const bool isPrimTypePatchesBSpline = 
        primType ==
            HdSt_GeometricShader::PrimitiveType::PRIM_MESH_BSPLINE;

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
     */
    bool vsSceneNormals =
        (normalsSource == NormalSourceScene &&
        normalsInterpolation != HdInterpolationUniform &&
        normalsInterpolation != HdInterpolationFaceVarying);
    bool gsSceneNormals =
        (normalsSource == NormalSourceScene && !vsSceneNormals);

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

    // tessellation control shader
    TCS[0] = isPrimTypePatches ? _tokens->instancing : TfToken();
    TCS[1] = isPrimTypePatches ? isPrimTypePatchesBSpline
                                   ? _tokens->mainBSplineQuadTCS
                                   : _tokens->mainBoxSplineTriangleTCS
                               : TfToken();
    TCS[2] = TfToken();

    // tessellation evaluation shader
    TES[0] = isPrimTypePatches ? _tokens->instancing : TfToken();
    TES[1] = isPrimTypePatches ? isPrimTypePatchesBSpline
                                   ? _tokens->mainBezierQuadTES
                                   : _tokens->mainBezierTriangleTES
                               : TfToken();
    TES[2] = isPrimTypePatches ? _tokens->mainVaryingInterpTES : TfToken();
    TES[3] = TfToken();

    // geometry shader
    uint8_t gsIndex = 0;
    GS[gsIndex++] = _tokens->instancing;

    GS[gsIndex++] = (normalsSource == NormalSourceFlat) ?
        _tokens->normalsFlat :
        (gsSceneNormals ? _tokens->normalsScene : _tokens->normalsPass);

    GS[gsIndex++] = (normalsSource == NormalSourceGeometryShader) ?
            _tokens->normalsGeometryFlat : _tokens->normalsGeometryNoFlat;

    GS[gsIndex++] = ((geomStyle == HdMeshGeomStyleEdgeOnly ||
                      geomStyle == HdMeshGeomStyleHullEdgeOnly)
                        ? _tokens->edgeOnlyGS
                        : (geomStyle == HdMeshGeomStyleEdgeOnSurf ||
                           geomStyle == HdMeshGeomStyleHullEdgeOnSurf)
                            ? _tokens->edgeOnSurfGS
                            : _tokens->edgeNoneGS);

    // emit edge param per vertex to help compute the edgeId
    TfToken gsEdgeIdMixin = isPrimTypePoints ? _tokens->edgeIdNoneGS
                                             : _tokens->edgeIdEdgeParamGS;
    GS[gsIndex++] = gsEdgeIdMixin;

    const bool renderWireframe = geomStyle == HdMeshGeomStyleEdgeOnly ||
                                 geomStyle == HdMeshGeomStyleHullEdgeOnly;    
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
    GS[gsIndex++] = (!useCustomDisplacement) ?
        _tokens->noCustomDisplacementGS :
        _tokens->customDisplacementGS;

    GS[gsIndex++] = isPrimTypeQuads ? _tokens->mainQuadGS :
                    (isPrimTypePatches ? _tokens->mainTriangleTessGS
                                       : _tokens->mainTriangleGS);
    GS[gsIndex] = TfToken();

    // Optimization : If the mesh is skipping displacement shading, we have an
    // opportunity to fully disable the geometry stage.
    if (!useCustomDisplacement
            && (normalsSource != NormalSourceLimit)
            && (normalsSource != NormalSourceGeometryShader)
            && (geomStyle == HdMeshGeomStyleSurf || geomStyle == HdMeshGeomStyleHull)
            && HdSt_GeometricShader::IsPrimTypeTriangles(primType)
            && (!forceGeometryShader)) {
            
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
    FS[fsIndex++] = _tokens->instancing;

    FS[fsIndex++] = (!gsStageEnabled && normalsSource == NormalSourceFlat) ?
        _tokens->normalsFlat :
        ((!gsStageEnabled && gsSceneNormals) ? _tokens->normalsScene :
         _tokens->normalsPass);

    FS[fsIndex++] = doubleSided ?
        _tokens->normalsDoubleSidedFS : _tokens->normalsSingleSidedFS;

    FS[fsIndex++] = useHardwareFaceCulling ? _tokens->faceCullHardwareFS :
                    (doubleSided ? _tokens->faceCullDoubleSidedFS :
                                   _tokens->faceCullSingleSidedFS);

    // Wire (edge) related mixins
    if ((geomStyle == HdMeshGeomStyleEdgeOnly ||
         geomStyle == HdMeshGeomStyleHullEdgeOnly)) {

        FS[fsIndex++] = _tokens->edgeCommonFS;
        if (isPrimTypePatches) {
            FS[fsIndex++] = _tokens->patchEdgeOnlyFS;
        } else {
            FS[fsIndex++] = blendWireframeColor ? _tokens->edgeOnlyBlendFS
                                        : _tokens->edgeOnlyNoBlendFS;
        }

    } else if ((geomStyle == HdMeshGeomStyleEdgeOnSurf ||
                geomStyle == HdMeshGeomStyleHullEdgeOnSurf)) {

        FS[fsIndex++] = _tokens->edgeCommonFS;
        if (isPrimTypePatches) {
            FS[fsIndex++] = _tokens->patchEdgeOnSurfFS;
        } else {
            FS[fsIndex++] = _tokens->edgeOnSurfFS;
        }

    } else {
        FS[fsIndex++] = _tokens->edgeNoneFS;
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
        terminalFS = _tokens->pointColorFS;
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
    if (gsStageEnabled) {
        TF_VERIFY(gsEdgeIdMixin == _tokens->edgeIdEdgeParamGS);
        FS[fsIndex++] = _tokens->edgeIdCommonFS;
        if (isPrimTypeTris) {
            // coarse and refined triangles and triangular parametric patches
            FS[fsIndex++] = _tokens->edgeIdTriangleParamFS;
        } else {
            // coarse and refined quads and rectangular parametric patches
            FS[fsIndex++] = _tokens->edgeIdRectangleParamFS;
        }
    } else {
        // the GS stage is skipped if we're dealing with points or triangles.
        // (see "Optimization" above)

        // for triangles, emit the fallback version.
        if (isPrimTypeTris) {
            FS[fsIndex++] = _tokens->edgeIdFallbackFS;
        }

        // for points, it isn't so simple. we don't know if the 'edgeIndices'
        // buffer was bound.
        // if the points repr alone is used, then it won't be generated.
        // (see GetPointsIndexBuilderComputation)
        // if any other *IndexBuilderComputation was used, and we then use the
        // points repr, the binding will exist.
        // we handle this scenario in hdStCodeGen since it has the binding info.
    }

    // PointId mixin for point picking and selection
    FS[fsIndex++] = isPrimTypePoints? _tokens->pointIdFS :
                                      _tokens->pointIdFallbackFS;
    FS[fsIndex++] = hasTopologicalVisibility? _tokens->topVisFS :
                                              _tokens->topVisFallbackFS;
    FS[fsIndex++] = _tokens->mainFS;
    FS[fsIndex] = TfToken();
}

HdSt_MeshShaderKey::~HdSt_MeshShaderKey()
{
}

PXR_NAMESPACE_CLOSE_SCOPE

