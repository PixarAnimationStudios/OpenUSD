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

#include "pxr/usdImaging/usdImagingGL/legacyEngine.h"

#include "pxr/usdImaging/usdImaging/cubeAdapter.h"
#include "pxr/usdImaging/usdImaging/sphereAdapter.h"
#include "pxr/usdImaging/usdImaging/coneAdapter.h"
#include "pxr/usdImaging/usdImaging/cylinderAdapter.h"
#include "pxr/usdImaging/usdImaging/capsuleAdapter.h"
#include "pxr/usdImaging/usdImaging/nurbsPatchAdapter.h"

#include "pxr/usd/usd/debugCodes.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/primRange.h"

// Geometry Schema
#include "pxr/usd/usdGeom/scope.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/basisCurves.h"
#include "pxr/usd/usdGeom/nurbsCurves.h"
#include "pxr/usd/usdGeom/cube.h"
#include "pxr/usd/usdGeom/sphere.h"
#include "pxr/usd/usdGeom/cone.h"
#include "pxr/usd/usdGeom/cylinder.h"
#include "pxr/usd/usdGeom/capsule.h"
#include "pxr/usd/usdGeom/points.h"
#include "pxr/usd/usdGeom/nurbsPatch.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/trace/trace.h"

#include "pxr/imaging/hdx/intersector.h"

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/info.h"

// Mesh Topology
#include "pxr/imaging/hd/meshTopology.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/gamma.h"
#include "pxr/base/tf/stl.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

struct _HitData {
    int xMin;
    int yMin;
    double zMin;
    int minIndex;
};
typedef TfHashMap< int32_t, _HitData > _HitDataById;

}

// Sentinel value for prim restarts, so that multiple prims can be lumped into a
// single draw call, if the hardware supports it.
#define _PRIM_RESTART_INDEX 0xffffffff 

UsdImagingGLLegacyEngine::UsdImagingGLLegacyEngine(const SdfPathVector &excludedPrimPaths) :
    _ctm(GfMatrix4d(1.0)),
    _vertCount(0),
    _lineVertCount(0),
    _attribBuffer(0),
    _indexBuffer(0)
{
    // Build a TfHashSet of excluded prims for fast rejection.
    TF_FOR_ALL(pathIt, excludedPrimPaths) {
        _excludedSet.insert(*pathIt);
    }
}

UsdImagingGLLegacyEngine::~UsdImagingGLLegacyEngine()
{
    TfNotice::Revoke(_objectsChangedNoticeKey);
    InvalidateBuffers();
}

bool
UsdImagingGLLegacyEngine::_SupportsPrimitiveRestartIndex()
{
    static bool supported = GlfHasExtensions("GL_NV_primitive_restart");
    return supported;
}

void
UsdImagingGLLegacyEngine::InvalidateBuffers()
{
    TRACE_FUNCTION();

    if (!_attribBuffer) {
        return;
    }

    // There is no sensible configuration that would have an attribBuffer but
    // not an indexBuffer.
    if (!TF_VERIFY(_indexBuffer)) {
        return;
    }

    // Make sure that a shared context is current while we're deleting.
    GlfSharedGLContextScopeHolder sharedGLContextScopeHolder;

    // Check that we are bound to the correct GL context; otherwise the
    // glDeleteBuffers() calls below will have no effect and we'll leak the
    // memory in these buffers (bug 34014).
    TF_VERIFY(glIsBuffer(_attribBuffer));
    TF_VERIFY(glIsBuffer(_indexBuffer));

    glDeleteBuffers(1, &_attribBuffer);
    glDeleteBuffers(1, &_indexBuffer);

    _attribBuffer = 0;
    _indexBuffer = 0;
}

template<class E, class T>
static void
_AppendSubData(GLenum target, GLintptr* offset, T const& vec)
{
    glBufferSubData(target, *offset, sizeof(E)*vec.size(), &(vec[0]));
    *offset += sizeof(E)*vec.size();
}

void
UsdImagingGLLegacyEngine::_PopulateBuffers()
{
    glGenBuffers(1, &_attribBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _attribBuffer);

    GLintptr offset = 0;

    // The array buffer contains the raw floats for the points, normals, and
    // colors.
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(GLfloat) *
                 (  _points.size()     +
                    _normals.size()    +
                    _colors.size()     +
                    _linePoints.size() +
                    _lineColors.size() +
                    _IDColors.size()   +
                    _lineIDColors.size()),
                 NULL, GL_STATIC_DRAW);

    // Write the raw points into the buffer.
    _AppendSubData<GLfloat>(GL_ARRAY_BUFFER, &offset, _points);

    // Write the raw normals into the buffer location right after the end of the
    // point data.
    _AppendSubData<GLfloat>(GL_ARRAY_BUFFER, &offset, _normals);

    // Write the raw colors into the buffer location right after the end of the
    // normals data, followed by each other vertex attribute.
    _AppendSubData<GLfloat>(GL_ARRAY_BUFFER, &offset, _colors);
    _AppendSubData<GLfloat>(GL_ARRAY_BUFFER, &offset, _linePoints);
    _AppendSubData<GLfloat>(GL_ARRAY_BUFFER, &offset, _lineColors);
    _AppendSubData<GLfloat>(GL_ARRAY_BUFFER, &offset, _IDColors);
    _AppendSubData<GLfloat>(GL_ARRAY_BUFFER, &offset, _lineIDColors);

    glGenBuffers(1, &_indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBuffer);

    // The index buffer contains the vertex indices defining each face and
    // line to be drawn.
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(GLuint)*(_verts.size()+_lineVerts.size()),
                 NULL, GL_STATIC_DRAW);

    // Write the indices for the polygons followed by lines.
    offset = 0;
    _AppendSubData<GLuint>(GL_ELEMENT_ARRAY_BUFFER, &offset, _verts);
    _AppendSubData<GLuint>(GL_ELEMENT_ARRAY_BUFFER, &offset, _lineVerts);
}

