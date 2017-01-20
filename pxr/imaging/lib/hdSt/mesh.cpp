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

#include "pxr/imaging/hdSt/mesh.h"
#include "pxr/imaging/hdSt/meshShaderKey.h"
#include "pxr/imaging/hdSt/meshTopology.h"
#include "pxr/imaging/hdSt/quadrangulate.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/tf/envSetting.h"

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/computation.h"
#include "pxr/imaging/hd/geometricShader.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/surfaceShader.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vertexAdjacency.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/vt/value.h"

TF_DEFINE_ENV_SETTING(HD_ENABLE_SMOOTH_NORMALS, "CPU",
                      "Select smooth normals computation device (CPU/GPU)");
TF_DEFINE_ENV_SETTING(HD_ENABLE_QUADRANGULATE, "0",
                      "Enable quadrangulation (0/CPU/GPU)");
TF_DEFINE_ENV_SETTING(HD_ENABLE_REFINE_GPU, 0, "GPU refinement");
TF_DEFINE_ENV_SETTING(HD_ENABLE_PACKED_NORMALS, 1,
                      "Use packed normals");

// static repr configuration
HdStMesh::_MeshReprConfig HdStMesh::_reprDescConfig;

HdStMesh::HdStMesh(HdSceneDelegate* delegate, SdfPath const& id,
               SdfPath const& instancerId)
    : HdMesh(delegate, id, instancerId)
    , _topology()
    , _topologyId(0)
    , _customDirtyBitsInUse(0)
    , _doubleSided(false)
    , _packedNormals(IsEnabledPackedNormals())
    , _cullStyle(HdCullStyleDontCare)
{
    /*NOTHING*/
}

HdStMesh::~HdStMesh()
{
    /*NOTHING*/
}

/* static */
bool
HdStMesh::IsEnabledSmoothNormalsGPU()
{
    static bool enabled = (TfGetEnvSetting(HD_ENABLE_SMOOTH_NORMALS) == "GPU");
    return enabled;
}

/* static */
bool
HdStMesh::IsEnabledQuadrangulationCPU()
{
    static bool enabled = (TfGetEnvSetting(HD_ENABLE_QUADRANGULATE) == "CPU");
    return enabled;
}

/* static */
bool
HdStMesh::IsEnabledQuadrangulationGPU()
{
    static bool enabled = (TfGetEnvSetting(HD_ENABLE_QUADRANGULATE) == "GPU");
    return enabled;
}

/* static */
bool
HdStMesh::IsEnabledRefineGPU()
{
    static bool enabled = (TfGetEnvSetting(HD_ENABLE_REFINE_GPU) == 1);
    return enabled;
}

/* static */
bool
HdStMesh::IsEnabledPackedNormals()
{
    static bool enabled = (TfGetEnvSetting(HD_ENABLE_PACKED_NORMALS) == 1);
    return enabled;
}

int
HdStMesh::_GetRefineLevelForDesc(HdStMeshReprDesc desc)
{
    if (desc.geomStyle == HdMeshGeomStyleHull         ||
        desc.geomStyle == HdMeshGeomStyleHullEdgeOnly || 
        desc.geomStyle == HdMeshGeomStyleHullEdgeOnSurf) {
        return 0;
    }
    if (!TF_VERIFY(_topology)) return 0;
    return _topology->GetRefineLevel();
}

