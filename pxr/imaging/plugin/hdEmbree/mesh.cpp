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
#include "pxr/imaging/plugin/hdEmbree/mesh.h"

#include "pxr/imaging/plugin/hdEmbree/context.h"
#include "pxr/imaging/plugin/hdEmbree/instancer.h"
#include "pxr/imaging/plugin/hdEmbree/renderParam.h"
#include "pxr/imaging/plugin/hdEmbree/renderPass.h"
#include "pxr/imaging/hd/extComputationUtils.h"
#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/smoothNormals.h"
#include "pxr/imaging/pxOsd/tokens.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include <algorithm> // sort

PXR_NAMESPACE_OPEN_SCOPE

HdEmbreeMesh::HdEmbreeMesh(SdfPath const& id)
    : HdMesh(id)
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
        ->AcquireSceneForEdit();
    // Delete any instances of this mesh in the top-level embree scene.
    for (size_t i = 0; i < _rtcInstanceIds.size(); ++i) {
        // Delete the instance context first...
        delete _GetInstanceContext(scene, i);
        // ...then the instance object in the top-level scene.
        //
        // I think this should probably actually be a detach from the
        // above scene
        //
        rtcDetachGeometry(scene,_rtcInstanceIds[i]);
        rtcReleaseGeometry(_rtcInstanceGeometries[i]);
    }
    _rtcInstanceIds.clear();
    _rtcInstanceGeometries.clear();

    // Delete the prototype geometry and the prototype scene.
    if (_rtcMeshScene != nullptr) {
        if (_rtcMeshId != RTC_INVALID_GEOMETRY_ID) {
            // Delete the prototype context first...
            TF_FOR_ALL(it, _GetPrototypeContext()->primvarMap) {
                delete it->second;
            }
            delete _GetPrototypeContext();
            rtcReleaseGeometry(_geometry);
            _rtcMeshId = RTC_INVALID_GEOMETRY_ID;
        }
        // Note: rtcReleaseScene implicitly detaches the geometry. The
        // geometry is refcounted, and rtcMeshScene will hold the last refcount.
        rtcReleaseScene(_rtcMeshScene);
        _rtcMeshScene = nullptr;
    }
}

HdDirtyBits
HdEmbreeMesh::GetInitialDirtyBitsMask() const
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
        | HdChangeTracker::DirtyDisplayStyle
        | HdChangeTracker::DirtySubdivTags
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyNormals
        | HdChangeTracker::DirtyInstancer
        ;

    return (HdDirtyBits)mask;
}

HdDirtyBits
HdEmbreeMesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

void
HdEmbreeMesh::_InitRepr(TfToken const &reprToken,
                        HdDirtyBits *dirtyBits)
{
    TF_UNUSED(dirtyBits);

    // Create an empty repr.
    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprToken));
    if (it == _reprs.end()) {
        _reprs.emplace_back(reprToken, HdReprSharedPtr());
    }
}

void
HdEmbreeMesh::Sync(HdSceneDelegate *sceneDelegate,
                   HdRenderParam   *renderParam,
                   HdDirtyBits     *dirtyBits,
                   TfToken const   &reprToken)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // XXX: A mesh repr can have multiple repr decs; this is done, for example, 
    // when the drawstyle specifies different rasterizing modes between front
    // faces and back faces.
    // With raytracing, this concept makes less sense, but
    // combining semantics of two HdMeshReprDesc is tricky in the general case.
    // For now, HdEmbreeMesh only respects the first desc; this should be fixed.
    _MeshReprConfig::DescArray descs = _GetReprDesc(reprToken);
    const HdMeshReprDesc &desc = descs[0];

    // Pull top-level embree state out of the render param.
    HdEmbreeRenderParam *embreeRenderParam =
        static_cast<HdEmbreeRenderParam*>(renderParam);
    RTCScene scene = embreeRenderParam->AcquireSceneForEdit();
    RTCDevice device = embreeRenderParam->GetEmbreeDevice();

    // Create embree geometry objects.
    _PopulateRtMesh(sceneDelegate, scene, device, dirtyBits, desc);
}

