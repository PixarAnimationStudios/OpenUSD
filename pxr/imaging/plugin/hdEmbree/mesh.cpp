//
// Copyright 2017 Pixar
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
#include "pxr/imaging/hdEmbree/mesh.h"

#include "pxr/imaging/hdEmbree/context.h"
#include "pxr/imaging/hdEmbree/instancer.h"
#include "pxr/imaging/hdEmbree/renderParam.h"
#include "pxr/imaging/hdEmbree/renderPass.h"
#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/pxOsd/tokens.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

PXR_NAMESPACE_OPEN_SCOPE

HdEmbreeMesh::HdEmbreeMesh(SdfPath const& id,
                           SdfPath const& instancerId)
    : HdMesh(id, instancerId)
    , _rtcMeshId(RTC_INVALID_GEOMETRY_ID)
    , _rtcMeshScene(nullptr)
    , _adjacencyValid(false)
    , _normalsValid(false)
    , _refined(false)
    , _smoothNormals(false)
    , _doubleSided(false)
    , _cullStyle(HdCullStyleDontCare)
{
}

void
HdEmbreeMesh::Finalize(HdRenderParam *renderParam)
{
    RTCScene scene = static_cast<HdEmbreeRenderParam*>(renderParam)
        ->GetEmbreeScene();
    // Delete any instances of this mesh in the top-level embree scene.
    for (size_t i = 0; i < _rtcInstanceIds.size(); ++i) {
        // Delete the instance context first...
        delete _GetInstanceContext(scene, i);
        // ...then the instance object in the top-level scene.
        rtcDeleteGeometry(scene, _rtcInstanceIds[i]);
    }
    _rtcInstanceIds.clear();

    // Delete the prototype geometry and the prototype scene.
    if (_rtcMeshScene != nullptr) {
        if (_rtcMeshId != RTC_INVALID_GEOMETRY_ID) {
            // Delete the prototype context first...
            TF_FOR_ALL(it, _GetPrototypeContext()->primvarMap) {
                delete it->second;
            }
            delete _GetPrototypeContext();
            // ... then the geometry object in the prototype scene...
            rtcDeleteGeometry(_rtcMeshScene, _rtcMeshId);
        }
        // ... then the prototype scene.
        rtcDeleteScene(_rtcMeshScene);
    }
    _rtcMeshId = RTC_INVALID_GEOMETRY_ID;
    _rtcMeshScene = nullptr;
}

HdDirtyBits
HdEmbreeMesh::_GetInitialDirtyBits() const
{
    // The initial dirty bits control what data is available on the first
    // run through _PopulateRtMesh(), so it should list every data item
    // that _PopulateRtMesh requests.
    int mask = HdChangeTracker::Clean
        | HdChangeTracker::InitRepr
        | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyTopology
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyCullStyle
        | HdChangeTracker::DirtyDoubleSided
        | HdChangeTracker::DirtyRefineLevel
        | HdChangeTracker::DirtySubdivTags
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyNormals
        | HdChangeTracker::DirtyInstanceIndex
        ;

    return (HdDirtyBits)mask;
}

HdDirtyBits
HdEmbreeMesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

void
HdEmbreeMesh::_InitRepr(TfToken const &reprName,
                        HdDirtyBits *dirtyBits)
{
    TF_UNUSED(dirtyBits);

    // Create an empty repr.
    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprName));
    if (it == _reprs.end()) {
        _reprs.emplace_back(reprName, HdReprSharedPtr());
    }
}


void
HdEmbreeMesh::_UpdateRepr(HdSceneDelegate *sceneDelegate,
                          TfToken const &reprName,
                          HdDirtyBits *dirtyBits)
{
    TF_UNUSED(sceneDelegate);
    TF_UNUSED(reprName);
    TF_UNUSED(dirtyBits);
    // Embree doesn't use the HdRepr structure.
}

