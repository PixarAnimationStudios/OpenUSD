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
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_ENV_SETTING(HD_ENABLE_OPENSUBDIV3_ADAPTIVE, 0,
                      "Enables OpenSubdiv 3 Adaptive Tessellation");

HdMeshTopology::HdMeshTopology()
 : HdTopology()
 , _topology()
 , _invisiblePoints()
 , _invisibleFaces()
 , _refineLevel(0)
 , _numPoints()
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->meshTopology);
}

HdMeshTopology::HdMeshTopology(const HdMeshTopology &src,
                               int refineLevel)
 : HdTopology(src)
 , _topology(src.GetPxOsdMeshTopology())
 , _geomSubsets(src._geomSubsets)
 , _invisiblePoints(src._invisiblePoints)
 , _invisibleFaces(src._invisibleFaces)
 , _refineLevel(refineLevel)
 , _numPoints(src._numPoints)
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->meshTopology);
}

HdMeshTopology::HdMeshTopology(const PxOsdMeshTopology &topo,
                               int refineLevel /* = 0 */)
 : HdTopology()
 , _topology(topo)
 , _invisiblePoints()
 , _invisibleFaces()
 , _refineLevel(refineLevel)
 , _numPoints()
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->meshTopology);
    _numPoints = HdMeshTopology::ComputeNumPoints(
            _topology.GetFaceVertexIndices());
}

HdMeshTopology::HdMeshTopology(const TfToken &scheme,
                               const TfToken &orientation,
                               const VtIntArray &faceVertexCounts,
                               const VtIntArray &faceVertexIndices,
                               int refineLevel /* = 0 */)
 : HdTopology()
 , _topology(scheme,
             orientation,
             faceVertexCounts,
             faceVertexIndices)
 , _invisiblePoints()
 , _invisibleFaces()
 , _refineLevel(refineLevel)
 , _numPoints()
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->meshTopology);
    _numPoints = HdMeshTopology::ComputeNumPoints(
            _topology.GetFaceVertexIndices());
}

HdMeshTopology::HdMeshTopology(const TfToken &scheme,
                               const TfToken &orientation,
                               const VtIntArray &faceVertexCounts,
                               const VtIntArray &faceVertexIndices,
                               const VtIntArray &holeIndices,
                               int refineLevel /* = 0 */)
 : HdTopology()
 , _topology(scheme,
             orientation,
             faceVertexCounts,
             faceVertexIndices,
             holeIndices)
 , _invisiblePoints()
 , _invisibleFaces()
 , _refineLevel(refineLevel)
 , _numPoints()
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->meshTopology);
    _numPoints = HdMeshTopology::ComputeNumPoints(
            _topology.GetFaceVertexIndices());
}

HdMeshTopology::~HdMeshTopology()
{
    HD_PERF_COUNTER_DECR(HdPerfTokens->meshTopology);
}

HdMeshTopology &
HdMeshTopology::operator =(const HdMeshTopology &copy)
{
    HdTopology::operator =(copy);

    _topology        = copy.GetPxOsdMeshTopology();
    _geomSubsets     = copy._geomSubsets;
    _refineLevel     = copy._refineLevel;
    _numPoints       = copy._numPoints;
    _invisiblePoints = copy._invisiblePoints;
    _invisibleFaces  = copy._invisibleFaces;

    return *this;
}

bool
HdMeshTopology::IsEnabledAdaptive()
{
    return TfGetEnvSetting(HD_ENABLE_OPENSUBDIV3_ADAPTIVE) == 1;
}

bool
HdMeshTopology::operator==(HdMeshTopology const &other) const {

    HD_TRACE_FUNCTION();

    return (_topology == other._topology)
        && (_geomSubsets == other._geomSubsets)
        && (_invisiblePoints == other._invisiblePoints)
        && (_invisibleFaces == other._invisibleFaces)
        && (_refineLevel == other._refineLevel);
    // Don't compare _numPoints, since it is derived from _topology.
}

int
HdMeshTopology::GetNumFaces() const
{
    return (int)_topology.GetFaceVertexCounts().size();
}

int
HdMeshTopology::GetNumFaceVaryings() const
{
    return (int)_topology.GetFaceVertexIndices().size();
}

int
HdMeshTopology::GetNumPoints() const
{
    return _numPoints;
}

/*static*/ int
HdMeshTopology::ComputeNumPoints(VtIntArray const &verts)
{
    HD_TRACE_FUNCTION();

    // compute numPoints from topology indices
    int numIndices = verts.size();
    int numPoints = -1;
    int const * vertsPtr = verts.cdata();
    for (int i= 0;i <numIndices; ++i) {
        // find the max vertex index in face verts
        numPoints = std::max(numPoints, vertsPtr[i]);
    }
    // numPoints = max vertex index + 1
    return numPoints + 1;
}

HdTopology::ID
HdMeshTopology::ComputeHash() const
{
    HD_TRACE_FUNCTION();

    HdTopology::ID hash =_topology.ComputeHash();
    hash = ArchHash64((const char*)&_refineLevel, sizeof(_refineLevel), hash);
    for (const HdGeomSubset &subset: _geomSubsets) {
        hash = ArchHash64((const char*)&subset.type,
                          sizeof(subset.type), hash);
        hash = ArchHash64((const char*)&subset.id,
                          sizeof(subset.id), hash);
        hash = ArchHash64((const char*)&subset.materialId,
                          sizeof(subset.materialId), hash);
        hash = ArchHash64((const char*)subset.indices.cdata(),
                          sizeof(int)*subset.indices.size(), hash);
    }
    // Note: We don't hash topological visibility, because it is treated as a
    // per-mesh opinion, and hence, shouldn't break topology sharing.

    // Do not hash _numPoints since it is derived from _topology.
    return hash;
}

PXR_NAMESPACE_CLOSE_SCOPE