SdfPath
UsdImagingGLLegacyEngine::GetRprimPathFromPrimId(int primId) const 
{
    _PrimIDMap::const_iterator it = _primIDMap.find(primId);
    if(it != _primIDMap.end()) {
        return it->second;
    }
    return SdfPath();
}

void
UsdImagingGLLegacyEngine::_DrawPolygons(bool drawID)
{
    if (_points.empty()) {
        return;
    }

    // Indicate the buffer offsets at which the vertex, normals, and color data
    // begin for polygons.
    glVertexPointer(3, GL_FLOAT, 0, 0);
    size_t offset = sizeof(GLfloat)* _points.size(); 
    glNormalPointer(GL_FLOAT, 0, (GLvoid*)offset);

    offset += sizeof(GLfloat) * _normals.size();
    if (drawID)
        offset += sizeof(GLfloat) * (_colors.size() + 
                                     _linePoints.size() + 
                                     _lineColors.size());
    glColorPointer(3, GL_FLOAT, 0, (GLvoid*)offset);

    if (!_SupportsPrimitiveRestartIndex()) {
        glMultiDrawElements(GL_POLYGON,
                            (GLsizei*)(&(_numVerts[0])),
                            GL_UNSIGNED_INT,
                            (const GLvoid**)(&(_vertIdxOffsets[0])),
                            _numVerts.size());
    } else {
        glDrawElements(GL_POLYGON, _verts.size(), GL_UNSIGNED_INT, 0);
    }
}

void
UsdImagingGLLegacyEngine::_DrawLines(bool drawID)
{
    // We are just drawing curves as unrefined line segments, so we turn off
    // normals.
    glDisableClientState(GL_NORMAL_ARRAY);

    if (_linePoints.empty()) {
        return;
    }

    // Indicate the buffer offsets at which the vertex and color data begin for
    // lines.
    size_t offset = sizeof(GLfloat) * (_points.size() +
                                        _normals.size() +
                                        _colors.size());
    glVertexPointer(3, GL_FLOAT, 0, (GLvoid*)offset);

    offset += sizeof(GLfloat) * _linePoints.size();
    if (drawID)
        offset += sizeof(GLfloat) * (_lineColors.size() + _IDColors.size());
    glColorPointer(3, GL_FLOAT, 0, (GLvoid*)offset);

    if (!_SupportsPrimitiveRestartIndex()) {
        glMultiDrawElements(GL_LINE_STRIP,
                            (GLsizei*)(&(_numLineVerts[0])),
                            GL_UNSIGNED_INT,
                            (const GLvoid**)(&(_lineVertIdxOffsets[0])),
                            _numLineVerts.size());
    } else {
        glDrawElements(GL_LINE_STRIP, _lineVerts.size(), GL_UNSIGNED_INT,
                       (GLvoid*)(sizeof(GLuint)*_verts.size()));
    }
}

void
UsdImagingGLLegacyEngine::Render(const UsdPrim& root, 
    const UsdImagingGLRenderParams& params)
{
    TRACE_FUNCTION();

    // Start listening for change notices from this stage.
    UsdImagingGLLegacyEnginePtr self = TfCreateWeakPtr(this);

    // Invalidate existing buffers if we are drawing from a different root or
    // frame.
    if (_root != root || _params.frame != params.frame ||
        _params.gammaCorrectColors != params.gammaCorrectColors) {
        InvalidateBuffers();

        TfNotice::Revoke(_objectsChangedNoticeKey);
        _objectsChangedNoticeKey = TfNotice::Register(self, 
                                                  &This::_OnObjectsChanged,
                                                  root.GetStage());
    }

    _root = root;
    _params = params;

    glPushAttrib( GL_LIGHTING_BIT );
    glPushAttrib( GL_POLYGON_BIT );
    glPushAttrib( GL_CURRENT_BIT );
    glPushAttrib( GL_ENABLE_BIT );

    if (params.cullStyle == UsdImagingGLCullStyle::CULL_STYLE_NOTHING) {
        glDisable( GL_CULL_FACE );
    } else {
        static const GLenum USD_2_GL_CULL_FACE[] =
        {
                0,         // No Opinion - Unused
                0,         // CULL_STYLE_NOTHING - Unused
                GL_BACK,   // CULL_STYLE_BACK
                GL_FRONT,  // CULL_STYLE_FRONT
                GL_BACK,   // CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED
        };
        static_assert((sizeof(USD_2_GL_CULL_FACE) / 
                       sizeof(USD_2_GL_CULL_FACE[0])) 
                == (size_t)UsdImagingGLCullStyle::CULL_STYLE_COUNT, 
            "enum size mismatch");

        // XXX: CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED, should disable cull face 
        // for double-sided prims.
        glEnable( GL_CULL_FACE );
        glCullFace(USD_2_GL_CULL_FACE[(size_t)params.cullStyle]);
    }

    if (_params.drawMode != UsdImagingGLDrawMode::DRAW_GEOM_ONLY      &&
        _params.drawMode != UsdImagingGLDrawMode::DRAW_GEOM_SMOOTH    &&
        _params.drawMode != UsdImagingGLDrawMode::DRAW_GEOM_FLAT) {
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE); 
                
        const float ambientColor[4] = {0.f,0.f,0.f,1.f};
        glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, ambientColor );
                
        glEnable(GL_NORMALIZE);
    }

    switch (_params.drawMode) {
    case UsdImagingGLDrawMode::DRAW_WIREFRAME:
        glDisable( GL_LIGHTING );
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        break;
    case UsdImagingGLDrawMode::DRAW_SHADED_FLAT:
    case UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH:
        break;
    default:
        break;
    }


    if (_SupportsPrimitiveRestartIndex()) {
        glPrimitiveRestartIndexNV( _PRIM_RESTART_INDEX );
        glEnableClientState( GL_PRIMITIVE_RESTART_NV );
    }

    if (!_attribBuffer) {

        _ctm = GfMatrix4d(1.0);
        TfReset(_xformStack);
        TfReset(_points);
        TfReset(_normals);
        TfReset(_colors);
        TfReset(_IDColors);
        TfReset(_verts);
        TfReset(_numVerts);
        TfReset(_linePoints);
        TfReset(_lineColors);
        TfReset(_lineIDColors);
        TfReset(_lineVerts);
        TfReset(_numLineVerts);
        TfReset(_vertIdxOffsets);
        TfReset(_lineVertIdxOffsets);
        _vertCount = 0;
        _lineVertCount = 0;
        _primIDCounter = 0;

        _TraverseStage(root);
    }

    TF_VERIFY(_xformStack.empty());

    if (!_attribBuffer) {
        _PopulateBuffers();
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, _attribBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBuffer);
    }

    glEnableClientState(GL_VERTEX_ARRAY);

    bool drawID = false;
    if (_params.enableIdRender) {
        glEnableClientState(GL_COLOR_ARRAY);
        glDisable( GL_LIGHTING );
        //glShadeModel(GL_FLAT);

        // XXX:
        // Will need to revisit this for semi-transparent geometry.
        glDisable( GL_ALPHA_TEST );
        glDisable( GL_BLEND );
        drawID = true;
    } else {
        glShadeModel(GL_SMOOTH);
    }

    switch (_params.drawMode) {
    case UsdImagingGLDrawMode::DRAW_GEOM_FLAT:
    case UsdImagingGLDrawMode::DRAW_GEOM_SMOOTH:
        glEnableClientState(GL_NORMAL_ARRAY);
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
        break;
    case UsdImagingGLDrawMode::DRAW_SHADED_FLAT:
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
        glShadeModel(GL_FLAT);
        break;
    case UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH:
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
        break;
    case UsdImagingGLDrawMode::DRAW_POINTS:
        glEnableClientState(GL_COLOR_ARRAY);
        glPolygonMode( GL_FRONT_AND_BACK, GL_POINT);
    default:
        break;
    }

    // Draw the overlay wireframe, if requested.
    if (_params.drawMode == UsdImagingGLDrawMode::DRAW_WIREFRAME_ON_SURFACE) {
        // We have to push lighting again since we don't know what state we want
        // after this without popping.
        glPushAttrib( GL_LIGHTING_BIT );
        glDisable( GL_LIGHTING );
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        _DrawPolygons(/*drawID*/false);
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
        glPopAttrib(); // GL_LIGHTING_BIT
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);

        // Offset the triangles we're about to draw next.
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(.5, 1.0);
    }

    // Draw polygons & curves.
    _DrawPolygons(drawID);

    if (_params.drawMode == UsdImagingGLDrawMode::DRAW_WIREFRAME_ON_SURFACE)
        glDisable(GL_POLYGON_OFFSET_FILL);

    _DrawLines(drawID);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    if (_SupportsPrimitiveRestartIndex()) {
        glDisableClientState(GL_PRIMITIVE_RESTART_NV);
    }

    glPopAttrib(); // GL_ENABLE_BIT
    glPopAttrib(); // GL_CURRENT_BIT
    glPopAttrib(); // GL_POLYGON_BIT
    glPopAttrib(); // GL_LIGHTING_BIT
}