void
HdStMesh::_PopulateTopology(HdDrawItem *drawItem,
                          HdChangeTracker::DirtyBits *dirtyBits,
                            HdStMeshReprDesc desc)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdResourceRegistry *resourceRegistry = &HdResourceRegistry::GetInstance();

    // note: there's a potential optimization if _topology is already registered
    // and it's not shared across prims, it can be updated without inserting new
    // entry into the topology registry. But in mose case topology varying prim
    // requires range resizing (reallocation), so for code simplicity we always
    // register as a new topology (it still can be shared if possible) and
    // allocate a new range for varying topology (= dirty topology)
    // for the time being. In other words, each range of index buffer is
    // immutable.

    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id)    || 
        HdChangeTracker::IsRefineLevelDirty(*dirtyBits, id) || 
        HdChangeTracker::IsSubdivTagsDirty(*dirtyBits, id)) {
        // make a shallow copy and the same time expand the topology to a
        // stream extended representation
        // note: if we add topologyId computation in delegate,
        // we can move this copy into topologyInstance.IsFirstInstance() block
        HdSt_MeshTopologySharedPtr topology =
                    HdSt_MeshTopology::New(GetMeshTopology(), GetRefineLevel());

        int refineLevel = topology->GetRefineLevel();
        if (refineLevel > 0) {
            // add subdiv tags before compute hash
            // XXX: calling GetSubdivTags on implicit prims raises an error.
            topology->SetSubdivTags(GetSubdivTags());
        }

        // Compute id here. In the future delegate can provide id directly without
        // hashing.
        _topologyId = topology->ComputeHash();

        // Salt the hash with refinement level and usePtexIndices.
        // (refinement level is moved into HdMeshTopology)
        //
        // Specifically for ptexIndices, we could do better here because all we
        // really need is the ability to compute quad indices late, however
        // splitting the topology shouldn't be a huge cost either.
        bool usePtexIndices = _UsePtexIndices();
        boost::hash_combine(_topologyId, usePtexIndices);

        {
            // XXX: Should be HdSt_MeshTopologySharedPtr
            HdInstance<HdTopology::ID, HdMeshTopologySharedPtr> topologyInstance;

            // ask registry if there's a sharable mesh topology
            std::unique_lock<std::mutex> regLock =
                resourceRegistry->RegisterMeshTopology(_topologyId, &topologyInstance);

            if (topologyInstance.IsFirstInstance()) {
                // if this is the first instance, set this topology to registry.
                topologyInstance.SetValue(
                        boost::static_pointer_cast<HdMeshTopology>(topology));

                // if refined, we submit a subdivision preprocessing
                // no matter what desc says
                // (see the lengthy comment in PopulateVertexPrimVar)
                if (refineLevel > 0) {
                    // OpenSubdiv preprocessing
                    HdBufferSourceSharedPtr
                        topologySource = topology->GetOsdTopologyComputation(id);
                    resourceRegistry->AddSource(topologySource);
                }

                // we also need quadinfo if requested.
                // Note that this is needed even if refineLevel > 0, in case
                // HdMeshGeomStyleHull is going to be used.
                if (usePtexIndices) {
                    // Quadrangulate preprocessing
                    HdSt_QuadInfoBuilderComputationSharedPtr quadInfoBuilder =
                        topology->GetQuadInfoBuilderComputation(
                            HdStMesh::IsEnabledQuadrangulationGPU(), id,
                            resourceRegistry);
                    resourceRegistry->AddSource(quadInfoBuilder);
                }
            }
            _topology = boost::static_pointer_cast<HdSt_MeshTopology>(
                                                   topologyInstance.GetValue());
        }
        TF_VERIFY(_topology);

        // hash collision check
        if (TfDebug::IsEnabled(HD_SAFE_MODE)) {
            TF_VERIFY(*topology == *_topology);
        }

        _vertexAdjacency.reset();
    }

    // here, we have _topology up-to-date.

    int refineLevelForDesc = _GetRefineLevelForDesc(desc);
    TfToken indexToken;  // bar-instance identifier

    // bail out if the index bar is already synced
    if (drawItem->GetDrawingCoord()->GetTopologyIndex() == HdStMesh::HullTopology) {
        if ((*dirtyBits & DirtyHullIndices) == 0) return;
        *dirtyBits &= ~DirtyHullIndices;
        indexToken = HdTokens->hullIndices;
    } else if (drawItem->GetDrawingCoord()->GetTopologyIndex() == HdStMesh::PointsTopology) {
        if ((*dirtyBits & DirtyPointsIndices) == 0) return;
        *dirtyBits &= ~DirtyPointsIndices;
        indexToken = HdTokens->pointsIndices;
    } else {
        if ((*dirtyBits & DirtyIndices) == 0) return;
        *dirtyBits &= ~DirtyIndices;
        indexToken = HdTokens->indices;
    }

    // note: don't early out even if the topology has no faces,
    // otherwise codegen takes inconsistent configuration and
    // fails to compile ( or even segfaults: filed as nvidia-bug 1719609 )

    {
        HdInstance<HdTopology::ID, HdBufferArrayRangeSharedPtr> rangeInstance;

        // ask again registry if there's a shareable buffer range for the topology
        std::unique_lock<std::mutex> regLock =
            resourceRegistry->RegisterMeshIndexRange(_topologyId, indexToken, &rangeInstance);

        if (rangeInstance.IsFirstInstance()) {
            // if not exists, update actual topology buffer to range.
            // Allocate new one if necessary.
            HdBufferSourceSharedPtr source;

            if (desc.geomStyle == HdMeshGeomStylePoints) {
                // create coarse points indices
                source = _topology->GetPointsIndexBuilderComputation();
            } else if (refineLevelForDesc > 0) {
                // create refined indices and primitiveParam
                source = _topology->GetOsdIndexBuilderComputation();
            } else if (_UsePtexIndices() /*not refined = quadrangulate*/) {
                // create quad indices and primitiveParam
                source = _topology->GetQuadIndexBuilderComputation(GetId());
            } else {
                // create triangle indices and primitiveParam
                source = _topology->GetTriangleIndexBuilderComputation(GetId());
            }
            HdBufferSourceVector sources;
            sources.push_back(source);

            // initialize buffer array
            //   * indices
            //   * primitiveParam
            HdBufferSpecVector bufferSpecs;
            HdBufferSpec::AddBufferSpecs(&bufferSpecs, sources);

            // allocate new range
            HdBufferArrayRangeSharedPtr range = 
                resourceRegistry->AllocateNonUniformBufferArrayRange(
                    HdTokens->topology, bufferSpecs);

            // add sources to update queue
            resourceRegistry->AddSources(range, sources);

            // save new range to registry
            rangeInstance.SetValue(range);

            if (drawItem->GetTopologyRange()) {
                // if this is a varying topology (we already have one and we're
                // going to replace it), set the garbage collection needed.
                _GetRenderIndex().GetChangeTracker().SetGarbageCollectionNeeded();
            }
        }

        // TODO: reuse same range for varying topology
        _sharedData.barContainer.Set(drawItem->GetDrawingCoord()->GetTopologyIndex(),
                                     rangeInstance.GetValue());
    }
}

