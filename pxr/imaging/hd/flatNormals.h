//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_FLAT_NORMALS_H
#define PXR_IMAGING_HD_FLAT_NORMALS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"

#include "pxr/imaging/hd/types.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"

PXR_NAMESPACE_OPEN_SCOPE


class HdMeshTopology;

/// \class Hd_FlatNormals
///
/// Hd_FlatNormals encapsulates mesh flat normals information.
/// It uses passed-in face index data and points data to compute
/// flat per-face normals.  It does this by breaking each face into
/// a triangle fan centered at vertex 0, and averaging triangle normals.
///
class Hd_FlatNormals final
{
public:
    /// Computes the flat normals result using the supplied face coord
    /// information and points data. Returns an array of the same size and
    /// type as the source points, with optional packing.
    HD_API
    static VtArray<GfVec3f> ComputeFlatNormals(
                                        HdMeshTopology const * topology,
                                        GfVec3f const * pointsPtr);
    HD_API
    static VtArray<GfVec3d> ComputeFlatNormals(
                                        HdMeshTopology const * topology,
                                        GfVec3d const * pointsPtr);
    HD_API
    static VtArray<HdVec4f_2_10_10_10_REV> ComputeFlatNormalsPacked(
                                        HdMeshTopology const * topology,
                                        GfVec3f const * pointsPtr);
    HD_API
    static VtArray<HdVec4f_2_10_10_10_REV> ComputeFlatNormalsPacked(
                                        HdMeshTopology const * topology,
                                        GfVec3d const * pointsPtr);

private:
    Hd_FlatNormals() = delete;
    ~Hd_FlatNormals() = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_FLAT_NORMALS_H
