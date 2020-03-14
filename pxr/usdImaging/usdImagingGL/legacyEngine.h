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
#ifndef PXR_USD_IMAGING_USD_IMAGING_GL_LEGACY_ENGINE_H
#define PXR_USD_IMAGING_USD_IMAGING_GL_LEGACY_ENGINE_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImagingGL/api.h"

#include "pxr/usdImaging/usdImagingGL/renderParams.h"

#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleMaterial.h"

#include "pxr/imaging/garch/gl.h"
#include "pxr/imaging/cameraUtil/conformWindow.h"

#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usd/notice.h"

#include "pxr/base/tf/declarePtrs.h"

#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/hashset.h"

#include <boost/unordered_map.hpp>


PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_PTRS(UsdImagingGLLegacyEngine);

class UsdImagingGLLegacyEngine : public TfWeakBase 
{

    typedef UsdImagingGLLegacyEngine This;

public:

    USDIMAGINGGL_API
    UsdImagingGLLegacyEngine(const SdfPathVector& excludedPaths);

    USDIMAGINGGL_API
    ~UsdImagingGLLegacyEngine();

    // Entry point for kicking off a render
    USDIMAGINGGL_API
    void Render(const UsdPrim& root, 
                const UsdImagingGLRenderParams& params);


    /// Set camera framing state
    /// -- Free Camera API
    USDIMAGINGGL_API
    void SetFreeCameraMatrices(const GfMatrix4d& viewMatrix,
                               const GfMatrix4d& projectionMatrix);

    /// -- Scene camera API
    USDIMAGINGGL_API
    void SetCameraPath(const SdfPath& id);

    /// -- Framing API
    void SetWindowPolicy(CameraUtilConformWindowPolicy policy) {
        _windowPolicy = policy;
    }
    void SetRenderViewport(const GfVec4d& viewport) {
        _viewport = viewport;
    }

    /// Set lighting state
    USDIMAGINGGL_API
    void SetLightingState(GlfSimpleLightVector const &lights,
                          GlfSimpleMaterial const &material,
                          GfVec4f const &sceneAmbient);

    USDIMAGINGGL_API
    void InvalidateBuffers();

    USDIMAGINGGL_API
    SdfPath GetRprimPathFromPrimId(int primId) const;

    USDIMAGINGGL_API
    bool TestIntersection(
        const GfMatrix4d &viewMatrix,
        const GfMatrix4d &projectionMatrix,
        const GfMatrix4d &worldToLocalSpace,
        const UsdPrim& root,
        const UsdImagingGLRenderParams& params,
        GfVec3d *outHitPoint,
        SdfPath *outHitPrimPath = NULL,
        SdfPath *outInstancerPath = NULL,
        int *outHitInstanceIndex = NULL,
        int *outHitElementIndex = NULL);

private:
    bool _SupportsPrimitiveRestartIndex();

private:

    // Extracts all data necessary for drawing the stage.
    void _TraverseStage(const UsdPrim& root);

    // Update GL camera state
    void _ResolveCamera();
    void _UpdateGLCameraFramingState(const GfMatrix4d& viewMatrix,
                                     const GfMatrix4d& projectionMatrix,
                                     const GfVec4d& viewport);

    // Common logic for extracting color information for all gprims.
    void _ProcessGprimColor(const UsdGeomGprim *gprimSchema,
                            const UsdPrim &prim,
                            bool *doubleSided,
                            VtArray<GfVec3f> *color,
                            TfToken *interpolation);

    // Standard geometry callbacks
    void _HandleXform(const UsdPrim &prim);
    void _HandleMesh(const UsdPrim &prim);
    void _HandleCurves(const UsdPrim& prim);
    void _HandleCube(const UsdPrim &prim);
    void _HandleSphere(const UsdPrim &prim);
    void _HandleCone(const UsdPrim &prim);
    void _HandleCylinder(const UsdPrim &prim);
    void _HandleCapsule(const UsdPrim &prim);
    void _HandlePoints(const UsdPrim &prim);
    void _HandleNurbsPatch(const UsdPrim &prim);
    void _RenderPrimitive(const UsdPrim &prim, const UsdGeomGprim *gprimSchema, 
                          const VtArray<GfVec3f> &pts, const VtIntArray &nmvts, 
                          const VtIntArray &vts);

    // Generates GPU buffers for raw float and index data.
    void _PopulateBuffers();

    // Sets up and issues draw call(s) for polygons.
    // When drawID is true, draw with ID color instead of vertex color.
    void _DrawPolygons(bool drawID);

    // Sets up and issues draw call(s) for lines.
    // When drawID is true, draw with ID color instead of vertex color.
    void _DrawLines(bool drawID);