void
HdEmbreeMesh::Sync(HdSceneDelegate* sceneDelegate,
                   HdRenderParam*   renderParam,
                   HdDirtyBits*     dirtyBits,
                   TfToken const&   reprName,
                   bool             forcedRepr)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // The repr token is used to look up an HdMeshReprDesc struct, which
    // has drawing settings for this prim to use. Repr opinions can come
    // from the render pass's rprim collection or the scene delegate;
    // _GetReprName resolves these multiple opinions.
    TfToken calculatedReprName = _GetReprName(reprName, forcedRepr);

    // XXX: Meshes can have multiple reprs; this is done, for example, when
    // the drawstyle specifies different rasterizing modes between front faces
    // and back faces. With raytracing, this concept makes less sense, but
    // combining semantics of two HdMeshReprDesc is tricky in the general case.
    // For now, HdEmbreeMesh only respects the first desc; this should be fixed.
    _MeshReprConfig::DescArray descs = _GetReprDesc(calculatedReprName);
    const HdMeshReprDesc &desc = descs[0];

    // Pull top-level embree state out of the render param.
    RTCScene scene = static_cast<HdEmbreeRenderParam*>(renderParam)
		->GetEmbreeScene();
    RTCDevice device = static_cast<HdEmbreeRenderParam*>(renderParam)
        ->GetEmbreeDevice();

    // Create embree geometry objects.
    _PopulateRtMesh(sceneDelegate, scene, device, dirtyBits, desc);
}

/* static */
void
HdEmbreeMesh::_EmbreeCullFaces(void *userData, RTCRay &ray)
{
    // Note: this is called to filter every candidate ray hit
    // with the bound object, so this function should be fast.

    // Only HdEmbreeMesh gets HdEmbreeMesh::_EmbreeCullFaces bound
    // as an intersection filter. The filter is bound to the prototype,
    // whose context's rprim always points back to the original HdEmbreeMesh.
    HdEmbreePrototypeContext *ctx =
        static_cast<HdEmbreePrototypeContext*>(userData);
    HdEmbreeMesh *mesh = static_cast<HdEmbreeMesh*>(ctx->rprim);

    // Calculate whether the provided hit is a front-face or back-face.
    bool isFrontFace = (ray.Ng[0] * ray.dir[0] + 
                        ray.Ng[1] * ray.dir[1] +
                        ray.Ng[2] * ray.dir[2]) > 0;

    // Determine if we should ignore this hit. HdCullStyleBack means
    // cull back faces.
    bool cull = false;
    switch(mesh->_cullStyle) {
        case HdCullStyleBack:
            cull = !isFrontFace; break;
        case HdCullStyleFront:
            cull =  isFrontFace; break;

        case HdCullStyleBackUnlessDoubleSided:
            cull = !isFrontFace && !mesh->_doubleSided; break;
        case HdCullStyleFrontUnlessDoubleSided:
            cull =  isFrontFace && !mesh->_doubleSided; break;

        default: break;
    }
    if (cull) {
        // Setting ray.geomId to null tells embree to discard this hit and
        // keep tracing...
        ray.geomID = RTC_INVALID_GEOMETRY_ID;
    }
}