void
HdStMesh::_PopulateAdjacency()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // The topology may be null in the event that it has zero faces.
    if (!_topology) return;

    HdResourceRegistry *resourceRegistry = &HdResourceRegistry::GetInstance();
    {
        HdInstance<HdTopology::ID, Hd_VertexAdjacencySharedPtr> adjacencyInstance;

        // ask registry if there's a sharable vertex adjacency
        std::unique_lock<std::mutex> regLock = 
            resourceRegistry->RegisterVertexAdjacency(_topologyId, &adjacencyInstance);

        if (adjacencyInstance.IsFirstInstance()) {
            Hd_VertexAdjacencySharedPtr adjacency(new Hd_VertexAdjacency());

            // create adjacency table for smooth normals
            HdBufferSourceSharedPtr adjacencyComputation =
                adjacency->GetAdjacencyBuilderComputation(_topology.get());

            resourceRegistry->AddSource(adjacencyComputation);

            if (IsEnabledSmoothNormalsGPU()) {
                // also send adjacency table to gpu
                HdBufferSourceSharedPtr adjacencyForGpuComputation =
                    adjacency->GetAdjacencyBuilderForGPUComputation();

                HdBufferSpecVector bufferSpecs;
                adjacencyForGpuComputation->AddBufferSpecs(&bufferSpecs);

                HdBufferArrayRangeSharedPtr adjRange =
                    resourceRegistry->AllocateNonUniformBufferArrayRange(
                        HdTokens->topology, bufferSpecs);

                adjacency->SetAdjacencyRange(adjRange);
                resourceRegistry->AddSource(adjRange,
                                            adjacencyForGpuComputation);
            }

            adjacencyInstance.SetValue(adjacency);
        }
        _vertexAdjacency = adjacencyInstance.GetValue();
    }
}

static HdBufferSourceSharedPtr
_QuadrangulatePrimVar(HdBufferSourceSharedPtr const &source,
                      HdComputationVector *computations,
                      HdSt_MeshTopologySharedPtr const &topology,
                      SdfPath const &id)
{
    if (!TF_VERIFY(computations)) return source;

    if (!HdStMesh::IsEnabledQuadrangulationGPU()) {
        // CPU quadrangulation
        HdResourceRegistry *resourceRegistry =
            &HdResourceRegistry::GetInstance();

        // set quadrangulation as source instead of original source.
        HdBufferSourceSharedPtr quadsource =
            topology->GetQuadrangulateComputation(source, id);

        if (quadsource) {
            // don't transfer source to gpu, it needs to be quadrangulated.
            // but it still has to be resolved. add to registry.
            resourceRegistry->AddSource(source);
            return quadsource;
        } else {
            return source;
        }
    } else {
        // GPU quadrangulation computation needs original vertices to be
        // transfered
        HdComputationSharedPtr computation = 
            topology->GetQuadrangulateComputationGPU(
                source->GetName(), source->GetGLComponentDataType(), id);
        // computation can be null for all quad mesh.
        if (computation)
            computations->push_back(computation);
        return source;
    }
}

static HdBufferSourceSharedPtr
_QuadrangulateFaceVaryingPrimVar(HdBufferSourceSharedPtr const &source,
                                 HdSt_MeshTopologySharedPtr const &topology,
                                 SdfPath const &id)
{
    // note: currently we don't support GPU facevarying quadrangulation.
    HdResourceRegistry *resourceRegistry = &HdResourceRegistry::GetInstance();

    // set quadrangulation as source instead of original source.
    HdBufferSourceSharedPtr quadSource =
        topology->GetQuadrangulateFaceVaryingComputation(source, id);

    // don't transfer source to gpu, it needs to be quadrangulated.
    // but it still has to be resolved, so add it to registry.
    resourceRegistry->AddSource(source);

    return quadSource;
}

static HdBufferSourceSharedPtr
_TriangulateFaceVaryingPrimVar(HdBufferSourceSharedPtr const &source,
                               HdSt_MeshTopologySharedPtr const &topology,
                               SdfPath const &id)
{
    HdResourceRegistry *resourceRegistry = &HdResourceRegistry::GetInstance();

    HdBufferSourceSharedPtr triSource =
        topology->GetTriangulateFaceVaryingComputation(source, id);

    // don't transfer source to gpu, it needs to be triangulated.
    // but it still has to be resolved, so add it to registry.
    resourceRegistry->AddSource(source);

    return triSource;
}

static HdBufferSourceSharedPtr
_RefinePrimVar(HdBufferSourceSharedPtr const &source,
               bool varying,
               HdComputationVector *computations,
               HdSt_MeshTopologySharedPtr const &topology)
{
    if (!TF_VERIFY(computations)) return source;

    if (!HdStMesh::IsEnabledRefineGPU()) {
        // CPU subdivision
        // note: if the topology is empty, the source will be returned
        //       without change. We still need the type of buffer
        //       to get the codegen work even for empty meshes
        return topology->GetOsdRefineComputation(source, varying);
    } else {
        // GPU subdivision
        HdComputationSharedPtr computation =
            topology->GetOsdRefineComputationGPU(
                source->GetName(), source->GetGLComponentDataType(),
                source->GetNumComponents());
        // computation can be null for empty mesh
        if (computation)
            computations->push_back(computation);
    }

    return source;
}