/* static */
void HdEmbreeMesh::_EmbreeCullFaces(const RTCFilterFunctionNArguments* args)
{
    if ( !args ) {
        // This breaks the Embree API spec so we shouldn't get here.
        TF_CODING_ERROR("_EmbreeCullFaces got NULL args pointer");
        return;
    }

    // Pull out the prototype context.
    // Only HdEmbreeMesh gets HdEmbreeMesh::_EmbreeCullFaces bound
    // as an intersection filter. The filter is bound to the prototype,
    // whose context's rprim always points back to the original HdEmbreeMesh.
    HdEmbreePrototypeContext *ctx =
        static_cast<HdEmbreePrototypeContext*>(args->geometryUserPtr);
    if (!ctx || !ctx->rprim) {
        TF_CODING_ERROR("_EmbreeCullFaces got NULL prototype context");
        return;
    }
    HdEmbreeMesh *mesh = static_cast<HdEmbreeMesh*>(ctx->rprim);

    // Note: this is called to filter every candidate ray hit
    // with the bound object, so this function should be fast.
    for (unsigned int i = 0; i < args->N; ++i) {
        // -1 = valid, 0 = invalid.
        // If it's already been marked invalid, skip our own opinion.
        if (args->valid[i] != -1) {
            continue;
        }

        // Calculate whether the provided hit is a front-face or back-face.
        // This is verbose because of SOA struct access, but it's just
        // dot(hit.Ng, ray.dir).
        bool isFrontFace = (
            RTCHitN_Ng_x(args->hit, args->N, i) *
                RTCRayN_dir_x(args->ray, args->N, i) +
            RTCHitN_Ng_y(args->hit, args->N, i) *
                RTCRayN_dir_y(args->ray, args->N, i) +
            RTCHitN_Ng_z(args->hit, args->N, i) *
                RTCRayN_dir_z(args->ray, args->N, i)
            ) > 0;

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
            // This is how you reject a hit in embree3 instead of setting
            // geomId to invalid on the ray
            args->valid[i] = 0;
        }
    }
}

