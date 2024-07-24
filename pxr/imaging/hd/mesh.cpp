//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdMeshReprDescTokens, HD_MESH_REPR_DESC_TOKENS);

HdMesh::HdMesh(SdfPath const& id)
    : HdRprim(id)
{
    /*NOTHING*/
}

HdMesh::~HdMesh() = default;

/* virtual */
TfTokenVector const &
HdMesh::GetBuiltinPrimvarNames() const
{
    static const TfTokenVector primvarNames = {
        HdTokens->points,
        HdTokens->normals,
    };
    return primvarNames;
}

// static repr configuration
HdMesh::_MeshReprConfig HdMesh::_reprDescConfig;

/* static */
void
HdMesh::ConfigureRepr(TfToken const &reprName,
                      HdMeshReprDesc desc1,
                      HdMeshReprDesc desc2/*=HdMeshReprDesc()*/)
{
    HD_TRACE_FUNCTION();

    _reprDescConfig.AddOrUpdate(
        reprName, _MeshReprConfig::DescArray{desc1, desc2});
}

/* static */
HdMesh::_MeshReprConfig::DescArray
HdMesh::_GetReprDesc(TfToken const &reprName)
{
    return _reprDescConfig.Find(reprName);
}

PXR_NAMESPACE_CLOSE_SCOPE