void
HdStMesh::_PopulateVertexPrimVars(HdDrawItem *drawItem,
                                HdChangeTracker::DirtyBits *dirtyBits,
                                bool isNew,
                                  HdStMeshReprDesc desc,
                                bool requireSmoothNormals)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdResourceRegistry *resourceRegistry = &HdResourceRegistry::GetInstance();

    // The "points" attribute is expected to be in this list.
    TfTokenVector primVarNames = GetPrimVarVertexNames();
    
    // Track the last vertex index to distinguish between vertex and varying
    // while processing.
    int vertexPartitionIndex = int(primVarNames.size()-1);
    
    // Add varying primvars.
    TfTokenVector const& varyingNames = GetPrimVarVaryingNames();
    primVarNames.reserve(primVarNames.size() + varyingNames.size());
    primVarNames.insert(primVarNames.end(), 
                        varyingNames.begin(), varyingNames.end());

    HdBufferSourceVector sources;
    sources.reserve(primVarNames.size());
    HdComputationVector computations;
    HdBufferSourceSharedPtr points;

    int numPoints = _topology ? _topology->ComputeNumPoints() : 0;

    bool cpuSmoothNormals =
        requireSmoothNormals && (!IsEnabledSmoothNormalsGPU());

    int refineLevel = _topology ? _topology->GetRefineLevel() : 0;
    // Don't call _GetRefineLevelForDesc(desc) instead of GetRefineLevel(). Why?
    //
    // We share the vertex BAR from both refined and hull topologies so that
    // the change tracker doesn't have to keep track the refined primvars.
    //
    // The hull topology refers coarse vertices that are placed on the beginning
    // of the vertex bar (this is a nature of OpenSubdiv adaptive/uniform
    // refinement). The refined topology refers entire vertex bar.
    //
    // If we only update the coarse vertices for the hull repr, and if we also
    // have a refined repr which stucks in an old state, DirtyPoints gets
    // cleared/ just updating coarse vertices and we lost a chance of updating
    // refined primvars. This state discrepancy could happen over frame, so
    // somebody has to maintain the versioning of each buffers.
    //
    // For topology, _indicesValid and _hullIndicesValid are used for that
    // purpose and it's possible because mesh topology is cached and shared in
    // the instance registry. We don't need to ask sceneDelegate, thus
    // individual (hull and refined) change trackings aren't needed.
    //
    // For vertex primvars, here we simply force to update all vertices at the
    // prim's authored refine level. Then both hull and refined topology can
    // safely access all valid data without having separate change tracking.
    //
    // This could be a performance concern, where a prim has higher refine level
    // and a hydra client keeps drawing only hull repr for some reason.
    // Currently we assume it's not likely a use-case, but we may revisit later
    // and optimize if necessary.
    //

    // Track index to identify varying primvars.
    int i = 0;
    TF_FOR_ALL(nameIt, primVarNames) {
        // If the index is greater than the last vertex index, isVarying=true.
        bool isVarying = i++ > vertexPartitionIndex;

        if (!HdChangeTracker::IsPrimVarDirty(*dirtyBits, id, *nameIt)) {
            // one exception: if smoothNormals=true and DirtyNormals set,
            // we need points even if they are clean.
            if (cpuSmoothNormals == false   ||
                *nameIt != HdTokens->points ||
                !(HdChangeTracker::IsPrimVarDirty(
                         *dirtyBits, id, HdTokens->normals))) {
                continue;
            }
        }

        // TODO: We don't need to pull primvar metadata every time a
        // value changes, but we need support from the delegate.

        VtValue value =  GetPrimVar(*nameIt);

        if (!value.IsEmpty()) {
            HdBufferSourceSharedPtr source(
                new HdVtBufferSource(*nameIt, value));

            // verify primvar length
            if (source->GetNumElements() != numPoints) {
                TF_WARN(
                    "# of points mismatch (%d != %d) for primvar %s, prim %s",
                    source->GetNumElements(), numPoints,
                    nameIt->GetText(), id.GetText());
                continue;
            }

            if (refineLevel > 0) {
                source = _RefinePrimVar(source, isVarying,
                                        &computations, _topology);
            } else if (_UsePtexIndices()) {
                source = _QuadrangulatePrimVar(source, &computations, _topology,
                                               GetId());
            }
            sources.push_back(source);

            // save the point buffer source for smooth normal computation.
            if (requireSmoothNormals
                && *nameIt == HdTokens->points) {
                points = source;
            }
        }
    }

    if (requireSmoothNormals &&
        HdChangeTracker::IsPrimVarDirty(*dirtyBits, id, HdTokens->normals)) {
        // note: normals gets dirty when points are marked as dirty,
        // at changetracker.

        // clear DirtySmoothNormals (this is not a scene dirtybit)
        *dirtyBits &= ~DirtySmoothNormals;

        TF_VERIFY(_vertexAdjacency);

        if (cpuSmoothNormals) {
            if (points) {
                // CPU smooth normals depends on CPU adjacency.
                //
                HdBufferSourceSharedPtr normal;
                bool doRefine = (refineLevel > 0);
                bool doQuadrangulate = _UsePtexIndices();

                if (doRefine || doQuadrangulate) {
                    if (_packedNormals) {
                        // we can't use packed normals for refined/quad,
                        // let's migrate the buffer to full precision
                        isNew = true;
                        _packedNormals = false;
                    }
                    normal = _vertexAdjacency->GetSmoothNormalsComputation(
                            points, HdTokens->normals);
                    if (doRefine) {
                        normal = _RefinePrimVar(normal, /*varying=*/false,
                                                &computations, _topology);
                    } else if (doQuadrangulate) {
                        normal = _QuadrangulatePrimVar(
                            normal, &computations, _topology, GetId());
                    }
                } else {
                    // if we haven't refined or quadrangulated normals,
                    // may use packed format if enabled.
                    if (_packedNormals) {
                        normal = _vertexAdjacency->GetSmoothNormalsComputation(
                            points, HdTokens->packedNormals, true);
                    } else {
                        normal = _vertexAdjacency->GetSmoothNormalsComputation(
                            points, HdTokens->normals, false);
                    }
                }
                sources.push_back(normal);
            }
        } else {
            // GPU smooth normals doesn't need to have an expliciy dependency.
            // The adjacency table should be commited before execution.

            // determine datatype. if we're updating points too, ask the buffer
            // source. Otherwise (if we're updating just normals) ask delegate.
            // This is very unfortunate. Can we force normals to be always
            // float? (e.g. when switing flat -> smooth first time).
            //
            // or, we should use HdSceneDelegate::GetPrimVarDataType() and
            // HdSceneDelegate::GetPrimVarComponents() once they are implemented
            // in UsdImagindDelegate.



            if (!points) {
                VtValue value = GetPoints();

                points = HdBufferSourceSharedPtr(
                          new HdVtBufferSource(HdTokens->points,
                                               value));
            }

            if (points) {
                GLenum normalDataType = points->GetGLComponentDataType();

                computations.push_back(
                    _vertexAdjacency->GetSmoothNormalsComputationGPU(
                        HdTokens->points,
                        HdTokens->normals,
                        normalDataType));

                // note: we haven't had explicit dependency for GPU
                // computations just yet. Currently they are executed
                // sequentially, so the dependency is expressed by
                // registering order.
                if (refineLevel > 0) {
                    HdComputationSharedPtr computation =
                        _topology->GetOsdRefineComputationGPU(
                            HdTokens->normals,
                            normalDataType, 3);
                        // computation can be null for empty mesh
                        if (computation)
                            computations.push_back(computation);
                } else if (_UsePtexIndices()) {
                    HdComputationSharedPtr computation =
                        _topology->GetQuadrangulateComputationGPU(
                            HdTokens->normals, normalDataType, GetId());
                    // computation can be null for all-quad mesh
                    if (computation)
                        computations.push_back(computation);
                }
            }
        }
    }

    // return before allocation if it's empty.
    if (sources.empty() && computations.empty()) {
        return;
    }

    HdBufferArrayRangeSharedPtr const &bar = drawItem->GetVertexPrimVarRange();
    if ((!bar) || (!bar->IsValid())) {
        // new buffer specs
        HdBufferSpecVector bufferSpecs;
        HdBufferSpec::AddBufferSpecs(&bufferSpecs, sources);
        HdBufferSpec::AddBufferSpecs(&bufferSpecs, computations);

        // allocate new range.
        HdBufferArrayRangeSharedPtr range =
            resourceRegistry->AllocateNonUniformBufferArrayRange(
                HdTokens->primVar, bufferSpecs);

        _sharedData.barContainer.Set(
            drawItem->GetDrawingCoord()->GetVertexPrimVarIndex(), range);
    } else {
        // already have a valid range.
        if (isNew) {
            // the range was created by other repr. check compatibility.
            HdBufferSpecVector bufferSpecs;
            HdBufferSpec::AddBufferSpecs(&bufferSpecs, sources);
            HdBufferSpec::AddBufferSpecs(&bufferSpecs, computations);

            HdBufferArrayRangeSharedPtr range =
                resourceRegistry->MergeNonUniformBufferArrayRange(
                    HdTokens->primVar, bufferSpecs,
                    drawItem->GetVertexPrimVarRange());

            _sharedData.barContainer.Set(
                drawItem->GetDrawingCoord()->GetVertexPrimVarIndex(), range);

            // If buffer migration actually happens, the old buffer will no
            // longer be needed, and GC is required to reclaim their memory.
            // But we don't trigger GC here for now, since it ends up
            // to make all collections dirty (see HdEngine::Draw), which can be
            // expensive.
            // (in other words, we should fix bug 103767:
            //  "Optimize varying topology buffer updates" first)
            //
            // if (range != bar) {
            //    _GetRenderIndex().GetChangeTracker().SetGarbageCollectionNeeded();
            // }
        }
    }

    // schedule buffer sources
    if (!sources.empty()) {
        // add sources to update queue
        resourceRegistry->AddSources(drawItem->GetVertexPrimVarRange(),
                                     sources);
    }
    if (!computations.empty()) {
        // add gpu computations to queue.
        TF_FOR_ALL(it, computations) {
            resourceRegistry->AddComputation(
                drawItem->GetVertexPrimVarRange(), *it);
        }
    }
}

