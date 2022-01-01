//
// Copyright 2020 benmalartre
//
// unlicensed
//
#include <iostream>
#include "pxr/imaging/plugin/LoFi/scene.h"
#include "pxr/imaging/plugin/LoFi/mesh.h"


PXR_NAMESPACE_OPEN_SCOPE

LoFiScene::LoFiScene():_gId(0)
{
}

LoFiScene::~LoFiScene()
{
}

void 
LoFiScene::_PopulateMeshDesc(LoFiMesh* mesh, LoFiMeshDesc* desc)
{
  desc->_numPoints = mesh->GetNumPoints();
  desc->_numTriangles = mesh->GetNumTriangles();
  desc->_positions = mesh->GetPositionsPtr();
  desc->_indices = mesh->GetIndicesPtr();
}

LoFiMeshDescMap& LoFiScene::GetMeshes()
{
  return _meshes;
}

int LoFiScene::NumMeshes()
{
  return _meshes.size();
}

int LoFiScene::SetMesh(LoFiMesh* mesh)
{
  int loFiId = mesh->GetLoFiId();
  // check if mesh is in the scene
  if(loFiId > -1)
  {
    bool found = false;
    _lock.lock();
    // search for it
    auto search = _meshes.find(loFiId);
    if( search != _meshes.end())
    {
      LoFiMeshDesc* desc = &search->second;
      _PopulateMeshDesc(mesh, desc);
      found = true;
    }
    _lock.unlock();
    return loFiId;
  }
  // it was not there, we add it
  else
  {
    LoFiMeshDesc desc;
    _PopulateMeshDesc(mesh, &desc);
    _lock.lock();
    mesh->SetLoFiId(_gId);
    _meshes.insert(std::pair<int, LoFiMeshDesc>(_gId, desc));
    _gId++;
    _lock.unlock();
    return mesh->GetLoFiId();
  }

}

void LoFiScene::RemoveMesh(LoFiMesh* mesh)
{

}

PXR_NAMESPACE_CLOSE_SCOPE