void
UsdImagingGLLegacyEngine::SetCameraState(const GfMatrix4d& viewMatrix,
                            const GfMatrix4d& projectionMatrix,
                            const GfVec4d& viewport)
{
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);   

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(projectionMatrix.GetArray());

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd(viewMatrix.GetArray());
}

void
UsdImagingGLLegacyEngine::SetLightingState(GlfSimpleLightVector const &lights,
                                        GlfSimpleMaterial const &material,
                                        GfVec4f const &sceneAmbient)
{
    if (lights.empty()) {
        glDisable(GL_LIGHTING);
    } else {
        glEnable(GL_LIGHTING);

        static int maxLights = 0;
        if (maxLights == 0) {
            glGetIntegerv(GL_MAX_LIGHTS, &maxLights);
        }

        for (size_t i = 0; i < static_cast<size_t>(maxLights); ++i) {
            if (i < lights.size()) {
                glEnable(GL_LIGHT0+i);
                GlfSimpleLight const &light = lights[i];

                glLightfv(GL_LIGHT0+i, GL_POSITION, light.GetPosition().data());
                glLightfv(GL_LIGHT0+i, GL_AMBIENT,  light.GetAmbient().data());
                glLightfv(GL_LIGHT0+i, GL_DIFFUSE,  light.GetDiffuse().data());
                glLightfv(GL_LIGHT0+i, GL_SPECULAR, light.GetSpecular().data());
                // omit spot parameters.
            } else {
                glDisable(GL_LIGHT0+i);
            }
        }
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,
                     material.GetAmbient().data());
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,
                     material.GetSpecular().data());
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS,
                    material.GetShininess());
    }
}

void 
UsdImagingGLLegacyEngine::_OnObjectsChanged(UsdNotice::ObjectsChanged const& notice,
                                       UsdStageWeakPtr const& sender)
{
    InvalidateBuffers(); 
}