void
HdEmbreeMesh::_CreateEmbreeSubdivMesh(RTCScene scene)
{
    const PxOsdSubdivTags &subdivTags = _topology.GetSubdivTags();

    // The embree edge crease buffer expects ungrouped edges: a pair
    // of indices marking an edge and one weight per crease.
    // HdMeshTopology stores edge creases compactly. A crease length
    // buffer stores the number of indices per crease and groups the
    // crease index buffer, much like the face buffer groups the vertex index
    // buffer except that creases don't automatically close. Crease weights
    // can be specified per crease or per individual edge.
    //
    // For example, to add the edges [v0->v1@2.0f] and [v1->v2@2.0f],
    // HdMeshTopology might store length = [3], indices = [v0, v1, v2],
    // and weight = [2.0f], or it might store weight = [2.0f, 2.0f].
    //
    // This loop calculates the number of edge creases, in preparation for
    // unrolling the edge crease buffer below.
    VtIntArray const creaseLengths = subdivTags.GetCreaseLengths();
    int numEdgeCreases = 0;
    for (size_t i = 0; i < creaseLengths.size(); ++i) {
        numEdgeCreases += creaseLengths[i] - 1;
    }

    // For vertex creases, sanity check that the weights and indices
    // arrays are the same length.
    int numVertexCreases =
        static_cast<int>(subdivTags.GetCornerIndices().size());
    if (numVertexCreases !=
            static_cast<int>(subdivTags.GetCornerWeights().size())) {
        TF_WARN("Mismatch between vertex crease indices and weights");
        numVertexCreases = 0;
    }

    // Populate an embree subdiv object.
    _rtcMeshId = rtcNewSubdivisionMesh(scene, RTC_GEOMETRY_DEFORMABLE,
        // numFaces is the size of RTC_FACE_BUFFER, which contains
        // the number of indices for each face. This is equivalent to
        // HdMeshTopology's GetFaceVertexCounts().
        _topology.GetFaceVertexCounts().size(),
        // numEdges is the size of RTC_INDEX_BUFFER, which contains
        // the vertex indices for each face (grouped into faces by the
        // face buffer). This is equivalent to HdMeshTopology's
        // GetFaceVertexIndices(). Note this is more properly a count
        // of half-edges.
        _topology.GetFaceVertexIndices().size(),
        // numVertices is the size of RTC_VERTEX_BUFFER, or vertex
        // positions.
        _points.size(),
        // numEdgeCreases is the size of RTC_EDGE_CREASE_WEIGHT_BUFFER,
        // and half the size of RTC_EDGE_CREASE_INDEX_BUFFER. See
        // the calculation of numEdgeCreases above.
        numEdgeCreases,
        // numVertexCreases is the size of
        // RTC_VERTEX_CREASE_WEIGHT_BUFFER and _INDEX_BUFFER.
        numVertexCreases,
        // numHoles is the size of RTC_HOLE_BUFFER.
        _topology.GetHoleIndices().size());

    // Fill the topology buffers.
    rtcSetBuffer(scene, _rtcMeshId, RTC_FACE_BUFFER,
        _topology.GetFaceVertexCounts().cdata(), 0, sizeof(int));
    rtcSetBuffer(scene, _rtcMeshId, RTC_INDEX_BUFFER,
        _topology.GetFaceVertexIndices().cdata(), 0, sizeof(int));
    rtcSetBuffer(scene, _rtcMeshId, RTC_HOLE_BUFFER,
        _topology.GetHoleIndices().cdata(), 0, sizeof(int));

    // If this topology has edge creases, unroll the edge crease buffer.
    if (numEdgeCreases > 0) {
        int *embreeCreaseIndices = static_cast<int*>(rtcMapBuffer(
            scene, _rtcMeshId, RTC_EDGE_CREASE_INDEX_BUFFER));
        float *embreeCreaseWeights = static_cast<float*>(rtcMapBuffer(
            scene, _rtcMeshId, RTC_EDGE_CREASE_WEIGHT_BUFFER));
        int embreeEdgeIndex = 0;

        VtIntArray const creaseIndices = subdivTags.GetCreaseIndices();
        VtFloatArray const creaseWeights =
            subdivTags.GetCreaseWeights();

        bool weightPerCrease =
            (creaseWeights.size() == creaseLengths.size());

        // Loop through the creases; for each crease, loop through
        // the edges.
        int creaseIndexStart = 0;
        for (size_t i = 0; i < creaseLengths.size(); ++i) {
            int numEdges = creaseLengths[i] - 1;
            for(int j = 0; j < numEdges; ++j) {
                // Store the crease indices.
                embreeCreaseIndices[2*embreeEdgeIndex+0] =
                    creaseIndices[creaseIndexStart+j];
                embreeCreaseIndices[2*embreeEdgeIndex+1] =
                    creaseIndices[creaseIndexStart+j+1];

                // Store the crease weight.
                embreeCreaseWeights[embreeEdgeIndex] = weightPerCrease ?
                    creaseWeights[i] : creaseWeights[embreeEdgeIndex];

                embreeEdgeIndex++;
            }
            creaseIndexStart += creaseLengths[i];
        }

        rtcUnmapBuffer(scene, _rtcMeshId, RTC_EDGE_CREASE_INDEX_BUFFER);
        rtcUnmapBuffer(scene, _rtcMeshId, RTC_EDGE_CREASE_WEIGHT_BUFFER);
    }

    if (numVertexCreases > 0) {
        rtcSetBuffer(scene, _rtcMeshId, RTC_VERTEX_CREASE_INDEX_BUFFER,
            subdivTags.GetCornerIndices().cdata(), 0, sizeof(int));
        rtcSetBuffer(scene, _rtcMeshId, RTC_VERTEX_CREASE_WEIGHT_BUFFER,
            subdivTags.GetCornerWeights().cdata(), 0, sizeof(float));
    }
}

