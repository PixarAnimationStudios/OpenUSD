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

#include "pxr/imaging/hdSt/mesh.h"
#include "pxr/imaging/hdSt/meshShaderKey.h"
#include "pxr/imaging/hdSt/meshTopology.h"
#include "pxr/imaging/hdSt/mixinShaderCode.h"
#include "pxr/imaging/hdSt/quadrangulate.h"
#include "pxr/imaging/hdSt/shader.h"
#include "pxr/imaging/hdSt/surfaceShader.h"
#include "pxr/imaging/hdSt/instancer.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/tf/envSetting.h"

#include "pxr/imaging/hd/bufferArrayRangeGL.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/bufferResourceGL.h"
#include "pxr/imaging/hd/computation.h"
#include "pxr/imaging/hd/geometricShader.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vertexAdjacency.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE


// for debugging
TF_DEFINE_ENV_SETTING(HD_ENABLE_FORCE_QUADRANGULATE, 0,
                      "Apply quadrangulation for all meshes for debug");

// default to use packed normals
TF_DEFINE_ENV_SETTING(HD_ENABLE_PACKED_NORMALS, 1,
                      "Use packed normals");

HdStMesh::HdStMesh(SdfPath const& id,
                   SdfPath const& instancerId)
    : HdMesh(id, instancerId)
    , _topology()
    , _topologyId(0)
    , _vertexPrimvarId(0)
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

void
HdStMesh::Sync(HdSceneDelegate *delegate,
               HdRenderParam   *renderParam,
               HdDirtyBits     *dirtyBits,
               TfToken const   &reprName,
               bool             forcedRepr)
{
    TF_UNUSED(renderParam);

    // Store the dirty bits because the rprim Sync() might clean
    // the DirtySurfaceShader bits which are useful later in 
    // _GetRepr() to detech if the GeometricShader needs to be updated.
    // Example : An rprim has a binding to a shader without displacement,
    //           later on, we update that binding to point to a shader 
    //           with displacement. We want the Geometric Shader to be updated.
    HdDirtyBits originalDirtyBits = *dirtyBits;

    HdRprim::_Sync(delegate,
                  reprName,
                  forcedRepr,
                  &originalDirtyBits);

    TfToken calcReprName = _GetReprName(delegate, reprName,
                                        forcedRepr, dirtyBits);
    _GetRepr(delegate, calcReprName, dirtyBits);

    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}


static bool
_IsEnabledForceQuadrangulate()
{
    static bool enabled = (TfGetEnvSetting(HD_ENABLE_FORCE_QUADRANGULATE) == 1);
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
HdStMesh::_GetRefineLevelForDesc(HdMeshReprDesc desc)
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
HdStMesh::_PopulateTopology(HdSceneDelegate *sceneDelegate,
                            HdDrawItem *drawItem,
                            HdDirtyBits *dirtyBits,
                            HdMeshReprDesc desc)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        boost::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());

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
        int refineLevel = GetRefineLevel(sceneDelegate);
        HdMeshTopology meshTopology = GetMeshTopology(sceneDelegate);

        // If the topology requires none subdivision scheme then force
        // refinement level to be 0 since we do not want subdivision.
        if (meshTopology.GetScheme() == PxOsdOpenSubdivTokens->none) {
            refineLevel = 0;
        }

        HdSt_MeshTopologySharedPtr topology =
                    HdSt_MeshTopology::New(meshTopology, refineLevel);
        if (refineLevel > 0) {
            // add subdiv tags before compute hash
            // XXX: calling GetSubdivTags on implicit prims raises an error.
            topology->SetSubdivTags(GetSubdivTags(sceneDelegate));
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
        bool usePtexIndices = _UsePtexIndices(sceneDelegate->GetRenderIndex());
        _topologyId = ArchHash64((const char*)&usePtexIndices,
            sizeof(usePtexIndices), _topologyId);

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
                            HdRenderContextCaps::GetInstance().gpuComputeEnabled,
                            id, resourceRegistry.get());
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
            } else if (_UsePtexIndices(sceneDelegate->GetRenderIndex())) {
                // not refined = quadrangulate
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
                sceneDelegate->GetRenderIndex().GetChangeTracker().SetGarbageCollectionNeeded();
            }
        }

        // TODO: reuse same range for varying topology
        _sharedData.barContainer.Set(drawItem->GetDrawingCoord()->GetTopologyIndex(),
                                     rangeInstance.GetValue());
    }
}

