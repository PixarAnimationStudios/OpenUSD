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

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hdSt/meshShaderKey.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX,              "mesh.glslfx"))

    // normal mixins
    ((smooth,                  "MeshNormal.Smooth"))
    ((flat,                    "MeshNormal.Flat"))
    ((limit,                   "MeshNormal.Limit"))

    ((doubleSidedFS,           "MeshNormal.Fragment.DoubleSided"))
    ((singleSidedFS,           "MeshNormal.Fragment.SingleSided"))

    // wireframe mixins
    ((edgeNoneGS,              "MeshWire.Geometry.NoEdge"))
    ((edgeNoneFS,              "MeshWire.Fragment.NoEdge"))

    ((edgeOnlyGS,              "MeshWire.Geometry.Edge"))
    ((edgeOnlyBlendFS,         "MeshWire.Fragment.EdgeOnlyBlendColor"))
    ((edgeOnlyNoBlendFS,       "MeshWire.Fragment.EdgeOnlyNoBlend"))

    ((edgeOnSurfGS,            "MeshWire.Geometry.Edge"))
    ((edgeOnSurfFS,            "MeshWire.Fragment.EdgeOnSurface"))
    ((patchEdgeOnlyFS,         "MeshPatchWire.Fragment.EdgeOnly"))
    ((patchEdgeOnSurfFS,       "MeshPatchWire.Fragment.EdgeOnSurface"))

    // edge id mixins (for edge picking & selection)
    ((edgeIdNoneGS,            "EdgeId.Geometry.None"))
    ((edgeIdEdgeParamGS,       "EdgeId.Geometry.EdgeParam"))
    ((edgeIdFallbackFS,        "EdgeId.Fragment.Fallback"))
    ((edgeIdTriangleParamFS,   "EdgeId.Fragment.TriangleParam"))
    ((edgeIdRectangleParamFS,  "EdgeId.Fragment.RectangleParam"))

    // point id mixins (for point picking & selection)
    ((pointIdVS,               "PointId.Vertex"))
    ((pointIdFS,               "PointId.Fragment.Points"))
    ((pointIdFallbackFS,       "PointId.Fragment.Fallback"))

    // main for all the shader stages
    ((mainVS,                  "Mesh.Vertex"))
    ((mainBSplineTCS,          "Mesh.TessControl.BSpline"))
    ((mainBezierTES,           "Mesh.TessEval.Bezier"))
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
);