RTCGeometry
HdEmbreeMesh::_CreateEmbreeSubdivMesh(RTCScene scene, RTCDevice device)
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
    // Note this geometry is committed outside this function, but that
    // is not "enforced"
    RTCGeometry geom = rtcNewGeometry (device, RTC_GEOMETRY_TYPE_SUBDIVISION);

    // Uses a BVH refitting approach when changing only the vertex buffer.
    rtcSetGeometryBuildQuality(geom, RTC_BUILD_QUALITY_REFIT);
    rtcSetGeometryTimeStepCount(geom,1);
    _rtcMeshId = rtcAttachGeometry(scene,geom);

    // Fill the topology buffers.
    rtcSetSharedGeometryBuffer(geom,
        RTC_BUFFER_TYPE_FACE,
        0, /* unsigned int slot */
        RTC_FORMAT_UINT,
        _topology.GetFaceVertexCounts().cdata(),
        0, /* size_t byteOffset */
        sizeof(int),  /*must be 4 byte aligned */
        _topology.GetFaceVertexCounts().size());
    rtcSetSharedGeometryBuffer(geom,
        RTC_BUFFER_TYPE_INDEX,
        0, /* unsigned int slot */
        RTC_FORMAT_UINT,
        _topology.GetFaceVertexIndices().cdata(),
        0, /* size_t byteOffset */
        sizeof(int),  /*must be 4 byte aligned */
        _topology.GetFaceVertexIndices().size());

    if (_topology.GetHoleIndices().size()>0) {
        // PSA : creating a hole buffer with 0 length has very unexpected
        // behavior in Embree (things draw wrong, but not determinisitcally)
        rtcSetSharedGeometryBuffer(geom,
            RTC_BUFFER_TYPE_HOLE,
            0, /* unsigned int slot */
            RTC_FORMAT_UINT,
            _topology.GetHoleIndices().cdata(),
            0, /* size_t byteOffset */
            sizeof(unsigned int),  /*must be 4 byte aligned */
            _topology.GetHoleIndices().size());
    }

    // If this topology has edge creases, unroll the edge crease buffer.
    if (numEdgeCreases > 0) {
        int *embreeCreaseIndices = static_cast<int*>(
            rtcSetNewGeometryBuffer(
                geom,
                RTC_BUFFER_TYPE_EDGE_CREASE_INDEX,
                0, /* unsigned int slot */
                RTC_FORMAT_UINT2,
                2*sizeof(int), /*must be 4 byte aligned */
                numEdgeCreases));
        float *embreeCreaseWeights = static_cast<float*>(
            rtcSetNewGeometryBuffer(
                geom,
                RTC_BUFFER_TYPE_EDGE_CREASE_WEIGHT,
                0, /* unsigned int slot */
                RTC_FORMAT_FLOAT,
                sizeof(float), /*must be 4 byte aligned */
                numEdgeCreases));
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
    }

    if (numVertexCreases > 0) {
        rtcSetSharedGeometryBuffer(
            geom,
            RTC_BUFFER_TYPE_VERTEX_CREASE_INDEX,
            0, /* unsigned int slot */
            RTC_FORMAT_UINT,
            subdivTags.GetCornerIndices().cdata(),
            0, /* size_t byteOffset */
            sizeof(int), /*must be 4 byte aligned */
            numVertexCreases);
        rtcSetSharedGeometryBuffer(
            geom,
            RTC_BUFFER_TYPE_VERTEX_CREASE_WEIGHT,
            0, /* unsigned int slot */
            RTC_FORMAT_FLOAT,
            subdivTags.GetCornerWeights().cdata(),
            0, /* size_t byteOffset */
            sizeof(float), /*must be 4 byte aligned */
            numVertexCreases);
    }

    return geom;
}

RTCGeometry
HdEmbreeMesh::_CreateEmbreeTriangleMesh(RTCScene scene, RTCDevice device)
{
    // Triangulate the input faces.
    HdMeshUtil meshUtil(&_topology, GetId());
    meshUtil.ComputeTriangleIndices(&_triangulatedIndices,
        &_trianglePrimitiveParams);

    // Create the new mesh.
    // geometry will be committed in the calling function
    RTCGeometry geom = rtcNewGeometry (device, RTC_GEOMETRY_TYPE_TRIANGLE);
    // Uses a BVH refitting approach when changing only the vertex buffer.
    rtcSetGeometryBuildQuality(geom,RTC_BUILD_QUALITY_REFIT);
    rtcSetGeometryTimeStepCount(geom,1);
    _rtcMeshId = rtcAttachGeometry(scene,geom);

    if (_rtcMeshId == RTC_INVALID_GEOMETRY_ID) {
        TF_CODING_ERROR("Couldn't create RTC mesh");
    }

    // Populate topology.
    rtcSetSharedGeometryBuffer(geom,
        RTC_BUFFER_TYPE_INDEX,
        0, /* unsigned int slot */
        RTC_FORMAT_UINT3,
        _triangulatedIndices.cdata(),
        0, /* size_t byteOffset */
        sizeof(GfVec3i), /*must be 4 byte aligned */
        _triangulatedIndices.size());

    return geom;
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

    HdPrimvarDescriptorVector primvars;
    for (size_t i=0; i < HdInterpolationCount; ++i) {
        HdInterpolation interp = static_cast<HdInterpolation>(i);
        primvars = GetPrimvarDescriptors(sceneDelegate, interp);
        for (HdPrimvarDescriptor const& pv: primvars) {
            if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, pv.name) &&
                pv.name != HdTokens->points) {
                _primvarSourceMap[pv.name] = {
                    GetPrimvar(sceneDelegate, pv.name),
                    interp
                };
            }
        }
    }
}

