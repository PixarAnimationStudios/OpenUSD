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
#include "mesh.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

HdTinyMesh::HdTinyMesh(SdfPath const& id, SdfPath const& instancerId)
    : HdMesh(id, instancerId)
{
}

HdDirtyBits
HdTinyMesh::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::Clean
        | HdChangeTracker::DirtyTransform;
}

HdDirtyBits
HdTinyMesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

void 
HdTinyMesh::_InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits)
{

}

void
HdTinyMesh::Sync(HdSceneDelegate *sceneDelegate,
                   HdRenderParam   *renderParam,
                   HdDirtyBits     *dirtyBits,
                   TfToken const   &reprToken)
{
    std::cout << "* (multithreaded) Sync Tiny Mesh id=" << GetId() << std::endl;
}

PXR_NAMESPACE_CLOSE_SCOPE
