//
// Copyright 2020 benmalartre
//
// unlicensed
//
#pragma once

#include "pxr/pxr.h"
#include <pxr/imaging/glf/glew.h>
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include <mutex>
#include <unordered_map>
#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

/// forward declaration
class LoFiMesh;

/// \struct description of a LoFi mesh in the scene
struct LoFiMeshDesc 
{
  uint32_t            _numPoints;
  uint32_t            _numTriangles;
  uint32_t            _basePointIndex;
  uint32_t            _baseTriangleIndex;
  const GfVec3f*      _positions;
  const GfVec3f*      _colors;
  const GfVec3i*      _indices;
};

typedef std::unordered_map<int, LoFiMeshDesc> LoFiMeshDescMap;

/// \class LoFiScene
///
///
class LoFiScene
{
public:
    /// LoFiScene constructor.
    LoFiScene();

    /// LoFiMesh destructor.
    ~LoFiScene();

    /// Add a mesh
    int SetMesh(LoFiMesh* mesh);

    /// Remove a mesh
    void RemoveMesh(LoFiMesh* mesh);

    /// Num meshes in scene
    int NumMeshes();

    /// Get meshes descriptions map
    LoFiMeshDescMap& GetMeshes();

    /// Populate a mesh description
    void _PopulateMeshDesc(LoFiMesh* mesh, LoFiMeshDesc* desc);

    // This class does not support copying.
    LoFiScene(const LoFiScene&) = delete;
    LoFiScene &operator =(const LoFiScene&) = delete;

private:
  std::vector<GLuint>                       _vaos;
  LoFiMeshDescMap                           _meshes;
  std::mutex                                _lock;
  std::atomic_int                           _gId;

  friend class LoFiMesh;
};

PXR_NAMESPACE_CLOSE_SCOPE