static void 
UsdImagingGLLegacyEngine_ComputeSmoothNormals(const VtVec3fArray &points,
                                  const VtIntArray &numVerts,
                                  const VtIntArray &verts, 
                                  bool ccw,
                                  VtVec3fArray * normals)
{
    TRACE_FUNCTION();

    // Compute an output normal for each point.
    size_t pointsCount = points.size();
    if (normals->size() != pointsCount)
        *normals = VtVec3fArray(pointsCount);

    // Use direct pointer access for speed.
    GfVec3f *normalsPtr = normals->data();
    GfVec3f const *pointsPtr = points.cdata();
    int const *vertsPtr = verts.cdata();

    // Zero out the normals
    for (size_t i=0; i<pointsCount; ++i)
        normalsPtr[i].Set(0,0,0);

    bool foundOutOfBoundsIndex = false;

    // Compute a normal at each vertex of each face.
    int firstIndex = 0;
    TF_FOR_ALL(j, numVerts) {

        int nv = *j;
        for (int i=0; i<nv; ++i) {

            int a = vertsPtr[firstIndex + i];
            int b = vertsPtr[firstIndex + ((i+1) < nv ? i+1 : i+1 - nv)];
            int c = vertsPtr[firstIndex + ((i+2) < nv ? i+2 : i+2 - nv)];

            // Make sure that we don't read or write using an out-of-bounds
            // index.
            if (a >= 0 && static_cast<size_t>(a) < pointsCount &&
                b >= 0 && static_cast<size_t>(b) < pointsCount &&
                c >= 0 && static_cast<size_t>(c) < pointsCount) {

                GfVec3f p0 = pointsPtr[a] - pointsPtr[b];
                GfVec3f p1 = pointsPtr[c] - pointsPtr[b];
                // Accumulate face normal
                GfVec3f n = normalsPtr[b];
                if (ccw)
                    normalsPtr[b] = n - GfCross(p0, p1);
                else
                    normalsPtr[b] = n + GfCross(p0, p1);
            } else {

                foundOutOfBoundsIndex = true;

                // Make sure we compute some normal for all points that are
                // in bounds.
                if (b >= 0 && static_cast<size_t>(b) < pointsCount)
                    normalsPtr[b] = GfVec3f(0);
            }
        }
        firstIndex += nv;
    }

    if (foundOutOfBoundsIndex) {
        TF_WARN("Out of bound indices detected while computing smooth normals.");
    }
}

static bool
_ShouldCullDueToOpacity(const UsdGeomGprim *gprimSchema, const UsdTimeCode &frame)
{
    // XXX:
    // Do not draw geometry below the opacity threshold, until we support
    // semi-transparent drawing.
    static const float OPACITY_THRESHOLD = 0.5f;
    VtArray<float> opacityArray;
    gprimSchema->GetDisplayOpacityPrimvar().ComputeFlattened(&opacityArray, frame);
    // XXX display opacity can vary on the surface, just using the first value
    //     for testing (the opacity is likely constant anyway)
    return opacityArray.size() > 0 && opacityArray[0] < OPACITY_THRESHOLD;
}

void
UsdImagingGLLegacyEngine::_TraverseStage(const UsdPrim& root)
{
    // Instead of using root.begin(), setup a special iterator that does both
    // pre-order and post-order traversal so we can push and pop state.
    UsdPrimRange range = UsdPrimRange::PreAndPostVisit(root);

    UsdPrim pseudoRoot = root.GetStage()->GetPseudoRoot();

    // Traverse the stage to extract data for drawing.
    for (auto iter = range.begin(); iter != range.end(); ++iter) {
        if (!iter.IsPostVisit()) {

            if (_excludedSet.find(iter->GetPath()) != _excludedSet.end()) {
                iter.PruneChildren();
                ++iter;
                continue;
            }

            bool visible = true;

            // Because we are pruning invisible subtrees, we can assume all
            // parent prims have "inherited" visibility.
            TfToken visibility;
            if (*iter != pseudoRoot &&
                iter->GetAttribute(UsdGeomTokens->visibility)
                    .Get(&visibility, _params.frame) &&
                visibility == UsdGeomTokens->invisible) {
                
                visible = false;
            }

            // Treat only the purposes we've been asked to show as visible
            TfToken purpose;
            if (*iter != pseudoRoot
                && iter->GetAttribute(UsdGeomTokens->purpose)
                   .Get(&purpose, _params.frame)
                && purpose != UsdGeomTokens->default_  // fast/common out
                && (
                     (purpose == UsdGeomTokens->guide 
                       && !_params.showGuides)
                     || (purpose == UsdGeomTokens->render
                         && !_params.showRender)
                     || (purpose == UsdGeomTokens->proxy
                         && !_params.showProxy))
                ) {
                visible = false;
            }

            // Do pre-visit data extraction.
            if (visible) {
                if (iter->IsA<UsdGeomXform>())
                    _HandleXform(*iter);
                else if (iter->IsA<UsdGeomMesh>())
                    _HandleMesh(*iter);
                else if (iter->IsA<UsdGeomCurves>())
                    _HandleCurves(*iter);
                else if (iter->IsA<UsdGeomCube>())
                    _HandleCube(*iter);
                else if (iter->IsA<UsdGeomSphere>())
                    _HandleSphere(*iter);
                else if (iter->IsA<UsdGeomCone>())
                    _HandleCone(*iter);
                else if (iter->IsA<UsdGeomCylinder>())
                    _HandleCylinder(*iter);
                else if (iter->IsA<UsdGeomCapsule>())
                    _HandleCapsule(*iter);
                else if (iter->IsA<UsdGeomPoints>())
                    _HandlePoints(*iter);
                else if (iter->IsA<UsdGeomNurbsPatch>())
                    _HandleNurbsPatch(*iter);                    
            } else {
                iter.PruneChildren();
            }

        } else {
            if (!_xformStack.empty()) {
                const std::pair<UsdPrim, GfMatrix4d> &entry =
                    _xformStack.back();
                if (entry.first == *iter) {
                    // pop state
                    _xformStack.pop_back();
                    _ctm = entry.second;
                }
            }
        }
    }

    // Apply the additional offset from the polygon vertex indices, which are
    // before the line vertex indices in the element array buffer.
    size_t polygonVertOffset = _verts.size() * sizeof(GLuint);
    TF_FOR_ALL(itr, _lineVertIdxOffsets) {
        *itr = (GLvoid*)((size_t)(*itr) + polygonVertOffset);
    }
}

void
UsdImagingGLLegacyEngine::_ProcessGprimColor(const UsdGeomGprim *gprimSchema,
                                 const UsdPrim &prim,
                                 bool *doubleSided,
                                 VtArray<GfVec3f> *color,
                                 TfToken *interpolation)
{
    // Get DoubleSided Attribute
    gprimSchema->GetDoubleSidedAttr().Get(doubleSided);

    // Get interpolation and color using UsdShadeMaterial'
    VtValue colorAsVt;
    if (UsdImagingGprimAdapter::GetColor(prim, 
                _params.frame, interpolation, &colorAsVt)) {
        VtVec3fArray temp = colorAsVt.Get<VtVec3fArray>();
        color->push_back(temp[0]);
    } else {
        color->push_back(GfVec3f(0.5f, 0.5f, 0.5f));
        *interpolation = UsdGeomTokens->constant;
    }
}

