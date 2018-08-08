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
#include "pxr/imaging/hd/mesh.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdMeshReprDescTokens, HD_MESH_REPR_DESC_TOKENS);

HdMesh::HdMesh(SdfPath const& id,
               SdfPath const& instancerId)
    : HdRprim(id, instancerId)
{
    /*NOTHING*/
}

HdMesh::~HdMesh()
{
    /*NOTHING*/
}

// static repr configuration
HdMesh::_MeshReprConfig HdMesh::_reprDescConfig;

/* static */
void
HdMesh::ConfigureRepr(TfToken const &reprName,
                      HdMeshReprDesc desc1,
                      HdMeshReprDesc desc2)
{
    HD_TRACE_FUNCTION();

    _reprDescConfig.Append(reprName, _MeshReprConfig::DescArray{desc1, desc2});
}

/* static */
HdMesh::_MeshReprConfig::DescArray
HdMesh::_GetReprDesc(HdReprSelector const &reprSelector)
{
    return _reprDescConfig.Find(reprSelector.GetReprToken());
}

PXR_NAMESPACE_CLOSE_SCOPE