void
HdStMesh::_PopulateFaceVaryingPrimVars(HdDrawItem *drawItem,
                                     HdChangeTracker::DirtyBits *dirtyBits,
                                       HdStMeshReprDesc desc)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    TfTokenVector primVarNames = GetPrimVarFacevaryingNames();
    if (primVarNames.empty()) return;

    HdResourceRegistry *resourceRegistry = &HdResourceRegistry::GetInstance();

    HdBufferSourceVector sources;
    sources.reserve(primVarNames.size());

    int refineLevel = _GetRefineLevelForDesc(desc);
    int numFaceVaryings = _topology ? _topology->GetNumFaceVaryings() : 0;

    TF_FOR_ALL(nameIt, primVarNames) {
        // note: facevarying primvars don't have to be refined.
        if (!HdChangeTracker::IsPrimVarDirty(*dirtyBits, id,*nameIt)) {
            continue;
        }

        VtValue value = GetPrimVar(*nameIt);
        if (!value.IsEmpty()) {

            HdBufferSourceSharedPtr source(new HdVtBufferSource(
                                               *nameIt,
                                               value));

            // verify primvar length
            if (source->GetNumElements() != numFaceVaryings) {
                TF_WARN(
                    "# of facevaryings mismatch (%d != %d)"
                    " for primvar %s, prim %s",
                    source->GetNumElements(), numFaceVaryings,
                    nameIt->GetText(), id.GetText());
                continue;
            }

            // FaceVarying primvar requires quadrangulation (both coarse and
            // refined) or triangulation (coase only), but refinement of the
            // primvar is not needed even if the repr is refined, since we only
            // support linear interpolation until OpenSubdiv 3.1 supports it.

            //
            // XXX: there is a bug of quad and tris confusion. see bug 121414
            //
            if (_UsePtexIndices() || refineLevel > 0) {
                source = _QuadrangulateFaceVaryingPrimVar(source, _topology,
                                                          GetId());
            } else {
                source = _TriangulateFaceVaryingPrimVar(source, _topology,
                                                        GetId());
            }
            sources.push_back(source);
        }
    }

    // return before allocation if it's empty.
    if (sources.empty()) return;

    // face varying primvars exist.
    // allocate new bar if not exists
    if (!drawItem->GetFaceVaryingPrimVarRange()) {
        HdBufferSpecVector bufferSpecs;
        HdBufferSpec::AddBufferSpecs(&bufferSpecs, sources);

        HdBufferArrayRangeSharedPtr range =
            resourceRegistry->AllocateNonUniformBufferArrayRange(
                HdTokens->primVar, bufferSpecs);
        _sharedData.barContainer.Set(
            drawItem->GetDrawingCoord()->GetFaceVaryingPrimVarIndex(), range);
    }

    TF_VERIFY(drawItem->GetFaceVaryingPrimVarRange()->IsValid());

    resourceRegistry->AddSources(
        drawItem->GetFaceVaryingPrimVarRange(), sources);
}

