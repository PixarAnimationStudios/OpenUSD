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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/meshShaderKey.h"
#include "pxr/base/tf/staticTokens.h"

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX,       "mesh.glslfx"))
    ((smooth,           "MeshNormal.Smooth"))
    ((flat,             "MeshNormal.Flat"))
    ((limit,            "MeshNormal.Limit"))
    ((doubleSidedFS,    "MeshNormal.Fragment.DoubleSided"))
    ((singleSidedFS,    "MeshNormal.Fragment.SingleSided"))
    ((edgeNoneGS,       "MeshWire.Geometry.NoEdge"))
    ((edgeNoneFS,       "MeshWire.Fragment.NoEdge"))
    ((edgeOnlyGS,       "MeshWire.Geometry.Edge"))
    ((edgeOnlyFS,       "MeshWire.Fragment.EdgeOnly"))
    ((edgeOnSurfGS,     "MeshWire.Geometry.Edge"))
    ((edgeOnSurfFS,     "MeshWire.Fragment.EdgeOnSurface"))
    ((patchEdgeOnlyFS,  "MeshPatchWire.Fragment.EdgeOnly"))
    ((patchEdgeOnSurfFS,"MeshPatchWire.Fragment.EdgeOnSurface"))
    ((mainVS,           "Mesh.Vertex"))
    ((mainBSplineTCS,   "Mesh.TessControl.BSpline"))
    ((mainBezierTES,    "Mesh.TessEval.Bezier"))
    ((mainTriangleGS,   "Mesh.Geometry.Triangle"))
    ((mainQuadGS,       "Mesh.Geometry.Quad"))
    ((litFS,            "Mesh.Fragment.Lit"))
    ((unlitFS,          "Mesh.Fragment.Unlit"))
    ((mainFS,           "Mesh.Fragment"))
    ((instancing,       "Instancing.Transform"))
);

Hd_MeshShaderKey::Hd_MeshShaderKey(
    GLenum primType,
    bool lit,
    bool smoothNormals,
    bool doubleSided,
    bool faceVarying,
    HdCullStyle cullStyle,
    HdMeshGeomStyle geomStyle)
    : primitiveMode(primType)
    , cullStyle(cullStyle)
    , polygonMode(HdPolygonModeFill)
    , glslfx(_tokens->baseGLSLFX)
{
    if (primType == GL_POINTS) {
        primitiveIndexSize = 1;
    } else if (primType == GL_TRIANGLES) {
        primitiveIndexSize = 3;
    } else if (primType == GL_LINES_ADJACENCY) {
        primitiveIndexSize = 4;
    } else if (primType == GL_PATCHES) {
        primitiveIndexSize = 16;
    } else {
        TF_CODING_ERROR("Unknown primitiveType %d\n", primType);
        primitiveIndexSize = 1;
    }

    if (geomStyle == HdMeshGeomStyleEdgeOnly or
        geomStyle == HdMeshGeomStyleHullEdgeOnly) {
        polygonMode = HdPolygonModeLine;
    }

    // vertex shader
    VS[0] = _tokens->instancing;
    VS[1] = (smoothNormals ? _tokens->smooth
                           : _tokens->flat);
    VS[2] = _tokens->mainVS;
    VS[3] = TfToken();

    // tessellation control shader
    TCS[0] = (primType == GL_PATCHES ? _tokens->instancing : TfToken());
    TCS[1] = (primType == GL_PATCHES ? _tokens->mainBSplineTCS : TfToken());
    TCS[2] = TfToken();

    // tessellation evaluation shader
    TES[0] = (primType == GL_PATCHES ? _tokens->instancing : TfToken());
    TES[1] = (primType == GL_PATCHES ? _tokens->mainBezierTES : TfToken());
    TES[2] = TfToken();

    // geometry shader (note that GL_PATCHES uses triangles)
    GS[0] = _tokens->instancing;
    GS[1] = (primType == GL_PATCHES ? _tokens->limit
                                    : (smoothNormals ? _tokens->smooth
                                                     : _tokens->flat));
    GS[2] = ((geomStyle == HdMeshGeomStyleEdgeOnly or
              geomStyle == HdMeshGeomStyleHullEdgeOnly)   ? _tokens->edgeOnlyGS
           : (geomStyle == HdMeshGeomStyleEdgeOnSurf or
              geomStyle == HdMeshGeomStyleHullEdgeOnSurf) ? _tokens->edgeOnSurfGS
                                                          : _tokens->edgeNoneGS);
    GS[3] = (primType == GL_LINES_ADJACENCY ? _tokens->mainQuadGS
                                            : _tokens->mainTriangleGS);
    GS[4] = TfToken();

    // mesh special optimization:
    //    there are some cases that we can skip the geometry shader.
    if (smoothNormals
        and (geomStyle == HdMeshGeomStyleSurf or geomStyle == HdMeshGeomStyleHull)
        and primType == GL_TRIANGLES
        and (not faceVarying)) {
        GS[0] = TfToken();
    }
    if (primType == GL_POINTS) {
        GS[0] = TfToken();
    }

    // fragment shader
    FS[0] = _tokens->instancing;
    FS[1] = (smoothNormals ? _tokens->smooth
                           : _tokens->flat);
    FS[2] = (doubleSided   ? _tokens->doubleSidedFS
                           : _tokens->singleSidedFS);
    if (primType == GL_PATCHES) {
        FS[3] = ((geomStyle == HdMeshGeomStyleEdgeOnly or
                  geomStyle == HdMeshGeomStyleHullEdgeOnly)   ? _tokens->patchEdgeOnlyFS
              : ((geomStyle == HdMeshGeomStyleEdgeOnSurf or
                  geomStyle == HdMeshGeomStyleHullEdgeOnSurf) ? _tokens->patchEdgeOnSurfFS
                                                              : _tokens->edgeNoneFS));
    } else {
        FS[3] = ((geomStyle == HdMeshGeomStyleEdgeOnly or
                  geomStyle == HdMeshGeomStyleHullEdgeOnly)   ? _tokens->edgeOnlyFS
              : ((geomStyle == HdMeshGeomStyleEdgeOnSurf or
                  geomStyle == HdMeshGeomStyleHullEdgeOnSurf) ? _tokens->edgeOnSurfFS
                                                              : _tokens->edgeNoneFS));
    }
    FS[4] = lit ? _tokens->litFS : _tokens->unlitFS;
    FS[5] = _tokens->mainFS;
    FS[6] = TfToken();
}

Hd_MeshShaderKey::~Hd_MeshShaderKey()
{
}
