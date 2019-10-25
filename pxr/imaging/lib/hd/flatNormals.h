//
// Copyright 2018 Pixar
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
#ifndef HD_FLAT_NORMALS_H
#define HD_FLAT_NORMALS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/computation.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMeshTopology;

/// \class Hd_FlatNormals
///
/// Hd_FlatNormals encapsulates mesh flat normals information.
/// It uses passed-in face index data and points data to compute
/// flat per-face normals.  It does this by breaking each face into
/// a triangle fan centered at vertex 0, and averaging triangle normals.
///
class Hd_FlatNormals final {
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

/// \class Hd_FlatNormalsComputation
///
/// Flat normal computation CPU.
///
class Hd_FlatNormalsComputation : public HdComputedBufferSource {
public:
    HD_API
    Hd_FlatNormalsComputation(HdMeshTopology const *topology,
                              HdBufferSourceSharedPtr const &points,
                              TfToken const &dstName,
                              bool packed);

    /// overrides
    HD_API
    virtual void GetBufferSpecs(HdBufferSpecVector *specs) const override;
    HD_API
    virtual bool Resolve() override;
    HD_API
    virtual TfToken const &GetName() const override;

protected:
    HD_API
    virtual bool _CheckValid() const override;

private:
    HdMeshTopology const *_topology;
    HdBufferSourceSharedPtr const _points;
    TfToken _dstName;
    bool _packed;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_FLAT_NORMALS_H