void
HdEmbreeMesh::_CreateEmbreeTriangleMesh(RTCScene scene)
{
    // Triangulate the input faces.
    HdMeshUtil meshUtil(&_topology, GetId());
    meshUtil.ComputeTriangleIndices(&_triangulatedIndices,
        &_trianglePrimitiveParams);

    // Create the new mesh.
    _rtcMeshId = rtcNewTriangleMesh(scene, RTC_GEOMETRY_DEFORMABLE,
            _triangulatedIndices.size(), _points.size());
    if (_rtcMeshId == RTC_INVALID_GEOMETRY_ID) {
        TF_CODING_ERROR("Couldn't create RTC mesh");
        return;
    }

    // Populate topology.
    rtcSetBuffer(scene, _rtcMeshId, RTC_INDEX_BUFFER,
        _triangulatedIndices.cdata(), 0, sizeof(GfVec3i));
}

void
HdEmbreeMesh::_UpdatePrimvarSources(HdSceneDelegate* sceneDelegate,
                                    HdDirtyBits dirtyBits)
{
    HD_TRACE_FUNCTION();
    SdfPath const& id = GetId();

    // Update _primvarSourceMap, our local cache of raw primvar data.
    // This function pulls data from the scene delegate, but defers processing.
    //
    // While iterating primvars, we skip "points" (vertex positions) because
    // the points primvar is processed by _PopulateRtMesh. We only call
    // GetPrimvar on primvars that have been marked dirty.
    //
    // Currently, hydra doesn't have a good way of communicating changes in
    // the set of primvars, so we only ever add and update to the primvar set.

    TfTokenVector names = GetPrimvarVertexNames(sceneDelegate);
    TF_FOR_ALL(nameIt, names) {
        if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, *nameIt) &&
            *nameIt != HdTokens->points) {
            _primvarSourceMap[*nameIt] = {
                GetPrimvar(sceneDelegate, *nameIt),
                HdInterpolationVertex
            };
        }
    }
    names = GetPrimvarVaryingNames(sceneDelegate);
    TF_FOR_ALL(nameIt, names) {
        if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, *nameIt) &&
            *nameIt != HdTokens->points) {
            _primvarSourceMap[*nameIt] = {
                GetPrimvar(sceneDelegate, *nameIt),
                HdInterpolationVarying
            };
        }
    }
    names = GetPrimvarFacevaryingNames(sceneDelegate);
    TF_FOR_ALL(nameIt, names) {
        if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, *nameIt) &&
            *nameIt != HdTokens->points) {
            _primvarSourceMap[*nameIt] = {
                GetPrimvar(sceneDelegate, *nameIt),
                HdInterpolationFaceVarying
            };
        }
    }
    names = GetPrimvarUniformNames(sceneDelegate);
    TF_FOR_ALL(nameIt, names) {
        if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, *nameIt) &&
            *nameIt != HdTokens->points) {
            _primvarSourceMap[*nameIt] = {
                GetPrimvar(sceneDelegate, *nameIt),
                HdInterpolationUniform
            };
        }
    }
    names = GetPrimvarConstantNames(sceneDelegate);
    TF_FOR_ALL(nameIt, names) {
        if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, *nameIt) &&
            *nameIt != HdTokens->points) {
            _primvarSourceMap[*nameIt] = {
                GetPrimvar(sceneDelegate, *nameIt),
                HdInterpolationConstant
            };
        }
    }
}