void
HdStMesh::_PopulateElementPrimVars(HdDrawItem *drawItem,
                                 HdChangeTracker::DirtyBits *dirtyBits,
                                 TfTokenVector const &primVarNames)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdResourceRegistry *resourceRegistry = &HdResourceRegistry::GetInstance();

    HdBufferSourceVector sources;
    sources.reserve(primVarNames.size());

    int numFaces = _topology ? _topology->GetNumFaces() : 0;

    TF_FOR_ALL(nameIt, primVarNames) {
        if (!HdChangeTracker::IsPrimVarDirty(*dirtyBits, id, *nameIt))
            continue;

        VtValue value = GetPrimVar(*nameIt);
        if (!value.IsEmpty()) {
            HdBufferSourceSharedPtr source(new HdVtBufferSource(
                                               *nameIt,
                                               value));

            // verify primvar length
            if (source->GetNumElements() != numFaces) {
                TF_WARN(
                    "# of faces mismatch (%d != %d) for primvar %s, prim %s",
                    source->GetNumElements(), numFaces,
                    nameIt->GetText(), id.GetText());
                continue;
            }

            sources.push_back(source);
        }
    }

    // return before allocation if it's empty.
    if (sources.empty()) return;

    // element primvars exist.
    // allocate new bar if not exists
    if (!drawItem->GetElementPrimVarRange()) {
        HdBufferSpecVector bufferSpecs;
        HdBufferSpec::AddBufferSpecs(&bufferSpecs, sources);

        HdBufferArrayRangeSharedPtr range =
            resourceRegistry->AllocateNonUniformBufferArrayRange(
                HdTokens->primVar, bufferSpecs);
        _sharedData.barContainer.Set(
            drawItem->GetDrawingCoord()->GetElementPrimVarIndex(), range);
    }

    TF_VERIFY(drawItem->GetElementPrimVarRange()->IsValid());

    resourceRegistry->AddSources(
        drawItem->GetElementPrimVarRange(), sources);
}

bool
HdStMesh::_UsePtexIndices() const
{
    HdSurfaceShaderSharedPtr const& ss = 
                            _GetRenderIndex().GetShader(GetSurfaceShaderId());
    TF_FOR_ALL(it, ss->GetParams()) {
        if (it->IsPtex())
            return true;
    }

    // Fallback to the environment variable, which allows forcing of
    // quadrangulation for debugging/testing.
    return IsEnabledQuadrangulation();
}

void
HdStMesh::_UpdateDrawItem(HdDrawItem *drawItem,
                        HdChangeTracker::DirtyBits *dirtyBits,
                        bool isNew,
                          HdStMeshReprDesc desc,
                        bool requireSmoothNormals)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();

    /* VISIBILITY */
    _UpdateVisibility(dirtyBits);

    /* CONSTANT PRIMVARS */
    _PopulateConstantPrimVars(drawItem, dirtyBits);

    /* INSTANCE PRIMVARS */
    _PopulateInstancePrimVars(drawItem, dirtyBits, InstancePrimVar);

    /* TOPOLOGY */
    // XXX: _PopulateTopology should be split into two phase
    //      for scene dirtybits and for repr dirtybits.
    if (*dirtyBits & (HdChangeTracker::DirtyTopology
                    | HdChangeTracker::DirtyRefineLevel
                    | HdChangeTracker::DirtySubdivTags
                                     | DirtyIndices
                                     | DirtyHullIndices
                                     | DirtyPointsIndices)) {
        _PopulateTopology(drawItem, dirtyBits, desc);
    }

    if (*dirtyBits & HdChangeTracker::DirtyDoubleSided) {
        _doubleSided = IsDoubleSided();
    }
    if (*dirtyBits & HdChangeTracker::DirtyCullStyle) {
        _cullStyle = GetCullStyle();
    }

    // disable smoothNormals for bilinear scheme mesh.
    // normal dirtiness will be cleared without computing/populating normals.
    TfToken scheme = _topology->GetScheme();
    if (scheme == PxOsdOpenSubdivTokens->bilinear) {
        requireSmoothNormals = false;
    }

    if (requireSmoothNormals && !_vertexAdjacency) {
        _PopulateAdjacency();
    }

    /* FACEVARYING PRIMVARS */
    if (HdChangeTracker::IsAnyPrimVarDirty(*dirtyBits, id)) {
        _PopulateFaceVaryingPrimVars(drawItem, dirtyBits, desc);
    }

    /* VERTEX PRIMVARS */
    if (isNew || HdChangeTracker::IsAnyPrimVarDirty(*dirtyBits, id)) {
        _PopulateVertexPrimVars(drawItem, dirtyBits, isNew, desc, requireSmoothNormals);
    }

    /* ELEMENT PRIMVARS */
    if (HdChangeTracker::IsAnyPrimVarDirty(*dirtyBits, id)) {
        TfTokenVector uniformPrimVarNames = GetPrimVarUniformNames();
        if (!uniformPrimVarNames.empty()) {
            _PopulateElementPrimVars(drawItem, dirtyBits, uniformPrimVarNames);
        }
    }

    // When we have multiple drawitems for the same mesh we need to clean the
    // bits for all the data fields touched in this function, otherwise it
    // will try to extract topology (for instance) twice, and this won't 
    // work with delegates that don't keep information around once extracted.
    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;

    TF_VERIFY(drawItem->GetConstantPrimVarRange());
    // Topology and VertexPrimVar may be null, if the mesh has zero faces.
    // Element primvar, Facevarying primvar and Instance primvar are optional
}