    // Allocates a new ID and color and associates it with the given path,
    // returns the ID color.
    GfVec4f _IssueID(SdfPath const& path);

    // Append a copy of the ID to the ID color buffer (one per vertex).
    void _AppendIDColor(GfVec4f const& ID, std::vector<GLfloat>* vec) {
        vec->push_back(ID[0]);
        vec->push_back(ID[1]);
        vec->push_back(ID[2]);
    }

    // USD Change notice handler.
    void _OnObjectsChanged(UsdNotice::ObjectsChanged const&,
                           UsdStageWeakPtr const& sender);

private:
    UsdImagingGLRenderParams _params;

    GfMatrix4d _ctm;

    std::vector<std::pair<UsdPrim, GfMatrix4d> > _xformStack;

    // Raw float data for all points to be drawn as polygons.
    std::vector<GLfloat> _points;

    // Raw float data for all normals on polygons (1 per point).
    std::vector<GLfloat> _normals;

    // Raw float data for all polygon colors (1 per point).
    std::vector<GLfloat> _colors;
    std::vector<GLfloat> _IDColors;

    // Indexes into _points to define polygons to be drawn.
    std::vector<GLuint> _verts;

    // The number of points on each polygon-- not needed if prim restart is
    // supported.
    std::vector<GLuint> _numVerts;

    // The byte-offsets into the element array buffer indicating the start of
    // each polygon-- not needed if prim restart is supported.
    std::vector<GLvoid*> _vertIdxOffsets;

    // A rolling count of points, to assist in providing buffer offsets for the
    // raw data of all meshes.
    int _vertCount;

    // Raw float data for all points to be drawn as lines.
    std::vector<GLfloat> _linePoints;

    // Raw float data for all line colors (1 per point).
    std::vector<GLfloat> _lineColors;
    std::vector<GLfloat> _lineIDColors;

    // Indexes into _linePoints to define individual segments. If prim restart
    // is supported, each segment will be separated by a prim restart index;
    // otherwise we need to manually account for each segment and break up the
    // draw calls accordingly.
    std::vector<GLuint> _lineVerts;

    // The number of points on each line segment-- not needed if prim restart is
    // supported.
    std::vector<GLuint> _numLineVerts;

    // The byte-offsets into the element array buffer indicating the start of
    // each line segment-- not needed if prim restart is supported.
    std::vector<GLvoid*> _lineVertIdxOffsets;

    // A rolling count of points, to assist in providing buffer offsets for the
    // raw data of all lines.
    int _lineVertCount;

    // The identifiers for the physical buffers on the card.
    GLuint _attribBuffer;
    GLuint _indexBuffer;

    UsdPrim _root;

    TfHashSet<SdfPath, SdfPath::Hash> _excludedSet;

    GfMatrix4d _freeCamViewMatrix;
    GfMatrix4d _freeCamProjMatrix;
    GfVec4d _viewport;
    CameraUtilConformWindowPolicy _windowPolicy;
    bool _usingSceneCam;
    SdfPath _sceneCamId;
    UsdPrim _sceneCam;

    // Define a type for PrimID along with some helper methods to pack a vec4i
    // into a single value (and the inverse).
    struct _PrimID {
        typedef int32_t ValueType;

        static ValueType Pack(GfVec4i primIdColor) {
            ValueType primID = {((primIdColor[0] & 0xff) <<  0) |
                                ((primIdColor[1] & 0xff) <<  8) |
                                ((primIdColor[2] & 0xff) << 16) };
            return primID;
        }

        static GfVec4f Unpack(ValueType primId) {
            GfVec4f primIdColor;                                                     
            // Convert the instance Id into an rbg color. We could potentially
            // use alpha if we want to extend the range.
            primIdColor[0] = ((float)((primId & 0x000000ff) >>  0)) / 255.0f;
            primIdColor[1] = ((float)((primId & 0x0000ff00) >>  8)) / 255.0f;
            primIdColor[2] = ((float)((primId & 0x00ff0000) >> 16)) / 255.0f;
            primIdColor[3] = 1;

            return primIdColor;
        }
    };

    // A running counter of prim IDs.
    _PrimID::ValueType _primIDCounter;

    // A mapping from ID to SdfPath, used for picking.
    typedef TfHashMap<_PrimID::ValueType, SdfPath> _PrimIDMap;
    _PrimIDMap _primIDMap;

    // For changes from UsdStage.
    TfNotice::Key _objectsChangedNoticeKey;

    typedef boost::unordered_map<GlfGLContextSharedPtr, GlfDrawTargetRefPtr> _DrawTargetPerContextMap;
    _DrawTargetPerContextMap _drawTargets;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_GL_LEGACY_ENGINE_H