void
HdStMesh::_PopulateAdjacency(HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // The topology may be null in the event that it has zero faces.
    if (!_topology) return;

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

        if (HdRenderContextCaps::GetInstance().gpuComputeEnabled) {
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

static HdBufferSourceSharedPtr
_QuadrangulatePrimVar(HdBufferSourceSharedPtr const &source,
                      HdComputationVector *computations,
                      HdSt_MeshTopologySharedPtr const &topology,
                      SdfPath const &id,
                      HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    if (!TF_VERIFY(computations)) return source;

    if (!HdRenderContextCaps::GetInstance().gpuComputeEnabled) {
        // CPU quadrangulation
        // set quadrangulation as source instead of original source.
        HdBufferSourceSharedPtr quadsource =
            topology->GetQuadrangulateComputation(source, id);

        if (quadsource) {
            // don't transfer source to gpu, it needs to be quadrangulated.
            // It will be resolved as a pre-chained source.
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
        if (computation) {
            computations->push_back(computation);
        }
        return source;
    }
}

static HdBufferSourceSharedPtr
_QuadrangulateFaceVaryingPrimVar(HdBufferSourceSharedPtr const &source,
                                 HdSt_MeshTopologySharedPtr const &topology,
                                 SdfPath const &id,
                                 HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    // note: currently we don't support GPU facevarying quadrangulation.

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
                               SdfPath const &id,
                               HdStResourceRegistrySharedPtr const &resourceRegistry)
{
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

    if (!HdRenderContextCaps::GetInstance().gpuComputeEnabled) {
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
HdStMesh::_PopulateVertexPrimVars(HdSceneDelegate *sceneDelegate,
                                  HdDrawItem *drawItem,
                                  HdDirtyBits *dirtyBits,
                                  bool requireSmoothNormals)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

    HdStResourceRegistrySharedPtr const &resourceRegistry =
        boost::static_pointer_cast<HdStResourceRegistry>(
        renderIndex.GetResourceRegistry());

    // The "points" attribute is expected to be in this list.
    TfTokenVector primVarNames = GetPrimVarVertexNames(sceneDelegate);

    // Track the last vertex index to distinguish between vertex and varying
    // while processing.
    int vertexPartitionIndex = int(primVarNames.size()-1);

    // Add varying primvars.
    TfTokenVector const& varyingNames = GetPrimVarVaryingNames(sceneDelegate);
    primVarNames.reserve(primVarNames.size() + varyingNames.size());
    primVarNames.insert(primVarNames.end(),
                        varyingNames.begin(), varyingNames.end());

    HdBufferSourceVector sources;
    sources.reserve(primVarNames.size());
    HdComputationVector computations;

    int numPoints = _topology ? _topology->GetNumPoints() : 0;
    int refineLevel = _topology ? _topology->GetRefineLevel() : 0;

    bool cpuSmoothNormals =
        (!HdRenderContextCaps::GetInstance().gpuComputeEnabled);

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

    HdBufferSourceSharedPtr points;

    // Track index to identify varying primvars.
    int i = 0;
    TF_FOR_ALL(nameIt, primVarNames) {
        // If the index is greater than the last vertex index, isVarying=true.
        bool isVarying = i++ > vertexPartitionIndex;

        if (!HdChangeTracker::IsPrimVarDirty(*dirtyBits, id, *nameIt)) {
            continue;
        }

        // TODO: We don't need to pull primvar metadata every time a
        // value changes, but we need support from the delegate.

        VtValue value =  GetPrimVar(sceneDelegate, *nameIt);

        if (!value.IsEmpty()) {
            HdBufferSourceSharedPtr source(
                new HdVtBufferSource(*nameIt, value));

            // verify primvar length
            if (source->GetNumElements() != numPoints) {
                TF_WARN(
                    "# of points mismatch (%d != %d) for primvar %s, prim %s",
                    source->GetNumElements(), numPoints,
                    nameIt->GetText(), id.GetText());

                if (*nameIt == HdTokens->points) {
                    // If points data is invalid, it pretty much invalidates
                    // the whole prim.  Drop the Bar, to invalidate the prim and
                    // stop further processing.
                    _sharedData.barContainer.Set(
                           drawItem->GetDrawingCoord()->GetVertexPrimVarIndex(),
                           HdBufferArrayRangeSharedPtr());
                    return;
                }

                continue;
            }

            if (refineLevel > 0) {
                source = _RefinePrimVar(source, isVarying,
                                        &computations, _topology);
            } else if (_UsePtexIndices(renderIndex)) {
                source = _QuadrangulatePrimVar(source, &computations, _topology,
                                               GetId(), resourceRegistry);
            }
            sources.push_back(source);

            // Special handling of points primvar.
            // We need to capture state about the points primvar
            // for use with smooth normal computation.
            if (*nameIt == HdTokens->points) {
                points = source; // For CPU Smooth Normals
            }
        }
    }

    // Take local copy of packed normal state, so we can
    // detect transitions from packed to unpacked normals.
    bool usePackedNormals = _packedNormals;

    if (requireSmoothNormals &&
        (*dirtyBits & DirtySmoothNormals)) {
        // note: normals gets dirty when points are marked as dirty,
        // at changetracker.

        // clear DirtySmoothNormals (this is not a scene dirtybit)
        *dirtyBits &= ~DirtySmoothNormals;

        TF_VERIFY(_vertexAdjacency);
        bool doRefine = (refineLevel > 0);
        bool doQuadrangulate = _UsePtexIndices(renderIndex);

        // we can't use packed normals for refined/quad,
        // let's migrate the buffer to full precision
        usePackedNormals &= !(doRefine || doQuadrangulate);

        TfToken normalsName = usePackedNormals ? HdTokens->packedNormals :
                                                 HdTokens->normals;

        // The smooth normals computation uses the points primvar as a source.
        //
        if (cpuSmoothNormals) {
            // CPU smooth normals require the points source data
            // So it is expected to be dirty.  So if the
            // points variable is not set it means the points primvar is
            // missing or invalid, so we skip smooth normals.
            if (points) {
                // CPU smooth normals depends on CPU adjacency.
                //
                HdBufferSourceSharedPtr normal =
                        _vertexAdjacency->GetSmoothNormalsComputation(
                                                              points,
                                                              normalsName,
                                                              usePackedNormals);

                if (doRefine) {
                    normal = _RefinePrimVar(normal, /*varying=*/false,
                                                      &computations, _topology);
                } else if (doQuadrangulate) {
                    normal = _QuadrangulatePrimVar(normal,
                                                   &computations,
                                                   _topology,
                                                   id,
                                                   resourceRegistry);
                }

                sources.push_back(normal);
            }
        } else {
            // GPU smooth normals doesn't need to have an explicit dependency.
            // The adjacency table should be committed before execution.

            // GPU smooth normals also uses the points primvar as input.
            // However, it might have already been copied to a GPU buffer
            // resource in a previous Sync.
            //
            // However, we do need to determine the type of the points buffer
            // so we either use the new points source or the GPU resource
            // to determine the type.
            //
            // One gotcha, is that the topology might have changed, such that
            // the GPU resource no-longer matches the topology.  Typically,
            // the points primvar would be updated at the same time, but
            // the new source might be invalid, so the GPU buffer didn't
            // get updated.
            //
            // Therefore, the code needs to check that the gpu buffer is
            // valid for the current topology before using it.


            GLenum pointsDataType = GL_NONE;
            if (points) {
                pointsDataType = points->GetGLComponentDataType();
                    } else {
                HdBufferArrayRangeSharedPtr const &bar =
                    drawItem->GetVertexPrimVarRange();

                if (bar) {
                    if (bar->IsValid()) {
                        HdBufferArrayRangeGLSharedPtr bar_ =
                            boost::static_pointer_cast<HdBufferArrayRangeGL>
                                                                          (bar);
                        HdBufferResourceGLSharedPtr pointsResource =
                                            bar_->GetResource(HdTokens->points);

                        if (pointsResource) {
                            pointsDataType = pointsResource->GetGLDataType();
                        }
                    }
                }
            }


            if (pointsDataType != GL_NONE) {
                // determine datatype. if we're updating points too, ask the
                // buffer source. Otherwise (if we're updating just normals)
                // ask delegate.
            // This is very unfortunate. Can we force normals to be always
            // float? (e.g. when switing flat -> smooth first time).
                GLenum normalsDataType = usePackedNormals ?
                                         GL_INT_2_10_10_10_REV : pointsDataType;
                computations.push_back(
                        _vertexAdjacency->GetSmoothNormalsComputationGPU(
                                                              HdTokens->points,
                                                              normalsName,
                                                              pointsDataType,
                                                              normalsDataType));

                // note: we haven't had explicit dependency for GPU
                // computations just yet. Currently they are executed
                // sequentially, so the dependency is expressed by
                // registering order.
                if (doRefine) {
                    HdComputationSharedPtr computation =
                        _topology->GetOsdRefineComputationGPU(
                                         HdTokens->normals, normalsDataType, 3);

                    // computation can be null for empty mesh
                    if (computation) {
                        computations.push_back(computation);
                    }
                } else if (doQuadrangulate) {
                    HdComputationSharedPtr computation =
                        _topology->GetQuadrangulateComputationGPU(
                                   HdTokens->normals, normalsDataType, GetId());

                    // computation can be null for all-quad mesh
                    if (computation) {
                            computations.push_back(computation);
                    }
                }
            }
        }
    }

    // return before allocation if it's empty.
    if (sources.empty() && computations.empty()) {
        return;
    }

    // new buffer specs
    HdBufferSpecVector bufferSpecs;
    HdBufferSpec::AddBufferSpecs(&bufferSpecs, sources);
    HdBufferSpec::AddBufferSpecs(&bufferSpecs, computations);

    HdBufferArrayRangeSharedPtr const &bar = drawItem->GetVertexPrimVarRange();
    if ((!bar) || (!bar->IsValid())) {
        // allocate new range
        HdBufferArrayRangeSharedPtr range;
        if (_IsEnabledSharedVertexPrimvar()) {
            // see if we can share an immutable primvar range
            // include topology and other topological computations
            // in the sharing id so that we can take into account
            // sharing of computed primvar data.
            _vertexPrimvarId = _ComputeSharedPrimvarId(_topologyId,
                                                       sources,
                                                       computations);

            bool isFirstInstance = true;
            range = _GetSharedPrimvarRange(_vertexPrimvarId,
                                           bufferSpecs, nullptr,
                                           &isFirstInstance,
                                           resourceRegistry);
            if (!isFirstInstance) {
                // this is not the first instance, skip redundant
                // sources and computations.
                sources.clear();
                computations.clear();
            }

        } else {
            range = resourceRegistry->AllocateNonUniformBufferArrayRange(
                                              HdTokens->primVar, bufferSpecs);
        }

        _sharedData.barContainer.Set(
            drawItem->GetDrawingCoord()->GetVertexPrimVarIndex(), range);

    } else {
        // already have a valid range, but the new repr may have
        // added additional items (smooth normals) or we may be transitioning
        // to unpacked normals
        bool isNew = (*dirtyBits & HdChangeTracker::NewRepr) ||
                     (usePackedNormals != _packedNormals);

        HdBufferArrayRangeSharedPtr range = bar;

        if (bar->IsImmutable() && _IsEnabledSharedVertexPrimvar()) {
            if (isNew) {
                // see if we can share an immutable buffer primvar range
                // include our existing sharing id so that we can take
                // into account previously committed sources along
                // with our new sources and computations.
                _vertexPrimvarId = _ComputeSharedPrimvarId(_vertexPrimvarId,
                                                           sources,
                                                           computations);

                bool isFirstInstance = true;
                range = _GetSharedPrimvarRange(_vertexPrimvarId,
                                               bufferSpecs, bar,
                                               &isFirstInstance,
                                               resourceRegistry);

                if (!isFirstInstance) {
                    // this is not the first instance, skip redundant
                    // sources and computations.
                    sources.clear();
                    computations.clear();
                }

            } else {
                // something is going to change and the existing bar
                // is immutable, migrate to a mutable buffer array
                _vertexPrimvarId = 0;
                range = resourceRegistry->MergeNonUniformBufferArrayRange(
                            HdTokens->primVar, bufferSpecs, bar);
            }
        } else if (isNew) {
            // the range was created by other repr. check compatibility.
            range = resourceRegistry->MergeNonUniformBufferArrayRange(
                                           HdTokens->primVar, bufferSpecs, bar);
        }

        if (range != bar) {
            _sharedData.barContainer.Set(
                drawItem->GetDrawingCoord()->GetVertexPrimVarIndex(), range);

            // If buffer migration actually happens, the old buffer will no
            // longer be needed, and GC is required to reclaim their memory.
            // But we don't trigger GC here for now, since it ends up
            // to make all collections dirty (see HdEngine::Draw),
            // which can be expensive.
            // (in other words, we should fix bug 103767:
            //  "Optimize varying topology buffer updates" first)
            //
            // if (range != bar) {
            //    _GetRenderIndex().GetChangeTracker().
            //                                     SetGarbageCollectionNeeded();
            // }

            // set deep invalidation to rebuild draw batch
            renderIndex.GetChangeTracker().MarkShaderBindingsDirty();
        }
    }

    // Now we've finished transitioning from packed to unpacked normals
    // so update the current state.
    _packedNormals = usePackedNormals;

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
HdStMesh::_PopulateFaceVaryingPrimVars(HdSceneDelegate *sceneDelegate,
                                       HdDrawItem *drawItem,
                                       HdDirtyBits *dirtyBits,
                                       HdMeshReprDesc desc)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    TfTokenVector primVarNames = GetPrimVarFacevaryingNames(sceneDelegate);
    if (primVarNames.empty()) return;

    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        boost::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());

    HdBufferSourceVector sources;
    sources.reserve(primVarNames.size());

    int refineLevel = _GetRefineLevelForDesc(desc);
    int numFaceVaryings = _topology ? _topology->GetNumFaceVaryings() : 0;

    TF_FOR_ALL(nameIt, primVarNames) {
        // note: facevarying primvars don't have to be refined.
        if (!HdChangeTracker::IsPrimVarDirty(*dirtyBits, id,*nameIt)) {
            continue;
        }

        VtValue value = GetPrimVar(sceneDelegate, *nameIt);
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
            if (_UsePtexIndices(sceneDelegate->GetRenderIndex()) ||
                refineLevel > 0) {
                source = _QuadrangulateFaceVaryingPrimVar(source, _topology,
                                                          GetId(), resourceRegistry);
            } else {
                source = _TriangulateFaceVaryingPrimVar(source, _topology,
                                                        GetId(), resourceRegistry);
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
HdStMesh::_PopulateElementPrimVars(HdSceneDelegate *sceneDelegate,
                                   HdDrawItem *drawItem,
                                   HdDirtyBits *dirtyBits,
                                   TfTokenVector const &primVarNames)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        boost::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());


    HdBufferSourceVector sources;
    sources.reserve(primVarNames.size());

    int numFaces = _topology ? _topology->GetNumFaces() : 0;

    TF_FOR_ALL(nameIt, primVarNames) {
        if (!HdChangeTracker::IsPrimVarDirty(*dirtyBits, id, *nameIt))
            continue;

        VtValue value = GetPrimVar(sceneDelegate, *nameIt);
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
HdStMesh::_UsePtexIndices(const HdRenderIndex &renderIndex) const
{
    const HdShader *shader = static_cast<const HdShader *>(
                                  renderIndex.GetSprim(HdPrimTypeTokens->shader,
                                                       GetMaterialId()));
    if (shader == nullptr) {
        shader = static_cast<const HdShader *>(
                        renderIndex.GetFallbackSprim(HdPrimTypeTokens->shader));
    }

    HdShaderCodeSharedPtr ss = shader->GetShaderCode();

    TF_FOR_ALL(it, ss->GetParams()) {
        if (it->IsPtex())
            return true;
    }

    // Fallback to the environment variable, which allows forcing of
    // quadrangulation for debugging/testing.
    return _IsEnabledForceQuadrangulate();
}

HdShaderCodeSharedPtr
HdStMesh::_GetShaderCode(HdSceneDelegate *sceneDelegate, HdShader const *shader) const
{
    VtValue mixinValue = GetShadingStyle(sceneDelegate);
    if (!mixinValue.IsEmpty()) {
        TfToken mixin = mixinValue.GetWithDefault<TfToken>();
        /// XXX In the future this could be a place to pull on a surface entry
        /// to get a custom terminal.
        if (!mixin.IsEmpty()) {
            std::string mixinSource = 
                sceneDelegate->GetMixinShaderSource(mixin);
            // Don't create code if the delegate returned an empty mixin source
            // from the given mixin.
            if (!mixinSource.empty()) {
                return HdShaderCodeSharedPtr(
                    new HdStMixinShaderCode(
                        mixinSource, shader->GetShaderCode()));
            }
        }
    }
    return shader->GetShaderCode();
}

void
HdStMesh::_UpdateDrawItem(HdSceneDelegate *sceneDelegate,
                          HdDrawItem *drawItem,
                          HdDirtyBits *dirtyBits,
                          HdMeshReprDesc desc,
                          bool requireSmoothNormals)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();

    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        boost::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());

    /* VISIBILITY */
    _UpdateVisibility(sceneDelegate, dirtyBits);


    /* CONSTANT PRIMVARS */
    // This will call _GetShaderCode to resolve per rprim shader mixins
    _PopulateConstantPrimVars(sceneDelegate, drawItem, dirtyBits);

    /* INSTANCE PRIMVARS */
    if (!GetInstancerId().IsEmpty()) {
        HdStInstancer *instancer = static_cast<HdStInstancer*>(
            sceneDelegate->GetRenderIndex().GetInstancer(GetInstancerId()));
        if (TF_VERIFY(instancer)) {
            instancer->PopulateDrawItem(drawItem, &_sharedData,
                dirtyBits, InstancePrimVar);
        }
    }

    /* TOPOLOGY */
    // XXX: _PopulateTopology should be split into two phase
    //      for scene dirtybits and for repr dirtybits.
    if (*dirtyBits & (HdChangeTracker::DirtyTopology
                    | HdChangeTracker::DirtyRefineLevel
                    | HdChangeTracker::DirtySubdivTags
                                     | DirtyIndices
                                     | DirtyHullIndices
                                     | DirtyPointsIndices)) {
        _PopulateTopology(sceneDelegate, drawItem, dirtyBits, desc);
    }

    if (*dirtyBits & HdChangeTracker::DirtyDoubleSided) {
        _doubleSided = IsDoubleSided(sceneDelegate);
    }
    if (*dirtyBits & HdChangeTracker::DirtyCullStyle) {
        _cullStyle = GetCullStyle(sceneDelegate);
    }

    // disable smoothNormals for bilinear and none scheme mesh.
    // normal dirtiness will be cleared without computing/populating normals.
    TfToken scheme = _topology->GetScheme();
    if (scheme == PxOsdOpenSubdivTokens->bilinear ||
        scheme == PxOsdOpenSubdivTokens->none) {
        requireSmoothNormals = false;
    }

    if (requireSmoothNormals && !_vertexAdjacency) {
        _PopulateAdjacency(resourceRegistry);
    }

    /* FACEVARYING PRIMVARS */
    if (HdChangeTracker::IsAnyPrimVarDirty(*dirtyBits, id)) {
        _PopulateFaceVaryingPrimVars(sceneDelegate, drawItem, dirtyBits, desc);
    }

    /* VERTEX PRIMVARS */
    if ((*dirtyBits & HdChangeTracker::NewRepr) ||
        (HdChangeTracker::IsAnyPrimVarDirty(*dirtyBits, id))) {
        _PopulateVertexPrimVars(sceneDelegate, drawItem, dirtyBits,
                                requireSmoothNormals);
    }

    /* ELEMENT PRIMVARS */
    if (HdChangeTracker::IsAnyPrimVarDirty(*dirtyBits, id)) {
        TfTokenVector uniformPrimVarNames =
                                         GetPrimVarUniformNames(sceneDelegate);
        if (!uniformPrimVarNames.empty()) {
            _PopulateElementPrimVars(sceneDelegate, drawItem, dirtyBits,
                                     uniformPrimVarNames);
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

void
HdStMesh::_UpdateDrawItemGeometricShader(HdSceneDelegate *sceneDelegate,
                                         HdDrawItem *drawItem,
                                         const HdMeshReprDesc &desc)
{
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

    bool hasFaceVaryingPrimVars =
        (bool)drawItem->GetFaceVaryingPrimVarRange();

    int refineLevel = _GetRefineLevelForDesc(desc);

    Hd_GeometricShader::PrimitiveType primType =
        Hd_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_TRIANGLES;

    if (desc.geomStyle == HdMeshGeomStylePoints) {
        primType = Hd_GeometricShader::PrimitiveType::PRIM_POINTS;
    } else if (refineLevel > 0) {
        if (_topology->RefinesToTriangles()) {
            // e.g. loop subdivision.
            primType =
                Hd_GeometricShader::PrimitiveType::PRIM_MESH_REFINED_TRIANGLES;
        } else if (_topology->RefinesToBSplinePatches()) {
            primType = Hd_GeometricShader::PrimitiveType::PRIM_MESH_PATCHES;
        } else {
            // uniform catmark/bilinear subdivision generates quads.
            primType =
                Hd_GeometricShader::PrimitiveType::PRIM_MESH_REFINED_QUADS;
        }
    } else if (_UsePtexIndices(renderIndex)) {
        // quadrangulate coarse mesh (for ptex)
        primType = Hd_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_QUADS;
    }

    // resolve geom style, cull style
    HdCullStyle cullStyle = _cullStyle;
    HdMeshGeomStyle geomStyle = desc.geomStyle;

    // We need to use smoothNormals flag per repr (and not requireSmoothNormals)
    // here since the geometric shader needs to know if we are actually
    // using normals or not.
    bool smoothNormals = desc.smoothNormals &&
        _topology->GetScheme() != PxOsdOpenSubdivTokens->bilinear &&
        _topology->GetScheme() != PxOsdOpenSubdivTokens->none;

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

    // check if the shader bound to this mesh has a custom displacement shader, 
    // if so, we want to make sure the geometric shader does not optimize the
    // geometry shader out of the code.
    bool hasCustomDisplacementTerminal = false;
    const HdStShader *shader = static_cast<const HdStShader *>(
        renderIndex.GetSprim(HdPrimTypeTokens->shader, GetMaterialId()));
    if (shader) {
        const std::string & displacementShader = 
            shader->GetDisplacementShaderSource(sceneDelegate);
        if (!displacementShader.empty()) {
            hasCustomDisplacementTerminal = true;
        }
    }

    // create a shaderKey and set to the geometric shader.
    HdSt_MeshShaderKey shaderKey(primType,
                                 desc.shadingTerminal,
                                 hasCustomDisplacementTerminal,
                                 smoothNormals,
                                 _doubleSided,
                                 hasFaceVaryingPrimVars,
                                 blendWireframeColor,
                                 cullStyle,
                                 geomStyle);

    Hd_GeometricShaderSharedPtr geomShader = Hd_GeometricShader::Create(
            shaderKey, renderIndex.GetResourceRegistry());

    TF_VERIFY(geomShader);

    drawItem->SetGeometricShader(geomShader);

    // The batches need to be validated and rebuilt if necessary.
    renderIndex.GetChangeTracker().MarkShaderBindingsDirty();
}

// virtual
HdDirtyBits
HdStMesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    // If subdiv tags are dirty, topology needs to be recomputed.
    // The latter implies we'll need to recompute all primvar data.
    // Any data fetched by the scene delegate should be marked dirty here.
    if (bits & HdChangeTracker::DirtySubdivTags) {
        bits |= (HdChangeTracker::DirtyPoints   |
                HdChangeTracker::DirtyNormals  |
                HdChangeTracker::DirtyPrimVar  |
                HdChangeTracker::DirtyTopology |
                HdChangeTracker::DirtyRefineLevel);
    } else if (bits & HdChangeTracker::DirtyTopology) {
        // Unlike basis curves, we always request refineLevel when topology is
        // dirty
        bits |= HdChangeTracker::DirtySubdivTags |
                HdChangeTracker::DirtyRefineLevel;
    }

    // propagate scene-based dirtyBits into rprim-custom dirtyBits
    if (bits & HdChangeTracker::DirtyPoints) {
        bits |= _customDirtyBitsInUse & DirtySmoothNormals;
    }

    if (bits & HdChangeTracker::DirtyTopology) {
        bits |= _customDirtyBitsInUse &
                   (DirtyIndices      |
                    DirtyHullIndices  |
                    DirtyPointsIndices);
    }

    // If smooth Normals are dirty and we are doing CPU smooth normals
    // then the smooth normals computation needs the Points primVar
    // so mark Points as dirty, so that the scene delegate will provide
    // the data.
    if ((bits & DirtySmoothNormals) &&
        (!HdRenderContextCaps::GetInstance().gpuComputeEnabled)) {
        bits |= HdChangeTracker::DirtyPoints;
    }

    return bits;
}

void
HdStMesh::_InitRepr(TfToken const &reprName, HdDirtyBits *dirtyBits)
{
    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprName));
    bool isNew = it == _reprs.end();
    if (isNew) {
        // add new repr
        _reprs.emplace_back(reprName, boost::make_shared<HdRepr>());
        HdReprSharedPtr &repr = _reprs.back().second;

        // set dirty bit to say we need to sync a new repr (buffer array
        // ranges may change)
        *dirtyBits |= HdChangeTracker::NewRepr;

        _MeshReprConfig::DescArray descs = _GetReprDesc(reprName);

        // allocate all draw items
        for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
            const HdMeshReprDesc &desc = descs[descIdx];

            if (desc.geomStyle == HdMeshGeomStyleInvalid) continue;

            // redirect hull topology to extra slot
            HdDrawItem *drawItem = repr->AddDrawItem(&_sharedData);
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

}

HdReprSharedPtr const &
HdStMesh::_GetRepr(HdSceneDelegate *sceneDelegate,
                   TfToken const &reprName,
                   HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _MeshReprConfig::DescArray reprDescs = _GetReprDesc(reprName);
    _ReprVector::iterator reprIt = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprName));

    if (reprIt == _reprs.end()) {
        // Hydra should have called _InitRepr earlier in sync when
        // before sending dirty bits to the delegate.
        TF_CODING_ERROR("_InitRepr() should be called for repr %s on prim %s.",
                        reprName.GetText(), GetId().GetText());

        static const HdReprSharedPtr ERROR_RETURN;
        return ERROR_RETURN;
    }

    // _reprs holds a pair of (TfToken, HdReprSharedPtr)
    HdReprSharedPtr const &curRepr = reprIt->second;

    if (TfDebug::IsEnabled(HD_RPRIM_UPDATED)) {
        std::cout << "HdStMesh::GetRepr " << GetId()
                  << " Repr = " << reprName << "\n";
        HdChangeTracker::DumpDirtyBits(*dirtyBits);
    }

    // For the bits geometric shader depends on, reset the geometric shaders
    // for all the draw items of all the reprs.
    if (*dirtyBits & (HdChangeTracker::DirtyRefineLevel|
                      HdChangeTracker::DirtyCullStyle|
                      HdChangeTracker::DirtyDoubleSided|
                      HdChangeTracker::DirtyMaterialId)) {
        TF_DEBUG(HD_RPRIM_UPDATED).
            Msg("HdStMesh - Resetting the geometric shader for all draw"
                " items of all reprs");

        TF_FOR_ALL (it, _reprs) {
            TF_FOR_ALL (drawItem, *(it->second->GetDrawItems())) {
                drawItem->SetGeometricShader(Hd_GeometricShaderSharedPtr());
            }
        }
    }

    // iterate through all reprdescs for the current repr to figure out if any 
    // of them requires smoothnormals
    // if so we will calculate the normals once (clean the bits) and reuse them.
    // This is important for modes like FeyRay which requires 2 draw items
    // and one requires smooth normals but the other doesn't.
    bool requireSmoothNormals = false;
    for (size_t descIdx = 0; descIdx < reprDescs.size(); ++descIdx) {
        const HdMeshReprDesc &desc = reprDescs[descIdx];
        if (desc.smoothNormals) {
            requireSmoothNormals = true;
            break;
        }
    }

    // Note: We only update/set the geometric shaders for the draw items of 
    // the incoming 'reprName'. The draw items corresponding to other reprs
    // may or may not be set.     
    int drawItemIndex = 0;
    for (size_t descIdx = 0; descIdx < reprDescs.size(); ++descIdx) {
        const HdMeshReprDesc &desc = reprDescs[descIdx];

        if (desc.geomStyle != HdMeshGeomStyleInvalid) {
            HdDrawItem *drawItem = curRepr->GetDrawItem(drawItemIndex++);

            if (HdChangeTracker::IsDirty(*dirtyBits)) {
                _UpdateDrawItem(sceneDelegate, drawItem, dirtyBits, desc,
                        requireSmoothNormals);
            } 
            
            if (!drawItem->GetGeometricShader()) {
                // None of the draw items should have an unset geometric shader
                // Can be the case after a collection rebuild of the draw items.
                _UpdateDrawItemGeometricShader(sceneDelegate, drawItem, desc);
            }
        }
    }

    *dirtyBits &= ~HdChangeTracker::NewRepr;


    return curRepr;
}

HdDirtyBits
HdStMesh::_GetInitialDirtyBits() const
{
    HdDirtyBits mask = HdChangeTracker::Clean
        | HdChangeTracker::InitRepr
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
        | HdChangeTracker::DirtyMaterialId
        | HdChangeTracker::DirtyTopology
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        ;

    return mask;
}

PXR_NAMESPACE_CLOSE_SCOPE