TfTokenVector
HdEmbreeMesh::_UpdateComputedPrimvarSources(HdSceneDelegate* sceneDelegate,
                                            HdDirtyBits dirtyBits)
{
    HD_TRACE_FUNCTION();
    
    SdfPath const& id = GetId();

    // Get all the dirty computed primvars
    HdExtComputationPrimvarDescriptorVector dirtyCompPrimvars;
    for (size_t i=0; i < HdInterpolationCount; ++i) {
        HdExtComputationPrimvarDescriptorVector compPrimvars;
        HdInterpolation interp = static_cast<HdInterpolation>(i);
        compPrimvars = sceneDelegate->GetExtComputationPrimvarDescriptors
                                    (GetId(),interp);

        for (auto const& pv: compPrimvars) {
            if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, pv.name)) {
                dirtyCompPrimvars.emplace_back(pv);
            }
        }
    }

    if (dirtyCompPrimvars.empty()) {
        return TfTokenVector();
    }
    
    HdExtComputationUtils::ValueStore valueStore
        = HdExtComputationUtils::GetComputedPrimvarValues(
            dirtyCompPrimvars, sceneDelegate);

    TfTokenVector compPrimvarNames;
    // Update local primvar map and track the ones that were computed
    for (auto const& compPrimvar : dirtyCompPrimvars) {
        auto const it = valueStore.find(compPrimvar.name);
        if (!TF_VERIFY(it != valueStore.end())) {
            continue;
        }
        
        compPrimvarNames.emplace_back(compPrimvar.name);
        if (compPrimvar.name == HdTokens->points) {
            _points = it->second.Get<VtVec3fArray>();
            _normalsValid = false;
        } else {
            _primvarSourceMap[compPrimvar.name] = {it->second,
                                                compPrimvar.interpolation};
        }
    }

    return compPrimvarNames;
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
                    _rtcMeshScene, _rtcMeshId, &_embreeBufferAllocator );
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
    TfTokenVector computedPrimvars =
        _UpdateComputedPrimvarSources(sceneDelegate, *dirtyBits);

    bool pointsIsComputed =
        std::find(computedPrimvars.begin(), computedPrimvars.end(),
                  HdTokens->points) != computedPrimvars.end();
    if (!pointsIsComputed &&
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->points)) {
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
    if (HdChangeTracker::IsSubdivTagsDirty(*dirtyBits, id) &&
        _topology.GetRefineLevel() > 0) {
        _topology.SetSubdivTags(sceneDelegate->GetSubdivTags(id));
    }
    if (HdChangeTracker::IsDisplayStyleDirty(*dirtyBits, id)) {
        HdDisplayStyle const displayStyle = sceneDelegate->GetDisplayStyle(id);
        _topology = HdMeshTopology(_topology,
            displayStyle.refineLevel);
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
    _smoothNormals = !desc.flatShadingEnabled;

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
        if (_rtcMeshId != RTC_INVALID_GEOMETRY_ID) {
            // Delete the prototype context first...
            TF_FOR_ALL(it, _GetPrototypeContext()->primvarMap) {
                delete it->second;
            }
            delete _GetPrototypeContext();
            // then the prototype geometry.
            rtcDetachGeometry(_rtcMeshScene, _rtcMeshId);
            rtcReleaseGeometry(_geometry);
            _rtcMeshId = RTC_INVALID_GEOMETRY_ID;
        }

        // Create the prototype mesh scene, if it doesn't exist yet.
        if (_rtcMeshScene == nullptr) {
            _rtcMeshScene = rtcNewScene(device);
            // RTC_SCENE_FLAG_DYNAMIC: Provides better build performance for dynamic
            // scenes (but also higher memory consumption).
            rtcSetSceneFlags(_rtcMeshScene, RTC_SCENE_FLAG_DYNAMIC);

            // RTC_BUILD_QUALITY_LOW: Create lower quality data structures,
            // e.g. for dynamic scenes. A two-level spatial index structure is built
            // when enabling this mode, which supports fast partial scene updates,
            // and allows for setting a per-geometry build quality through
            // the rtcSetGeometryBuildQuality function.
            rtcSetSceneBuildQuality(_rtcMeshScene, RTC_BUILD_QUALITY_LOW);
        }

        // Populate either a subdiv or a triangle mesh object. The helper
        // functions will take care of populating topology buffers.
        if (doRefine) {
            _geometry = _CreateEmbreeSubdivMesh(_rtcMeshScene, device);
        } else {
            _geometry = _CreateEmbreeTriangleMesh(_rtcMeshScene, device);
        }
        if( _rtcMeshId == RTC_INVALID_GEOMETRY_ID ){
            TF_CODING_ERROR("Unable to create a mesh for the requested geometry");
            return;
        }

        _refined = doRefine;
        // In both cases, RTC_VERTEX_BUFFER will be populated below.

        // Prototype geometry gets tagged with a prototype context, that the
        // ray-hit algorithm can use to look up data.
        rtcSetGeometryUserData(_geometry,new HdEmbreePrototypeContext);
        _GetPrototypeContext()->rprim = this;
        _GetPrototypeContext()->primitiveParams = (_refined ?
            _trianglePrimitiveParams : VtIntArray());

        // Add _EmbreeCullFaces as a filter function for backface culling.
        rtcSetGeometryIntersectFilterFunction(_geometry,_EmbreeCullFaces);
        rtcSetGeometryOccludedFilterFunction(_geometry,_EmbreeCullFaces);

        // Force the smooth normals code to rebuild the "normals" primvar the
        // next time smooth normals is enabled.
        _normalsValid = false;
    }

    // If the refine level changed or the mesh was recreated, we need to pass
    // the refine level into the embree subdiv object.
    if (newMesh || HdChangeTracker::IsDisplayStyleDirty(*dirtyBits, id)) {
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
            rtcSetGeometryTessellationRate(_geometry,static_cast<float>(tessellationRate));
        }
    }

    // If the subdiv tags changed or the mesh was recreated, we need to update
    // the subdivision boundary mode.
    if (newMesh || HdChangeTracker::IsSubdivTagsDirty(*dirtyBits, id)) {
        if (doRefine) {
            TfToken const vertexRule =
                _topology.GetSubdivTags().GetVertexInterpolationRule();

            if (vertexRule == PxOsdOpenSubdivTokens->none) {
                rtcSetGeometrySubdivisionMode(_geometry,0,RTC_SUBDIVISION_MODE_NO_BOUNDARY);
            } else if (vertexRule == PxOsdOpenSubdivTokens->edgeOnly) {
                rtcSetGeometrySubdivisionMode(_geometry,0,RTC_SUBDIVISION_MODE_SMOOTH_BOUNDARY);
            } else if (vertexRule == PxOsdOpenSubdivTokens->edgeAndCorner) {
                rtcSetGeometrySubdivisionMode(_geometry,0,RTC_SUBDIVISION_MODE_PIN_CORNERS);
            } else {
                if (!vertexRule.IsEmpty()) {
                    TF_WARN("Unknown vertex interpolation rule: %s",
                            vertexRule.GetText());
                }
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
        _computedNormals = Hd_SmoothNormals::ComputeSmoothNormals(
            &_adjacency, _points.size(), _points.cdata());
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
        rtcSetSharedGeometryBuffer(
            _geometry,
            RTC_BUFFER_TYPE_VERTEX,
            0, /* unsigned int slot */
            RTC_FORMAT_FLOAT3,
            _points.cdata(),
            0, /* size_t byteOffset */
            sizeof(GfVec3f),
            _points.size());

        rtcCommitGeometry(_geometry);
    }

    // Update visibility by pulling the object into/out of the embree BVH.
    if (_sharedData.visible) {
        rtcEnableGeometry(_geometry);
    } else {
        rtcDisableGeometry(_geometry);
    }

    rtcCommitScene(_rtcMeshScene);

    ////////////////////////////////////////////////////////////////////////
    // 4. Populate embree instance objects.

    // First, update our own instancer data.
    _UpdateInstancer(sceneDelegate, dirtyBits);

    // Make sure we call sync on parent instancers.
    // XXX: In theory, this should be done automatically by the render index.
    // At the moment, it's done by rprim-reference.  The helper function on
    // HdInstancer needs to use a mutex to guard access, if there are actually
    // updates pending, so this might be a contention point.
    HdInstancer::_SyncInstancerAndParents(
        sceneDelegate->GetRenderIndex(), GetInstancerId());

    // If the instance topology changes, we need to update the instance
    // geometries. Un-instanced prims are treated here as a special case.
    // Instance geometries read from the instancer (for per-instance transform)
    // and the rprim transform, which gets added to the per instance transform.
    if (HdChangeTracker::IsInstancerDirty(*dirtyBits, id) ||
        HdChangeTracker::IsTransformDirty(*dirtyBits, id)) {

        VtMatrix4dArray transforms;
        if (!GetInstancerId().IsEmpty()) {
            // Retrieve instance transforms from the instancer.
            HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
            HdInstancer *instancer =
                renderIndex.GetInstancer(GetInstancerId());
            transforms = static_cast<HdEmbreeInstancer*>(instancer)->
                ComputeInstanceTransforms(GetId());
        } else {
            // If there's no instancer, add a single instance with transform I.
            transforms.push_back(GfMatrix4d(1.0));
        }

        size_t oldSize = _rtcInstanceIds.size();
        size_t newSize = transforms.size();

        // Size down (if necessary).
        for(size_t i = newSize; i < oldSize; ++i) {
            // Delete instance context first...
            delete _GetInstanceContext(scene, i);
            // Then Embree instance.
            rtcDetachGeometry(scene,_rtcInstanceIds[i]);
            rtcReleaseGeometry(_rtcInstanceGeometries[i]);
        }
        _rtcInstanceIds.resize(newSize);
        _rtcInstanceGeometries.resize(newSize);

        // Size up (if necessary).
        for(size_t i = oldSize; i < newSize; ++i) {
            // Create the new instance.
            RTCGeometry geom = rtcNewGeometry (device, RTC_GEOMETRY_TYPE_INSTANCE);
            rtcSetGeometryInstancedScene(geom,_rtcMeshScene);
            rtcSetGeometryTimeStepCount(geom,1);
            _rtcInstanceIds[i] = rtcAttachGeometry(scene,geom);

            // Create the instance context.
            HdEmbreeInstanceContext *ctx = new HdEmbreeInstanceContext;
            ctx->rootScene = _rtcMeshScene;
            ctx->instanceId = i;
            rtcSetGeometryUserData(geom,ctx);
            _rtcInstanceGeometries[i] = geom;
        }

        // Update transform
        for (size_t i = 0; i < transforms.size(); ++i) {
            // Combine the local transform and the instance transform.
            GfMatrix4f matf = _transform * GfMatrix4f(transforms[i]);

            // Update the transform in the BVH.
            rtcSetGeometryTransform(rtcGetGeometry(scene,_rtcInstanceIds[i]),0,RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR,matf.GetArray());
            // // Update the transform in the instance context.
            _GetInstanceContext(scene, i)->objectToWorldMatrix = matf;
            // // Mark the instance as updated in the BVH.
            rtcCommitGeometry(_rtcInstanceGeometries[i]);
        }
    }

    //
    // We are relying on the code calling this to commit the scene
    // since there are a bunch of commits to instances of geom
    // in the root scene
    //

    // Clean all dirty bits.
    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

HdEmbreePrototypeContext*
HdEmbreeMesh::_GetPrototypeContext()
{
    return static_cast<HdEmbreePrototypeContext*>(
        rtcGetGeometryUserData(_geometry));
}

HdEmbreeInstanceContext*
HdEmbreeMesh::_GetInstanceContext(RTCScene scene, size_t i)
{
    return static_cast<HdEmbreeInstanceContext*>(
        rtcGetGeometryUserData(_rtcInstanceGeometries[i]));
}

PXR_NAMESPACE_CLOSE_SCOPE