void
HdEmbreeMesh::_CreatePrimvarSampler(TfToken const& name, VtValue const& data,
                                    HdInterpolation interpolation,
                                    bool refined)
{
    // Delete the old sampler, if it exists.
    HdEmbreePrototypeContext *ctx = _GetPrototypeContext();
    if (ctx->primvarMap.count(name) > 0) {
        delete ctx->primvarMap[name];
    }
    ctx->primvarMap.erase(name);

    // Construct the correct type of sampler from the interpolation mode and
    // geometry mode.
    HdEmbreePrimvarSampler *sampler = nullptr;
    switch(interpolation) {
        case HdInterpolationConstant:
            sampler = new HdEmbreeConstantSampler(name, data);
            break;
        case HdInterpolationUniform:
            if (refined) {
                sampler = new HdEmbreeUniformSampler(name, data);
            } else {
                sampler = new HdEmbreeUniformSampler(name, data,
                    _trianglePrimitiveParams);
            }
            break;
        case HdInterpolationVertex:
            if (refined) {
                sampler = new HdEmbreeSubdivVertexSampler(name, data,
                    _rtcMeshScene, _rtcMeshId, &_embreeBufferAllocator);
            } else {
                sampler = new HdEmbreeTriangleVertexSampler(name, data,
                    _triangulatedIndices);
            }
            break;
        case HdInterpolationVarying:
            if (refined) {
                // XXX: Fixme! This isn't strictly correct, as "varying" in
                // the context of subdiv meshes means bilinear interpolation,
                // not reconstruction from the subdivision basis.
                sampler = new HdEmbreeSubdivVertexSampler(name, data,
                    _rtcMeshScene, _rtcMeshId, &_embreeBufferAllocator);
            } else {
                sampler = new HdEmbreeTriangleVertexSampler(name, data,
                    _triangulatedIndices);
            }
            break;
        case HdInterpolationFaceVarying:
            if (refined) {
                // XXX: Fixme! HdEmbree doesn't currently support face-varying
                // primvars on subdivision meshes.
                TF_WARN("HdEmbreeMesh doesn't support face-varying primvars"
                        " on refined meshes.");
            } else {
                HdMeshUtil meshUtil(&_topology, GetId());
                sampler = new HdEmbreeTriangleFaceVaryingSampler(name, data,
                    meshUtil);
            }
            break;
        default:
            TF_CODING_ERROR("Unrecognized interpolation mode");
            break;
    }

    // Put the new sampler back in the primvar map.
    if (sampler != nullptr) {
        ctx->primvarMap[name] = sampler;
    }
}