void 
UsdImagingGLLegacyEngine::_HandleXform(const UsdPrim &prim) 
{ 
    // Don't apply the root prim's transform.
    if (prim == _root)
        return;

    GfMatrix4d xform(1.0);
    UsdGeomXform xf(prim);
    bool resetsXformStack = false;
    xf.GetLocalTransformation(&xform, &resetsXformStack, _params.frame);
    static const GfMatrix4d IDENTITY(1.0);

    // XXX:
    // Should do GfIsClose for each element.
    if (xform != IDENTITY) {
        _xformStack.push_back(std::make_pair(prim, _ctm));
        if (!resetsXformStack)
            _ctm = xform * _ctm;
        else
            _ctm = xform;
    }
}

GfVec4f 
UsdImagingGLLegacyEngine::_IssueID(SdfPath const& path)
{
    _PrimID::ValueType maxId = (1 << 24) - 1;
    // Notify the user (failed verify) and return an invalid ID.
    // Picking will fail, but execution can continue.
    if (!TF_VERIFY(_primIDCounter < maxId))
        return GfVec4f(0);

    _PrimID::ValueType id = _primIDCounter++;
    _primIDMap[id] = path;
    return _PrimID::Unpack(id);
}

void 
UsdImagingGLLegacyEngine::_HandleMesh(const UsdPrim &prim)
{ 
    TRACE_FUNCTION();

    UsdGeomMesh geoSchema(prim);

    if (_ShouldCullDueToOpacity(&geoSchema, _params.frame))
        return;

    // Apply xforms for node-collapsed geometry.
    _HandleXform(prim);

    // Get points and topology from the mesh
    VtVec3fArray pts;
    geoSchema.GetPointsAttr().Get(&pts, _params.frame); 
    VtIntArray nmvts;
    geoSchema.GetFaceVertexCountsAttr().Get(&nmvts, _params.frame);
    VtIntArray vts;
    geoSchema.GetFaceVertexIndicesAttr().Get(&vts, _params.frame);

    _RenderPrimitive(prim, &geoSchema, pts, nmvts, vts );
}

void
UsdImagingGLLegacyEngine::_HandleCurves(const UsdPrim& prim)
{
    TRACE_FUNCTION();

    UsdGeomCurves curvesSchema(prim);

    if (_ShouldCullDueToOpacity(&curvesSchema, _params.frame))
        return;

    // Setup an ID color for picking.
    GfVec4f idColor = _IssueID(prim.GetPath());

    // Apply xforms for node-collapsed geometry.
    _HandleXform(prim);

    bool doubleSided = false;
    VtArray<GfVec3f> color;
    TfToken colorInterpolation = UsdGeomTokens->constant;

    _ProcessGprimColor(&curvesSchema, prim, &doubleSided, &color,
                       &colorInterpolation);

    VtVec3fArray pts;
    curvesSchema.GetPointsAttr().Get(&pts, _params.frame);


    if (color.size() < 1) {

        // set default
        color = VtArray<GfVec3f>(1);
        color[0] = GfVec3f(0.5f, 0.5f, 0.5f);
        colorInterpolation = UsdGeomTokens->constant;

    // Check for error condition for vertex colors
    } else if (colorInterpolation == UsdGeomTokens->vertex &&
               color.size() != pts.size()) {

        // fallback to default
        color = VtArray<GfVec3f>(1);
        color[0] = GfVec3f(0.5f, 0.5f, 0.5f);
        colorInterpolation = UsdGeomTokens->constant;
        TF_WARN("Color primvar error on prim '%s'", prim.GetPath().GetText());
    }

    int index = 0;
    TF_FOR_ALL(itr, pts) {
        GfVec3f pt = _ctm.Transform(*itr);
        _linePoints.push_back(pt[0]);
        _linePoints.push_back(pt[1]);
        _linePoints.push_back(pt[2]);
        
        GfVec3f currColor = color[0];
        if (colorInterpolation == UsdGeomTokens->uniform) {
            // XXX uniform not yet supported, fallback to constant
        } else if (colorInterpolation == UsdGeomTokens->vertex) {
            currColor = color[index];
        } else if (colorInterpolation == UsdGeomTokens->faceVarying) {
            // XXX faceVarying not yet supported, fallback to constant
        }
        _lineColors.push_back(currColor[0]);
        _lineColors.push_back(currColor[1]);
        _lineColors.push_back(currColor[2]);
        _AppendIDColor(idColor, &_lineIDColors);
        index++;
    }

    VtIntArray nmvts;
    curvesSchema.GetCurveVertexCountsAttr().Get(&nmvts, _params.frame);

    TF_FOR_ALL(itr, nmvts) {
        for(int idx=0; idx < *itr; idx++) {
            _lineVerts.push_back(idx + _lineVertCount);
        }
        if (!_SupportsPrimitiveRestartIndex()) {
            // If prim restart is not supported, we need to keep track of the
            // number of vertices per line segment, as well as the byte-offsets
            // into the element array buffer containing the vertex indices for
            // the lines. Upon completion of stage traversal, we will apply the
            // additional offset from the polygon vertex indices, which are
            // before the line vertex indices in the element array buffer.
            _numLineVerts.push_back(*itr);
            _lineVertIdxOffsets.push_back((GLvoid*)(_lineVertCount * sizeof(GLuint)));
        } else {
            // Append a primitive restart index at the end of each numVerts
            // index boundary.
            _lineVerts.push_back(_PRIM_RESTART_INDEX);
        }

        _lineVertCount += *itr;
    }

    // Ignoring normals and widths, since we are only drawing the unrefined CV's
    // as lines segments.
}