/* static */
void
HdStMesh::ConfigureRepr(TfToken const &reprName,
                        HdStMeshReprDesc desc1,
                        HdStMeshReprDesc desc2)
{
    HD_TRACE_FUNCTION();

    _reprDescConfig.Append(reprName, _MeshReprConfig::DescArray{desc1, desc2});
}

void
HdStMesh::_UpdateDrawItemGeometricShader(HdDrawItem *drawItem,
                                         HdStMeshReprDesc desc)
{
    if (drawItem->GetGeometricShader()) return;

    bool hasFaceVaryingPrimVars =
        (bool)drawItem->GetFaceVaryingPrimVarRange();

    int refineLevel = _GetRefineLevelForDesc(desc);

    // geometry type
    GLenum primType = GL_TRIANGLES;
    if (desc.geomStyle == HdMeshGeomStylePoints) {
        primType = GL_POINTS;
    } else if (refineLevel > 0) {
        if (_topology->RefinesToTriangles()) {
            // e.g. loop subdivision.
            primType = GL_TRIANGLES;
        } else if (_topology->RefinesToBSplinePatches()) {
            primType = GL_PATCHES;
        } else {
            // uniform catmark/bilinear subdivision generates quads.
            primType = GL_LINES_ADJACENCY;
        }
    } else if (_UsePtexIndices()) {
        // quadrangulate coarse mesh (for ptex)
        primType = GL_LINES_ADJACENCY;
    }

    // resolve geom style, cull style
    HdCullStyle cullStyle = _cullStyle;
    HdMeshGeomStyle geomStyle = desc.geomStyle;

    // We need to use smoothNormals flag per repr (and not requireSmoothNormals)
    // here since the geometric shader needs to know if we are actually
    // using normals or not.
    bool smoothNormals = desc.smoothNormals &&
        _topology->GetScheme() != PxOsdOpenSubdivTokens->bilinear;

    // if the prim doesn't have an opinion about cullstyle,
    // use repr's default (it could also be DontCare, then renderPass's
    // cullStyle is going to be used).
    //
    // i.e.
    //   Rprim CullStyle > Repr CullStyle > RenderPass CullStyle
    //
    if (cullStyle == HdCullStyleDontCare) {
        cullStyle = desc.cullStyle;
    }

    bool blendWireframeColor = desc.blendWireframeColor;

    // create a shaderKey and set to the geometric shader.
    HdSt_MeshShaderKey shaderKey(primType,
                                 desc.lit,
                                 smoothNormals,
                                 _doubleSided,
                                 hasFaceVaryingPrimVars,
                                 blendWireframeColor,
                                 cullStyle,
                                 geomStyle);

    drawItem->SetGeometricShader(Hd_GeometricShader::Create(shaderKey));

    // The batches need to be validated and rebuilt if necessary.
    _GetChangeTracker().MarkShaderBindingsDirty();
}

HdChangeTracker::DirtyBits
HdStMesh::_PropagateDirtyBits(HdChangeTracker::DirtyBits dirtyBits)
{
    // propagate scene-based dirtyBits into rprim-custom dirtyBits
    if (dirtyBits & HdChangeTracker::DirtyPoints) {
        dirtyBits |= _customDirtyBitsInUse & DirtySmoothNormals;
    }

    if (dirtyBits & HdChangeTracker::DirtyTopology) {
        dirtyBits |= _customDirtyBitsInUse & (DirtyIndices|DirtyHullIndices|DirtyPointsIndices);
    }

    // XXX: we should probably consider moving DirtyNormals out of ChangeTracker.
    //
    // pretend DirtySmoothNormals is DirtyNormal (to make IsPrimVarDirty work)
    if (dirtyBits & DirtySmoothNormals) {
        dirtyBits |= HdChangeTracker::DirtyNormals;
    }

    return dirtyBits;
}