void
HdEmbreeMesh::_PopulateRtMesh(HdSceneDelegate* sceneDelegate,
                              RTCScene         scene,
                              RTCDevice        device,
                              HdDirtyBits*     dirtyBits,
                              HdMeshReprDesc const &desc)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();

    ////////////////////////////////////////////////////////////////////////
    // 1. Pull scene data.

    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->points)) {
        VtValue value = sceneDelegate->Get(id, HdTokens->points);
        _points = value.Get<VtVec3fArray>();
        _normalsValid = false;
    }

    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id)) {
        // When pulling a new topology, we don't want to overwrite the
        // refine level or subdiv tags, which are provided separately by the
        // scene delegate, so we save and restore them.
        PxOsdSubdivTags subdivTags = _topology.GetSubdivTags();
        int refineLevel = _topology.GetRefineLevel();
        _topology = HdMeshTopology(GetMeshTopology(sceneDelegate), refineLevel);
        _topology.SetSubdivTags(subdivTags);
        _adjacencyValid = false;
    }
    if (HdChangeTracker::IsSubdivTagsDirty(*dirtyBits, id)) {
        _topology.SetSubdivTags(sceneDelegate->GetSubdivTags(id));
    }
    if (HdChangeTracker::IsRefineLevelDirty(*dirtyBits, id)) {
        _topology = HdMeshTopology(_topology,
            sceneDelegate->GetRefineLevel(id));
    }

    if (HdChangeTracker::IsTransformDirty(*dirtyBits, id)) {
        _transform = GfMatrix4f(sceneDelegate->GetTransform(id));
    }

    if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, id)) {
        _UpdateVisibility(sceneDelegate, dirtyBits);
    }

    if (HdChangeTracker::IsCullStyleDirty(*dirtyBits, id)) {
        _cullStyle = GetCullStyle(sceneDelegate);
    }
    if (HdChangeTracker::IsDoubleSidedDirty(*dirtyBits, id)) {
        _doubleSided = IsDoubleSided(sceneDelegate);
    }
    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->normals) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->widths) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->primvar)) {
        _UpdatePrimvarSources(sceneDelegate, *dirtyBits);
    }

    ////////////////////////////////////////////////////////////////////////
    // 2. Resolve drawstyles

    // The repr defines a set of geometry styles for drawing the mesh
    // (see hd/enums.h). We're ignoring points and wireframe for now, so
    // HdMeshGeomStyleSurf maps to subdivs and everything else maps to
    // HdMeshGeomStyleHull (coarse triangulated mesh).
    bool doRefine = (desc.geomStyle == HdMeshGeomStyleSurf);

    // If the subdivision scheme is "none", force us to not refine.
    doRefine = doRefine && (_topology.GetScheme() != PxOsdOpenSubdivTokens->none);

    // If the refine level is 0, triangulate instead of subdividing.
    doRefine = doRefine && (_topology.GetRefineLevel() > 0);

    // The repr defines whether we should compute smooth normals for this mesh:
    // per-vertex normals taken as an average of adjacent faces, and
    // interpolated smoothly across faces.
    _smoothNormals = desc.smoothNormals;

    // If the subdivision scheme is "none" or "bilinear", force us not to use
    // smooth normals.
    _smoothNormals = _smoothNormals &&
        (_topology.GetScheme() != PxOsdOpenSubdivTokens->none) &&
        (_topology.GetScheme() != PxOsdOpenSubdivTokens->bilinear);

    // If the scene delegate has provided authored normals, force us to not use
    // smooth normals.
    bool authoredNormals = false;
    if (_primvarSourceMap.count(HdTokens->normals) > 0) {
        authoredNormals = true;
    }
    _smoothNormals = _smoothNormals && !authoredNormals;

    ////////////////////////////////////////////////////////////////////////
    // 3. Populate embree prototype object.

    // If the topology has changed, or the value of doRefine has changed, we
    // need to create or recreate the embree mesh object.
    // _GetInitialDirtyBits() ensures that the topology is dirty the first time
    // this function is called, so that the embree mesh is always created.
    bool newMesh = false;
    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id) ||
        doRefine != _refined) {

        newMesh = true;

        // Destroy the old mesh, if it exists.
        if (_rtcMeshScene != nullptr &&
            _rtcMeshId != RTC_INVALID_GEOMETRY_ID) {
            // Delete the prototype context first...
            TF_FOR_ALL(it, _GetPrototypeContext()->primvarMap) {
                delete it->second;
            }
            delete _GetPrototypeContext();
            // then the prototype geometry.
            rtcDeleteGeometry(_rtcMeshScene, _rtcMeshId);
            _rtcMeshId = RTC_INVALID_GEOMETRY_ID;
        }

        // Create the prototype mesh scene, if it doesn't exist yet.
        if (_rtcMeshScene == nullptr) {
            _rtcMeshScene = rtcDeviceNewScene(device, RTC_SCENE_DYNAMIC,
                RTC_INTERSECT1 | RTC_INTERPOLATE);
        }

        // Populate either a subdiv or a triangle mesh object. The helper
        // functions will take care of populating topology buffers.
        if (doRefine) {
            _CreateEmbreeSubdivMesh(_rtcMeshScene);
        } else {
            _CreateEmbreeTriangleMesh(_rtcMeshScene);
        }
        _refined = doRefine;
        // In both cases, RTC_VERTEX_BUFFER will be populated below.

        // Prototype geometry gets tagged with a prototype context, that the
        // ray-hit algorithm can use to look up data.
        rtcSetUserData(_rtcMeshScene, _rtcMeshId,
            new HdEmbreePrototypeContext);
        _GetPrototypeContext()->rprim = this;

        // Add _EmbreeCullFaces as a filter function for backface culling.
        rtcSetIntersectionFilterFunction(_rtcMeshScene, _rtcMeshId,
            _EmbreeCullFaces);
        rtcSetOcclusionFilterFunction(_rtcMeshScene, _rtcMeshId,
            _EmbreeCullFaces);

        // Force the smooth normals code to rebuild the "normals" primvar the
        // next time smooth normals is enabled.
        _normalsValid = false;
    }

    // If the refine level changed or the mesh was recreated, we need to pass
    // the refine level into the embree subdiv object.
    if (newMesh || HdChangeTracker::IsRefineLevelDirty(*dirtyBits, id)) {
        if (doRefine) {
            // Pass the target number of uniform refinements to Embree.
            // Embree refinement is specified as the number of quads to generate
            // per edge, whereas hydra refinement is the number of recursive
            // splits, so we need to pass embree (2^refineLevel).

            int tessellationRate = (1 << _topology.GetRefineLevel());
            // XXX: As of Embree 2.9.0, rendering with tessellation level 1
            // (i.e. coarse mesh) results in weird normals, so force at least
            // one level of subdivision.
            if (tessellationRate == 1) {
                tessellationRate++;
            }
            rtcSetTessellationRate(_rtcMeshScene, _rtcMeshId,
                static_cast<float>(tessellationRate));
        }
    }

    // If the subdiv tags changed or the mesh was recreated, we need to update
    // the subdivision boundary mode.
    if (newMesh || HdChangeTracker::IsSubdivTagsDirty(*dirtyBits, id)) {
        if (doRefine) {
            TfToken const vertexRule =
                _topology.GetSubdivTags().GetVertexInterpolationRule();
            if (vertexRule == PxOsdOpenSubdivTokens->none) {
                rtcSetBoundaryMode(_rtcMeshScene, _rtcMeshId,
                    RTC_BOUNDARY_NONE);
            } else if (vertexRule == PxOsdOpenSubdivTokens->edgeOnly) {
                rtcSetBoundaryMode(_rtcMeshScene, _rtcMeshId,
                    RTC_BOUNDARY_EDGE_ONLY);
            } else if (vertexRule == PxOsdOpenSubdivTokens->edgeAndCorner) {
                rtcSetBoundaryMode(_rtcMeshScene, _rtcMeshId,
                    RTC_BOUNDARY_EDGE_AND_CORNER);
            } else {
                TF_WARN("Unknown vertex interpolation rule: %s",
                    vertexRule.GetText());
            }
        }
    }

    // Update the smooth normals in steps:
    // 1. If the topology is dirty, update the adjacency table, a processed
    //    form of the topology that helps calculate smooth normals quickly.
    // 2. If the points are dirty, update the smooth normal buffer itself.
    if (_smoothNormals && !_adjacencyValid) {
        _adjacency.BuildAdjacencyTable(&_topology);
        _adjacencyValid = true;
        // If we rebuilt the adjacency table, force a rebuild of normals.
        _normalsValid = false;
    }
    if (_smoothNormals && !_normalsValid) {
        _computedNormals = _adjacency.ComputeSmoothNormals(_points.size(),
            _points.cdata());
        _normalsValid = true;

        // Create a sampler for the "normals" primvar. If there are authored
        // normals, the smooth normals flag has been suppressed, so it won't
        // be overwritten by the primvar population below.
        _CreatePrimvarSampler(HdTokens->normals, VtValue(_computedNormals),
            HdInterpolationVertex, _refined);
    }

    // If smooth normals are off and there are no authored normals, make sure
    // there's no "normals" sampler so the renderpass can use its fallback
    // behavior.
    if (!_smoothNormals && !authoredNormals) {
        HdEmbreePrototypeContext *ctx = _GetPrototypeContext();
        if (ctx->primvarMap.count(HdTokens->normals) > 0) {
            delete ctx->primvarMap[HdTokens->normals];
        }
        ctx->primvarMap.erase(HdTokens->normals);

        // Force the smooth normals code to rebuild the "normals" primvar the
        // next time smooth normals is enabled.
        _normalsValid = false;
    }

    // Populate primvars if they've changed or we recreated the mesh.
    TF_FOR_ALL(it, _primvarSourceMap) {
        if (newMesh ||
            HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, it->first)) {
            _CreatePrimvarSampler(it->first, it->second.data,
                    it->second.interpolation, _refined);
        }
    }

    // Populate points in the RTC mesh.
    if (newMesh || 
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->points)) {
        rtcSetBuffer(_rtcMeshScene, _rtcMeshId, RTC_VERTEX_BUFFER,
            _points.cdata(), 0, sizeof(GfVec3f));
    }

    // Update visibility by pulling the object into/out of the embree BVH.
    if (_sharedData.visible) {
        rtcEnable(_rtcMeshScene, _rtcMeshId);
    } else {
        rtcDisable(_rtcMeshScene, _rtcMeshId);
    }

    // Mark embree objects dirty and rebuild the bvh.
    if (newMesh ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->points)) {
        rtcUpdate(_rtcMeshScene, _rtcMeshId);
    }
    rtcCommit(_rtcMeshScene);

    ////////////////////////////////////////////////////////////////////////
    // 4. Populate embree instance objects.

    // If the mesh is instanced, create one new instance per transform.
    // XXX: The current instancer invalidation tracking makes it hard for
    // HdEmbree to tell whether transforms will be dirty, so this code
    // pulls them every frame.
    if (!GetInstancerId().IsEmpty()) {

        // Retrieve instance transforms from the instancer.
        HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
        HdInstancer *instancer =
            renderIndex.GetInstancer(GetInstancerId());
        VtMatrix4dArray transforms =
            static_cast<HdEmbreeInstancer*>(instancer)->
                ComputeInstanceTransforms(GetId());

        size_t oldSize = _rtcInstanceIds.size();
        size_t newSize = transforms.size();

        // Size down (if necessary).
        for(size_t i = newSize; i < oldSize; ++i) {
            // Delete instance context first...
            delete _GetInstanceContext(scene, i);
            // Then Embree instance.
            rtcDeleteGeometry(scene, _rtcInstanceIds[i]);
        }
        _rtcInstanceIds.resize(newSize);

        // Size up (if necessary).
        for(size_t i = oldSize; i < newSize; ++i) {
            // Create the new instance.
            _rtcInstanceIds[i] = rtcNewInstance(scene, _rtcMeshScene);
            // Create the instance context.
            HdEmbreeInstanceContext *ctx = new HdEmbreeInstanceContext;
            ctx->rootScene = _rtcMeshScene;
            rtcSetUserData(scene, _rtcInstanceIds[i], ctx);
        }

        // Update transforms.
        for (size_t i = 0; i < transforms.size(); ++i) {
            // Combine the local transform and the instance transform.
            GfMatrix4f matf = _transform * GfMatrix4f(transforms[i]);
            // Update the transform in the BVH.
            rtcSetTransform(scene, _rtcInstanceIds[i],
                RTC_MATRIX_COLUMN_MAJOR_ALIGNED16, matf.GetArray());
            // Update the transform in the instance context.
            _GetInstanceContext(scene, i)->objectToWorldMatrix = matf;
            // Mark the instance as updated in the BVH.
            rtcUpdate(scene, _rtcInstanceIds[i]);
        }
    }
    // Otherwise, create our single instance (if necessary) and update
    // the transform (if necessary).
    else {
        bool newInstance = false;
        if (_rtcInstanceIds.size() == 0) {
            // Create our single instance.
            _rtcInstanceIds.push_back(rtcNewInstance(scene, _rtcMeshScene));
            // Create the instance context.
            HdEmbreeInstanceContext *ctx = new HdEmbreeInstanceContext;
            ctx->rootScene = _rtcMeshScene;
            rtcSetUserData(scene, _rtcInstanceIds[0], ctx);
            // Update the flag to force-set the transform.
            newInstance = true;
        }
        if (newInstance || HdChangeTracker::IsTransformDirty(*dirtyBits, id)) {
            // Update the transform in the BVH.
            rtcSetTransform(scene, _rtcInstanceIds[0],
                RTC_MATRIX_COLUMN_MAJOR_ALIGNED16, _transform.GetArray());
            // Update the transform in the render context.
            _GetInstanceContext(scene, 0)->objectToWorldMatrix = _transform;
        }
        if (newInstance || newMesh ||
            HdChangeTracker::IsTransformDirty(*dirtyBits, id) ||
            HdChangeTracker::IsPrimvarDirty(*dirtyBits, id,
                                            HdTokens->points)) {
            // Mark the instance as updated in the top-level BVH.
            rtcUpdate(scene, _rtcInstanceIds[0]);
        }
    }

    // Clean all dirty bits.
    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

HdEmbreePrototypeContext*
HdEmbreeMesh::_GetPrototypeContext()
{
    return static_cast<HdEmbreePrototypeContext*>(
        rtcGetUserData(_rtcMeshScene, _rtcMeshId));
}

HdEmbreeInstanceContext*
HdEmbreeMesh::_GetInstanceContext(RTCScene scene, size_t i)
{
    return static_cast<HdEmbreeInstanceContext*>(
        rtcGetUserData(scene, _rtcInstanceIds[i]));
}

PXR_NAMESPACE_CLOSE_SCOPE
