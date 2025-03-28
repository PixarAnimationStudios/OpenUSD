//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_SMOOTH_NORMALS_H
#define PXR_IMAGING_HD_SMOOTH_NORMALS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"

PXR_NAMESPACE_OPEN_SCOPE


class Hd_VertexAdjacency;

/// \class Hd_SmoothNormals
///
/// Hd_SmoothNormals encapsulates mesh smooth normals information.
/// It uses passed-in adjacency information and points data to compute
/// smooth per-vertex normals.  It does this by averaging face normals of
/// faces surrounding a vertex.
///
class Hd_SmoothNormals final
{
public:
    /// Computes the smooth normals result using the supplied adjacency
    /// information and points data. Returns an array of the same size and
    /// type as the source points, with optional packing.
    HD_API
    static VtArray<GfVec3f> ComputeSmoothNormals(
                                          Hd_VertexAdjacency const * adjacency,
                                          int numPoints,
                                          GfVec3f const * pointsPtr);
    HD_API
    static VtArray<GfVec3d> ComputeSmoothNormals(
                                          Hd_VertexAdjacency const * adjacency,
                                          int numPoints,
                                          GfVec3d const * pointsPtr);
    HD_API
    static VtArray<HdVec4f_2_10_10_10_REV> ComputeSmoothNormalsPacked(
                                          Hd_VertexAdjacency const * adjacency,
                                          int numPoints,
                                          GfVec3f const * pointsPtr);
    HD_API
    static VtArray<HdVec4f_2_10_10_10_REV> ComputeSmoothNormalsPacked(
                                          Hd_VertexAdjacency const * adjacency,
                                          int numPoints,
                                          GfVec3d const * pointsPtr);

private:
    Hd_SmoothNormals() = delete;
    ~Hd_SmoothNormals() = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_SMOOTH_NORMALS_H
