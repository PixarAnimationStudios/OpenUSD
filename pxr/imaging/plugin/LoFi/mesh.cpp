//
// Copyright 2020 Pixar
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
#include "pxr/imaging/plugin/LoFi/mesh.h"
#include "pxr/imaging/plugin/LoFi/renderParam.h"
#include "pxr/imaging/plugin/LoFi/renderPass.h"
#include "pxr/imaging/hd/extComputationUtils.h"
#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/smoothNormals.h"
#include "pxr/imaging/pxOsd/tokens.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

LoFiMesh::LoFiMesh(SdfPath const& id, SdfPath const& instancerId)
    : HdMesh(id, instancerId)
{
}

HdDirtyBits
LoFiMesh::GetInitialDirtyBitsMask() const
{
    // The initial dirty bits control what data is available on the first
    // run through _PopulateLoFiMesh(), so it should list every data item
    // that _PopulateLoFi requests.
    int mask = HdChangeTracker::Clean
        | HdChangeTracker::InitRepr
        | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyTopology
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyCullStyle
        | HdChangeTracker::DirtyDoubleSided
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyNormals
        | HdChangeTracker::DirtyInstancer
        ;

    return (HdDirtyBits)mask;
}

HdDirtyBits
LoFiMesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

void 
LoFiMesh::_InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits)
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
LoFiMesh::Sync(HdSceneDelegate *sceneDelegate,
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
  // For now, LoFiMesh only respects the first desc; this should be fixed.
  _MeshReprConfig::DescArray descs = _GetReprDesc(reprToken);
  const HdMeshReprDesc &desc = descs[0];
  // Pull top-level embree state out of the render param.
  LoFiRenderParam *lofiRenderParam =
      static_cast<LoFiRenderParam*>(renderParam);
  LoFiScene* scene = lofiRenderParam->GetScene();

  // Create lofi geometry objects.
  _PopulateLoFiMesh(sceneDelegate, scene, dirtyBits, desc);
}

void
LoFiMesh::_PopulateLoFiMesh(HdSceneDelegate* sceneDelegate,
                            LoFiScene*         scene,
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
        std::cout << "POINTS WAS NOT COMPUTED : NOW " << _points.size() << std::endl;
        _normalsValid = false;
    }

    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id)) {
        _topology = HdMeshTopology(GetMeshTopology(sceneDelegate), 0);
        _adjacencyValid = false;
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
    /*
    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->normals) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->widths) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->primvar)) {
        _UpdatePrimvarSources(sceneDelegate, *dirtyBits);
    }
    */

    ////////////////////////////////////////////////////////////////////////
    // 2. Resolve drawstyles

    // The repr defines a set of geometry styles for drawing the mesh
    // (see hd/enums.h). 
    // We treat Everything as HdMeshGeomStyleHull (coarse triangulated mesh).

    // The repr defines whether we should compute smooth normals for this mesh:
    // per-vertex normals taken as an average of adjacent faces, and
    // interpolated smoothly across faces.
    _smoothNormals = !desc.flatShadingEnabled;

    // If the scene delegate has provided authored normals, force us to not use
    // smooth normals.
    bool authoredNormals = false;
    if (_primvarSourceMap.count(HdTokens->normals) > 0) {
        authoredNormals = true;
    }
    _smoothNormals = _smoothNormals && !authoredNormals;

    ////////////////////////////////////////////////////////////////////////
    // 3. Populate lofi object.

    // If the topology has changed, we need to create or recreate 
    // the lofi mesh object.
    // _GetInitialDirtyBits() ensures that the topology is dirty the first time
    // this function is called, so that the lofi mesh is always created.
    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id)) {

        // Destroy the old mesh, if it exists.
        if (scene != NULL && _loFiId != -1)
          scene->RemoveMesh(this);

        // Triangulate the input faces.
        HdMeshUtil meshUtil(&_topology, GetId());
        meshUtil.ComputeTriangleIndices(&_triangulatedIndices,
            &_trianglePrimitiveParams);

        // Here we'll compute the toon line

        // Add the new mesh to the scene
        _loFiId = scene->SetMesh(this);
        if (_loFiId == -1) {
            TF_CODING_ERROR("Couldn't add LoFi mesh to the scene");
            return;
        }

        // Force the smooth normals code to rebuild the "normals" primvar the
        // next time smooth normals is enabled.
        _normalsValid = false;
    }

    /*
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
    if (!_smoothNormals && !authoredNormals) 
    {
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
    */
    ////////////////////////////////////////////////////////////////////////
    // 4. Populate embree instance objects.
    /*
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
            _GetInstanceContext(scene, i)->instanceId = i;
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
            _GetInstanceContext(scene, 0)->instanceId = 0;
        }
        if (newInstance || newMesh ||
            HdChangeTracker::IsTransformDirty(*dirtyBits, id) ||
            HdChangeTracker::IsPrimvarDirty(*dirtyBits, id,
                                            HdTokens->points)) {
            // Mark the instance as updated in the top-level BVH.
            rtcUpdate(scene, _rtcInstanceIds[0]);
        }
    }
    */
    // Clean all dirty bits.
    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

TfTokenVector
LoFiMesh::_UpdateComputedPrimvarSources(HdSceneDelegate* sceneDelegate,
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
            std::cout << "NUM POINTS : " << _points.size() << std::endl;
            _normalsValid = false;
        } else {
            _primvarSourceMap[compPrimvar.name] = {it->second,
                                                compPrimvar.interpolation};
        }
    }

    return compPrimvarNames;
}


PXR_NAMESPACE_CLOSE_SCOPE
