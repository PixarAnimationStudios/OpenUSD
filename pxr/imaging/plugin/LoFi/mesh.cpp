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
  _loFiId = -1;
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
LoFiMesh::Sync( HdSceneDelegate *sceneDelegate,
                HdRenderParam   *renderParam,
                HdDirtyBits     *dirtyBits,
                TfToken const   &reprToken)
{
  HD_TRACE_FUNCTION();
  HF_MALLOC_TAG_FUNCTION();

  _MeshReprConfig::DescArray descs = _GetReprDesc(reprToken);
  const HdMeshReprDesc& desc = descs[0];

  const SdfPath& id = GetId();

  if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id)) {
      _topology = HdMeshTopology(GetMeshTopology(sceneDelegate), 0);
      _adjacencyValid = false;
      topoDirty = true;
  }

  if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->points)) {
      VtValue value = sceneDelegate->Get(id, HdTokens->points);
      _points = value.Get<VtVec3fArray>();
      _normalsValid = false;
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
  
  // Clean all dirty bits.
  *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

PXR_NAMESPACE_CLOSE_SCOPE