void 
UsdImagingGLLegacyEngine::_HandleCube(const UsdPrim &prim)
{ 
    TRACE_FUNCTION();

    UsdGeomCube geoSchema(prim);
    if (_ShouldCullDueToOpacity(&geoSchema, _params.frame))
        return;

    // Apply xforms for node-collapsed geometry.
    _HandleXform(prim);

    // Update the transform with the size authored for the cube.
    GfMatrix4d xf = UsdImagingCubeAdapter::GetMeshTransform(prim, _params.frame);
    _ctm = xf * _ctm;

    // Get points and topology from the mesh
    VtValue ptsSource = UsdImagingCubeAdapter::GetMeshPoints(prim, _params.frame);
    VtArray<GfVec3f> pts = ptsSource.Get< VtArray<GfVec3f> >();

    VtValue tpSource = UsdImagingCubeAdapter::GetMeshTopology();
    HdMeshTopology tp = tpSource.Get<HdMeshTopology>();
    VtIntArray nmvts = tp.GetFaceVertexCounts();
    VtIntArray vts = tp.GetFaceVertexIndices();

    _RenderPrimitive(prim, &geoSchema, pts, nmvts, vts );
}

void 
UsdImagingGLLegacyEngine::_HandleSphere(const UsdPrim &prim)
{ 
    TRACE_FUNCTION();

    UsdGeomSphere geoSchema(prim);
    if (_ShouldCullDueToOpacity(&geoSchema, _params.frame))
        return;

    // Apply xforms for node-collapsed geometry.
    _HandleXform(prim);

    // Update the transform with the size authored for the cube.
    GfMatrix4d xf = UsdImagingSphereAdapter::GetMeshTransform(prim, _params.frame);
    _ctm = xf * _ctm;

    // Get points and topology from the mesh
    VtValue ptsSource = UsdImagingSphereAdapter::GetMeshPoints(prim, _params.frame);
    VtArray<GfVec3f> pts = ptsSource.Get< VtArray<GfVec3f> >();

    VtValue tpSource = UsdImagingSphereAdapter::GetMeshTopology();
    HdMeshTopology tp = tpSource.Get<HdMeshTopology>();
    VtIntArray nmvts = tp.GetFaceVertexCounts();
    VtIntArray vts = tp.GetFaceVertexIndices();

    _RenderPrimitive(prim, &geoSchema, pts, nmvts, vts );
}

void 
UsdImagingGLLegacyEngine::_HandleCone(const UsdPrim &prim)
{ 
    TRACE_FUNCTION();

    UsdGeomCone geoSchema(prim);
    if (_ShouldCullDueToOpacity(&geoSchema, _params.frame))
        return;

    // Apply xforms for node-collapsed geometry.
    _HandleXform(prim);

    // Get points and topology from the mesh
    VtValue ptsSource = UsdImagingConeAdapter::GetMeshPoints(prim, _params.frame);
    VtArray<GfVec3f> pts = ptsSource.Get< VtArray<GfVec3f> >();

    VtValue tpSource = UsdImagingConeAdapter::GetMeshTopology();
    HdMeshTopology tp = tpSource.Get<HdMeshTopology>();
    VtIntArray nmvts = tp.GetFaceVertexCounts();
    VtIntArray vts = tp.GetFaceVertexIndices();

    _RenderPrimitive(prim, &geoSchema, pts, nmvts, vts );
}

void 
UsdImagingGLLegacyEngine::_HandleCylinder(const UsdPrim &prim)
{ 
    TRACE_FUNCTION();

    UsdGeomCylinder geoSchema(prim);
    if (_ShouldCullDueToOpacity(&geoSchema, _params.frame))
        return;

    // Apply xforms for node-collapsed geometry.
    _HandleXform(prim);

    // Get points and topology from the mesh
    VtValue ptsSource = UsdImagingCylinderAdapter::GetMeshPoints(prim, _params.frame);
    VtArray<GfVec3f> pts = ptsSource.Get< VtArray<GfVec3f> >();

    VtValue tpSource = UsdImagingCylinderAdapter::GetMeshTopology();
    HdMeshTopology tp = tpSource.Get<HdMeshTopology>();
    VtIntArray nmvts = tp.GetFaceVertexCounts();
    VtIntArray vts = tp.GetFaceVertexIndices();

    _RenderPrimitive(prim, &geoSchema, pts, nmvts, vts );
}

void 
UsdImagingGLLegacyEngine::_HandleCapsule(const UsdPrim &prim)
{ 
    TRACE_FUNCTION();

    UsdGeomCapsule geoSchema(prim);
    if (_ShouldCullDueToOpacity(&geoSchema, _params.frame))
        return;

    // Apply xforms for node-collapsed geometry.
    _HandleXform(prim);

    // Get points and topology from the mesh
    VtValue ptsSource = UsdImagingCapsuleAdapter::GetMeshPoints(prim, _params.frame);
    VtArray<GfVec3f> pts = ptsSource.Get< VtArray<GfVec3f> >();

    VtValue tpSource = UsdImagingCapsuleAdapter::GetMeshTopology();
    HdMeshTopology tp = tpSource.Get<HdMeshTopology>();
    VtIntArray nmvts = tp.GetFaceVertexCounts();
    VtIntArray vts = tp.GetFaceVertexIndices();

    _RenderPrimitive(prim, &geoSchema, pts, nmvts, vts );
}

void 
UsdImagingGLLegacyEngine::_HandlePoints(const UsdPrim &prim)
{
    TF_WARN("Point primitives are not yet supported.");
}

void 
UsdImagingGLLegacyEngine::_HandleNurbsPatch(const UsdPrim &prim)
{ 
    TRACE_FUNCTION();

    UsdGeomNurbsPatch geoSchema(prim);
    if (_ShouldCullDueToOpacity(&geoSchema, _params.frame))
        return;

    // Apply xforms for node-collapsed geometry.
    _HandleXform(prim);

    // Get points and topology from the mesh
    VtValue ptsSource = UsdImagingNurbsPatchAdapter::GetMeshPoints(prim, _params.frame);
    VtArray<GfVec3f> pts = ptsSource.Get< VtArray<GfVec3f> >();
    VtValue tpSource = UsdImagingNurbsPatchAdapter::GetMeshTopology(prim, _params.frame);
    HdMeshTopology tp = tpSource.Get<HdMeshTopology>();
    VtIntArray nmvts = tp.GetFaceVertexCounts();
    VtIntArray vts = tp.GetFaceVertexIndices();

    _RenderPrimitive(prim, &geoSchema, pts, nmvts, vts );
}