HdReprSharedPtr const &
HdStMesh::_GetRepr(TfToken const &reprName,
                 HdChangeTracker::DirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _MeshReprConfig::DescArray descs = _reprDescConfig.Find(reprName);

    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprName));
    bool isNew = it == _reprs.end();
    if (isNew) {
        // add new repr
        it = _reprs.insert(_reprs.end(),
                           std::make_pair(reprName, HdReprSharedPtr(new HdRepr())));

        // allocate all draw items
        // XXX: TF_FOR_ALL doesn't work with std::array?
        for (auto desc : descs) {
            if (desc.geomStyle == HdMeshGeomStyleInvalid) continue;

            // redirect hull topology to extra slot
            HdDrawItem *drawItem = it->second->AddDrawItem(&_sharedData);
            HdDrawingCoord *drawingCoord = drawItem->GetDrawingCoord();

            if (desc.geomStyle == HdMeshGeomStyleHull         || 
                desc.geomStyle == HdMeshGeomStyleHullEdgeOnly || 
                desc.geomStyle == HdMeshGeomStyleHullEdgeOnSurf) {

                drawingCoord->SetTopologyIndex(HdStMesh::HullTopology);
                if (!(_customDirtyBitsInUse & DirtyHullIndices)) {
                    _customDirtyBitsInUse |= DirtyHullIndices;
                    *dirtyBits |= DirtyHullIndices;
                }

            } else if (desc.geomStyle == HdMeshGeomStylePoints) {
                // in the current implementation, we use topology(DrawElements)
                // for points too, to draw a subset of vertex primvars
                // (not that the points may be followed by the refined vertices)
                drawingCoord->SetTopologyIndex(HdStMesh::PointsTopology);
                if (!(_customDirtyBitsInUse & DirtyPointsIndices)) {
                    _customDirtyBitsInUse |= DirtyPointsIndices;
                    *dirtyBits |= DirtyPointsIndices;
                }
            } else {
                if (!(_customDirtyBitsInUse & DirtyIndices)) {
                    _customDirtyBitsInUse |= DirtyIndices;
                    *dirtyBits |= DirtyIndices;
                }
            }
            if (desc.smoothNormals) {
                if (!(_customDirtyBitsInUse & DirtySmoothNormals)) {
                    _customDirtyBitsInUse |= DirtySmoothNormals;
                    *dirtyBits |= DirtySmoothNormals;
                }
            }
        }
    }

    *dirtyBits = _PropagateDirtyBits(*dirtyBits);

    if (TfDebug::IsEnabled(HD_RPRIM_UPDATED)) {
        std::cout << "HdStMesh::GetRepr " << GetId() << " Repr = " << reprName << "\n";
        HdChangeTracker::DumpDirtyBits(*dirtyBits);
    }

    bool needsSetGeometricShader = false;
    // For the bits geometric shader depends on, reset all geometric shaders.
    // they are populated again at the end of _GetRepr.
    // Since the dirty bits are cleaned by UpdateDrawItem (because certain
    // reprs have multiple draw items) we need to remember if we need to set 
    // the geometric shader again
    if (*dirtyBits & (HdChangeTracker::DirtyRefineLevel|
                      HdChangeTracker::DirtyCullStyle|
                      HdChangeTracker::DirtyDoubleSided)) {
        _ResetGeometricShaders();
        needsSetGeometricShader = true;
    }

    // iterate through all reprs to figure out if any requires smoothnormals
    // if so we will calculate the normals once (clean the bits) and reuse them.
    // This is important for modes like FeyRay which requires 2 draw items 
    // and one requires smooth normals but the other doesn't.
    bool requireSmoothNormals = false;
    for (auto desc : descs) {
        if (desc.smoothNormals) {
            requireSmoothNormals = true;
            break;
        }
    }

    // iterate and update all draw items
    int drawItemIndex = 0;
    for (auto desc : descs) {
        if (desc.geomStyle == HdMeshGeomStyleInvalid) continue;

        if (isNew || HdChangeTracker::IsDirty(*dirtyBits)) {
            HdDrawItem *drawItem = it->second->GetDrawItem(drawItemIndex);
            _UpdateDrawItem(drawItem, dirtyBits, isNew, desc, requireSmoothNormals);
            _UpdateDrawItemGeometricShader(drawItem, desc);
        }
        ++drawItemIndex;
    }

    // if we need to rebuild geometric shader, make sure all reprs to have
    // their geometric shader up-to-date.
    if (needsSetGeometricShader) {
        _SetGeometricShaders();
    }

    return it->second;
}

void
HdStMesh::_ResetGeometricShaders()
{
    TF_FOR_ALL (it, _reprs) {
        TF_FOR_ALL (drawItem, *(it->second->GetDrawItems())) {
            drawItem->SetGeometricShader(Hd_GeometricShaderSharedPtr());
        }
    }
}

void
HdStMesh::_SetGeometricShaders()
{
    TF_FOR_ALL (it, _reprs) {
        _MeshReprConfig::DescArray descs = _reprDescConfig.Find(it->first);
        int drawItemIndex = 0;
        for (auto desc : descs) {
            if (desc.geomStyle == HdMeshGeomStyleInvalid) continue;

            HdDrawItem *drawItem = it->second->GetDrawItem(drawItemIndex);
            _UpdateDrawItemGeometricShader(drawItem, desc);
            ++drawItemIndex;
        }
    }
}

HdChangeTracker::DirtyBits
HdStMesh::_GetInitialDirtyBits() const
{
    int mask = HdChangeTracker::Clean
        | HdChangeTracker::DirtyCullStyle
        | HdChangeTracker::DirtyDoubleSided
        | HdChangeTracker::DirtyExtent
        | HdChangeTracker::DirtyInstanceIndex
        | HdChangeTracker::DirtyNormals
        | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyPrimID
        | HdChangeTracker::DirtyPrimVar
        | HdChangeTracker::DirtyRefineLevel
        | HdChangeTracker::DirtyRepr
        | HdChangeTracker::DirtySurfaceShader
        | HdChangeTracker::DirtyTopology
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        ;

    return (HdChangeTracker::DirtyBits)mask;
}