HdSt_MeshShaderKey::HdSt_MeshShaderKey(
    HdSt_GeometricShader::PrimitiveType primitiveType,
    TfToken shadingTerminal,
    bool useCustomDisplacement,
    bool smoothNormals,
    bool doubleSided,
    bool faceVarying,
    bool blendWireframeColor,
    HdCullStyle cullStyle,
    HdMeshGeomStyle geomStyle,
    float lineWidth)
    : primType(primitiveType)
    , cullStyle(cullStyle)
    , polygonMode(HdPolygonModeFill)
    , lineWidth(lineWidth)
    , isFaceVarying(faceVarying)
    , glslfx(_tokens->baseGLSLFX)
{
    if (geomStyle == HdMeshGeomStyleEdgeOnly ||
        geomStyle == HdMeshGeomStyleHullEdgeOnly) {
        polygonMode = HdPolygonModeLine;
    }

    // vertex shader
    VS[0] = _tokens->instancing;
    VS[1] = (smoothNormals ? _tokens->smooth
                           : _tokens->flat);
    VS[2] = _tokens->pointIdVS;
    VS[3] = _tokens->mainVS;
    VS[4] = TfToken();

    // tessellation control shader
    const bool isPrimTypePatches = 
        HdSt_GeometricShader::IsPrimTypePatches(primType);

    TCS[0] = isPrimTypePatches ? _tokens->instancing : TfToken();
    TCS[1] = isPrimTypePatches ? _tokens->mainBSplineTCS : TfToken();
    TCS[2] = TfToken();

    // tessellation evaluation shader
    TES[0] = isPrimTypePatches ? _tokens->instancing : TfToken();
    TES[1] = isPrimTypePatches ? _tokens->mainBezierTES : TfToken();
    TES[2] = TfToken();

    // geometry shader (note that PRIM_MESH_PATCHES uses triangles)
    GS[0] = _tokens->instancing;
    GS[1] = isPrimTypePatches ? _tokens->limit : 
                (smoothNormals ? _tokens->smooth : _tokens->flat);
    GS[2] = ((geomStyle == HdMeshGeomStyleEdgeOnly ||
              geomStyle == HdMeshGeomStyleHullEdgeOnly)   ? _tokens->edgeOnlyGS
           : (geomStyle == HdMeshGeomStyleEdgeOnSurf ||
              geomStyle == HdMeshGeomStyleHullEdgeOnSurf) ? _tokens->edgeOnSurfGS
                                                          : _tokens->edgeNoneGS);

    bool isPrimTypePoints = HdSt_GeometricShader::IsPrimTypePoints(primType);
    bool isPrimTypeQuads = HdSt_GeometricShader::IsPrimTypeQuads(primType);
    bool isPrimTypeTris = HdSt_GeometricShader::IsPrimTypeTriangles(primType);

    // emit edge param per vertex to help compute the edgeId
    TfToken gsEdgeIdMixin = isPrimTypePoints ? _tokens->edgeIdNoneGS
                                             : _tokens->edgeIdEdgeParamGS;
    GS[3] = gsEdgeIdMixin;

    // Displacement shading can be disabled explicitly, or if the entrypoint
    // doesn't exist (resolved in HdStMesh).
    GS[4] = (!useCustomDisplacement) ?
        _tokens->noCustomDisplacementGS :
        _tokens->customDisplacementGS;

    GS[5] = isPrimTypeQuads? _tokens->mainQuadGS :
                (isPrimTypePatches ? _tokens->mainTriangleTessGS
                                   : _tokens->mainTriangleGS);
    GS[6] = TfToken();

    // Optimization : If the mesh is skipping displacement shading, we have an
    // opportunity to fully disable the geometry stage.
    if (!useCustomDisplacement
            && smoothNormals
            && (geomStyle == HdMeshGeomStyleSurf || geomStyle == HdMeshGeomStyleHull)
            && HdSt_GeometricShader::IsPrimTypeTriangles(primType)
            && (!isFaceVarying)) {
            
        GS[0] = TfToken();
    }

    // Optimization : Points don't need any sort of geometry shader so
    //                we ignore it here.
    if (isPrimTypePoints) {
        GS[0] = TfToken();
    }

    // fragment shader
    FS[0] = _tokens->instancing;
    FS[1] = (smoothNormals ? _tokens->smooth
                           : _tokens->flat);
    FS[2] = (doubleSided   ? _tokens->doubleSidedFS
                           : _tokens->singleSidedFS);
    if (isPrimTypePatches) {
        FS[3] = ((geomStyle == HdMeshGeomStyleEdgeOnly ||
                  geomStyle == HdMeshGeomStyleHullEdgeOnly)   ? _tokens->patchEdgeOnlyFS
              : ((geomStyle == HdMeshGeomStyleEdgeOnSurf ||
                  geomStyle == HdMeshGeomStyleHullEdgeOnSurf) ? _tokens->patchEdgeOnSurfFS
                                                              : _tokens->edgeNoneFS));
    } else {
        if (geomStyle == HdMeshGeomStyleEdgeOnly ||
            geomStyle == HdMeshGeomStyleHullEdgeOnly) {
            FS[3] = blendWireframeColor ? _tokens->edgeOnlyBlendFS
                                        : _tokens->edgeOnlyNoBlendFS;
        } else if (geomStyle == HdMeshGeomStyleEdgeOnSurf ||
                   geomStyle == HdMeshGeomStyleHullEdgeOnSurf) {
            FS[3] = _tokens->edgeOnSurfFS;
        } else {
            FS[3] = _tokens->edgeNoneFS;
        }
    }

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

    FS[4] = terminalFS;
    FS[5] = _tokens->commonFS;

    // edge id
    uint8_t fsIndex = 6;
    if (!GS[0].IsEmpty()) {
        TF_VERIFY(gsEdgeIdMixin == _tokens->edgeIdEdgeParamGS);
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

    FS[fsIndex++] = isPrimTypePoints? _tokens->pointIdFS :
                                      _tokens->pointIdFallbackFS;
    FS[fsIndex++] = _tokens->mainFS;
    FS[fsIndex] = TfToken();
}

HdSt_MeshShaderKey::~HdSt_MeshShaderKey()
{
}

PXR_NAMESPACE_CLOSE_SCOPE