void 
UsdImagingGLLegacyEngine::_RenderPrimitive(const UsdPrim &prim, 
                                      const UsdGeomGprim *gprimSchema, 
                                      const VtArray<GfVec3f>& pts, 
                                      const VtIntArray& nmvts,
                                      const VtIntArray& vts)
{ 
    // Prepare vertex/color/index buffers
    bool doubleSided = false;
    VtArray<GfVec3f> color;
    TfToken colorInterpolation = UsdGeomTokens->constant;

    _ProcessGprimColor(gprimSchema, prim, &doubleSided, &color, &colorInterpolation);
    if (color.size() < 1) {
        // set default
        color = VtArray<GfVec3f>(1);
        color[0] = GfVec3f(0.5, 0.5, 0.5);
        colorInterpolation = UsdGeomTokens->constant;
    }

    // Setup an ID color for picking.
    GfVec4f idColor = _IssueID(prim.GetPath());

    int index = 0;
    TF_FOR_ALL(itr, pts) {
        GfVec3f pt = _ctm.Transform(*itr);
        _points.push_back(pt[0]);
        _points.push_back(pt[1]);
        _points.push_back(pt[2]);
        
        GfVec3f currColor = color[0];
        if (colorInterpolation == UsdGeomTokens->uniform) {
            // XXX uniform not yet supported, fallback to constant
        } else if (colorInterpolation == UsdGeomTokens->vertex) {
            currColor = color[index];
        } else if (colorInterpolation == UsdGeomTokens->faceVarying) {
            // XXX faceVarying not yet supported, fallback to constant
        }
        _colors.push_back(currColor[0]);
        _colors.push_back(currColor[1]);
        _colors.push_back(currColor[2]);
        _AppendIDColor(idColor, &_IDColors);
        index++;
    }

    VtVec3fArray normals;

    if (!_SupportsPrimitiveRestartIndex()) {
        // If prim restart is not supported, we need to keep track of the number
        // of vertices per polygon, as well as the byte-offsets for where the
        // indices in the element array buffer start for each polygon.
        int indexCount = _verts.size();
        TF_FOR_ALL(itr, nmvts) {
            _vertIdxOffsets.push_back((GLvoid*)(indexCount * sizeof(GLuint)));
            indexCount += *itr;

            _numVerts.push_back(*itr);
        }

        TF_FOR_ALL(itr, vts) {
            _verts.push_back(*itr + _vertCount);
        }
    } else {
        for (size_t i=0, j=0, k=0; i<vts.size(); ++i) {
            _verts.push_back(vts[i] + _vertCount);
            
            // Append a primitive restart index at the end of each numVerts
            // index boundary.
            if (k < nmvts.size() && ++j == static_cast<size_t>(nmvts[k])) {
                _verts.push_back(_PRIM_RESTART_INDEX);
                j=0, k++;
            }
        }
    }

    _vertCount += pts.size();

    // XXX:
    // Need to add orientation to GeometrySchema and reconvert assets if any of
    // them have authored opinions.

    // If the user is using FLAT SHADING it will still use interpolated normals
    // which means that OpenGL will pick one normal (provoking vertex) out of the 
    // normals array.
    UsdImagingGLLegacyEngine_ComputeSmoothNormals(pts, nmvts, vts, true /*ccw*/, &normals);

    TF_FOR_ALL(itr, normals) {
        _normals.push_back((*itr)[0]);
        _normals.push_back((*itr)[1]);
        _normals.push_back((*itr)[2]);
    }

    if (doubleSided) {

        // Duplicate the geometry with the normals inverted and topology
        // reversed, so that we handle doublesided geometry alongside
        // backface-culled geometry in the same draw call.

        TRACE_SCOPE("UsdImagingGLLegacyEngine::HandleMesh (doublesided)");

        index = 0;
        TF_FOR_ALL(itr, pts) {
            GfVec3f pt = _ctm.Transform(*itr);
            _points.push_back(pt[0]);
            _points.push_back(pt[1]);
            _points.push_back(pt[2]);
            
            GfVec3f currColor = color[0];
            if (colorInterpolation == UsdGeomTokens->uniform) {
                // XXX uniform not yet supported, fallback to constant
            } else if (colorInterpolation == UsdGeomTokens->vertex) {
                currColor = color[index];
            } else if (colorInterpolation == UsdGeomTokens->faceVarying) {
                // XXX faceVarying not yet supported, fallback to constant
            }
            _colors.push_back(currColor[0]);
            _colors.push_back(currColor[1]);
            _colors.push_back(currColor[2]);
            _AppendIDColor(idColor, &_IDColors);

            index++;
        }

        if (!_SupportsPrimitiveRestartIndex()) {
            int indexCount = _verts.size();
            for (int i=nmvts.size()-1; i>=0; --i) {
                _vertIdxOffsets.push_back((GLvoid*)(indexCount * sizeof(GLuint)));
                _numVerts.push_back(nmvts[i]);
                indexCount += nmvts[i];
            }
            for (int i=vts.size()-1; i>=0; --i) {
                _verts.push_back(vts[i] + _vertCount);
            }
        } else {
            for (int i=vts.size()-1, j=0, k=nmvts.size()-1; i>=0; --i) {
                _verts.push_back(vts[i] + _vertCount);
                
                // Append a primitive restart index at the end of each numVerts
                // index boundary.
                if (k >= 0  && ++j == nmvts[k]) {
                    _verts.push_back(_PRIM_RESTART_INDEX);
                    j=0, k--;
                }
            }
        }

        _vertCount += pts.size();

        TF_FOR_ALL(itr, normals) {
            _normals.push_back(-(*itr)[0]);
            _normals.push_back(-(*itr)[1]);
            _normals.push_back(-(*itr)[2]);
        }
    }
}

bool
UsdImagingGLLegacyEngine::TestIntersection(
    const GfMatrix4d &viewMatrix,
    const GfMatrix4d &projectionMatrix,
    const GfMatrix4d &worldToLocalSpace,
    const UsdPrim& root, 
    const UsdImagingGLRenderParams& params,
    GfVec3d *outHitPoint,
    SdfPath *outHitPrimPath,
    SdfPath *outHitInstancerPath,
    int *outHitInstanceIndex,
    int *outHitElementIndex)
{
    // Choose a framebuffer that's large enough to catch thin slice polys.  No
    // need to go too large though, since the depth writes will accumulate to
    // the correct answer.

    const int width = 128;
    const int height = width;

    if (GlfHasLegacyGraphics()) {
        TF_RUNTIME_ERROR("framebuffer object not supported");
        return false;
    }

    // Use a separate drawTarget (framebuffer object) for each GL context
    // that uses this renderer, but the drawTargets can share attachments.
    
    GlfGLContextSharedPtr context = GlfGLContext::GetCurrentGLContext();
    if (!TF_VERIFY(context)) {
        TF_RUNTIME_ERROR("Invalid GL context");
        return false;
    }

    GfVec2i attachmentSize(width,height);
    GlfDrawTargetRefPtr drawTarget;
    if (!TfMapLookup(_drawTargets, context, &drawTarget)) {

        // Create an instance for use with this GL context
        drawTarget = GlfDrawTarget::New(attachmentSize);

        if (!_drawTargets.empty()) {
            // Share existing attachments
            drawTarget->Bind();
            drawTarget->CloneAttachments(_drawTargets.begin()->second);
            drawTarget->Unbind();
        } else {
            // Need to create initial attachments
            drawTarget->Bind();
            drawTarget->AddAttachment(
                "primId", GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8);
            drawTarget->AddAttachment(
                "instanceId", GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8);
            drawTarget->AddAttachment(
                "elementId", GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8);
            drawTarget->AddAttachment(
                "depth", GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_COMPONENT32F);
            drawTarget->Unbind();
        }

        // This is a good time to clean up any drawTargets no longer in use.
        for (_DrawTargetPerContextMap::iterator
                it = _drawTargets.begin(); it != _drawTargets.end(); ++it) {
            if (!(it->first && it->first->IsValid())) {
                _drawTargets.erase(it);
            }
        }

        _drawTargets[context] = drawTarget;
    }

    // Resize if necessary
    if (drawTarget->GetSize() != attachmentSize) {
        drawTarget->SetSize(attachmentSize);
    }

    drawTarget->Bind();

    glPushAttrib( GL_VIEWPORT_BIT |
                  GL_ENABLE_BIT |
                  GL_COLOR_BUFFER_BIT |
                  GL_DEPTH_BUFFER_BIT |
                  GL_TEXTURE_BIT );

    GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);
    
    glDepthMask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    // Setup the modelview matrix
    const GfMatrix4d modelViewMatrix = worldToLocalSpace * viewMatrix;

    // Set up camera matrices and viewport. At some point in the future,
    // this may be handled by Hydra itself since we are calling SetCameraState
    // with all of this information so we can support culling
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixd(projectionMatrix.GetArray());
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixd(modelViewMatrix.GetArray());
   
    glViewport(0, 0, width, height);

    SetCameraState(modelViewMatrix, projectionMatrix, GfVec4d(0,0,width, height) );

    GLF_POST_PENDING_GL_ERRORS();
    
    // to enable wireframe picking, should respect incoming drawMode
    //params.drawMode = DRAW_GEOM_ONLY;
    Render(root, params);
    
    GLF_POST_PENDING_GL_ERRORS();

    // Restore all gl state
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    int xMin = 0;
    int yMin = 0;
    double zMin = 1.0;
    int zMinIndex = -1;

    GLubyte primId[width*height*4];
    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("primId")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, primId);

    GLubyte instanceId[width*height*4];
    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("instanceId")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, instanceId);

    GLubyte elementId[width*height*4];
    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("elementId")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, elementId);

    GLfloat depths[width*height];
    glBindTexture(GL_TEXTURE_2D,
        drawTarget->GetAttachments().at("depth")->GetGlTextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, depths);

    glPopAttrib(); /* GL_VIEWPORT_BIT |
                      GL_ENABLE_BIT |
                      GL_COLOR_BUFFER_BIT
                      GL_DEPTH_BUFFER_BIT
                      GL_TEXTURE_BIT */

    GLF_POST_PENDING_GL_ERRORS();
    
    // Find the smallest value (nearest pixel) in the z buffer
    for (int y=0, i=0; y<height; y++) {
        for (int x=0; x<width; x++, i++) {
            if (depths[i] < zMin) {
	            xMin = x;
	            yMin = y;
	            zMin = depths[i];
                    zMinIndex = i;
            }
        }
    }

    bool didHit = (zMin < 1.0);

    if (didHit) {
        GLint viewport[4] = { 0, 0, width, height };
        GfVec3d hitPoint;

        gluUnProject( xMin, yMin, zMin,
                      viewMatrix.GetArray(),
                      projectionMatrix.GetArray(),
                      viewport,
                      &((*outHitPoint)[0]),
                      &((*outHitPoint)[1]),
                      &((*outHitPoint)[2]));

        if (outHitPrimPath) {
            int idIndex = zMinIndex*4;

            *outHitPrimPath = GetRprimPathFromPrimId(
                    HdxIntersector::DecodeIDRenderColor(&primId[idIndex]));
            if (outHitInstanceIndex) {
                *outHitInstanceIndex = HdxIntersector::DecodeIDRenderColor(
                        &instanceId[idIndex]);
            }
            if (outHitElementIndex) {
                *outHitElementIndex = HdxIntersector::DecodeIDRenderColor(
                        &elementId[idIndex]);
            }

        }
    }

    drawTarget->Unbind();
    GLF_POST_PENDING_GL_ERRORS();

    return didHit;
}

PXR_NAMESPACE_CLOSE_SCOPE

