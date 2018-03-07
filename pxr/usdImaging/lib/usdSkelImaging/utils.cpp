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
#include "pxr/usdImaging/usdSkelImaging/utils.h"

#include "pxr/base/work/loops.h"
#include "pxr/usd/usdSkel/topology.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/pxOsd/tokens.h"


PXR_NAMESPACE_OPEN_SCOPE


namespace {


/*
  Bones are constructed from child to parent as a pyramid-shaped
  object with square base at the parent and tip at the child.

  PERFORMANCE: This current implementation is sub-optimal in several ways:

  1. At scale (thousands of skels), it's more efficient to construct
     bones on the GPU. Eg., via a geometry shader, with lines as input.
     In addition to benefiting from additional parallelism, this
     could greatly reduce the amount of data sent to the GPU.

  2. Even though all faces are tris, we waste time and memory passing
     along a face vertex counts array. Hydra then must then spend
     extra time attempting to triangulate that data.
     It would be more efficient if HdMeshTopology had an additional
     flag to indicate that its data is pure-tris, removing the
     need for both re-triangulation as well as the construction of
     the face vertex counts array.
*/

const int _boneVerts[] = {0,2,1, 0,3,2, 0,4,3, 0,1,4};
const int _boneNumVerts = 12;
const int _boneNumVertsPerFace = 3;
const int _boneNumFaces = 4;
const int _boneNumPoints = 5;


size_t
_ComputeBoneCount(const UsdSkelTopology& topology)
{
    int numJoints = static_cast<int>(topology.GetNumJoints());

    size_t numBones = 0;
    for(size_t i = 0; i < topology.GetNumJoints(); ++i) {
        int parent = topology.GetParent(i);
        numBones += (parent >= 0 && parent < numJoints);
    }
    return numBones;
}


} // namespace


bool
UsdSkelImagingComputeBoneTopology(const UsdSkelTopology& skelTopology,
                                  HdMeshTopology* meshTopology,
                                  size_t* numPoints)
{
    if(!meshTopology) {
        TF_CODING_ERROR("'meshTopology' pointer is null.");
        return false;
    }
    if(!numPoints) {
        TF_CODING_ERROR("'numPoints' pointer is null.");
        return false;
    }

    size_t numBones = _ComputeBoneCount(skelTopology);
    
    VtIntArray faceVertexCounts;
    faceVertexCounts.assign(numBones*_boneNumFaces, _boneNumVertsPerFace);

    VtIntArray faceVertexIndices(numBones*_boneNumVerts);
    int* vertsData = faceVertexIndices.data();
    for(size_t i = 0; i < numBones; ++i) {
        for(int j = 0; j < _boneNumVerts; ++j) {
            vertsData[i*_boneNumVerts+j] = _boneVerts[j] + i*_boneNumPoints;
        }
    }

    *meshTopology = HdMeshTopology(PxOsdOpenSubdivTokens->none,
                                   HdTokens->rightHanded,
                                   faceVertexCounts,
                                   faceVertexIndices);

    *numPoints = numBones * _boneNumPoints;

    return true;
}


namespace {


/// Wrapper for parallel loops that execs in serial if \p count
/// is below a reasonable threading threshold.
template <typename Fn>
void
_ParallelForN(size_t count, Fn&& callback)
{
    // XXX: Profiling shows that most of our loops only benefit
    // from parallelism past this threshold.
    int threshold = 1000;

    if(count < threshold) {
        WorkSerialForN(count, callback);
    } else {
        WorkParallelForN(count, callback);
    }
}


/// Return the index of the basis of \p mx that is best aligned with \p dir.
/// This assumes that \p mx is orthogonal.
int
_FindBestAlignedBasis(const GfMatrix4d& mx, const GfVec3d& dir)
{
    const float PI_4 = static_cast<float>( M_PI ) / 4.0f;

    for(int i = 0; i < 2; ++i) {
        // If the transform is orthogonal, the best aligned 
        // basis has an absolute dot product > PI/4.
        if(std::abs(GfDot(mx.GetRow3(i),dir)) > PI_4)
            return i;
    }
    // Assume it's the last basis...
    return 2;
}


void
_ComputePointsForSingleBone(GfVec3f* points,
                            const GfMatrix4d& xform,
                            const GfMatrix4d& parentXform)
{
    GfVec3f end = GfVec3f(xform.ExtractTranslation());
    GfVec3f start = GfVec3f(parentXform.ExtractTranslation());

    // Need local basis vectors along which to displace the
    // base of the bone. Use whichever basis vectors of the
    // target xform are best aligned with the direction of the bone.
    GfVec3f boneDir = end - start;

    static const int iAxis[] = {1,0,0};
    static const int jAxis[] = {2,2,1};
    
    // XXX: This is pretty expensive at scale. Alternatives?
    int principleAxis =
        _FindBestAlignedBasis(parentXform, boneDir.GetNormalized());
    
    GfVec3f i = GfVec3f(
        parentXform.GetRow3(iAxis[principleAxis])).GetNormalized();
    GfVec3f j = GfVec3f(
        parentXform.GetRow3(jAxis[principleAxis])).GetNormalized();

    // Determine a size (thickness) of bones in proportion to their length.
    // TODO: Later, may be worth considering allowing a UsdSkelSkeleton
    // to provide per-bone size information.
    float size = boneDir.GetLength() * 0.1;
    i *= size;
    j *= size;

    points[0] = end;
    points[1] = start + i + j;
    points[2] = start + i - j;
    points[3] = start - i - j;
    points[4] = start - i + j;
}


} // namespace


bool
UsdSkelImagingComputeBonePoints(const UsdSkelTopology& topology,
                                const VtMatrix4dArray& jointSkelXforms,
                                size_t numPoints,
                                VtVec3fArray* points)
{
    if(!points) {
        TF_CODING_ERROR("'points' pointer is null.");
        return false;
    }

    if(jointSkelXforms.size() == topology.GetNumJoints()) {

        points->resize(numPoints);

        return UsdSkelImagingComputeBonePoints(
            topology, jointSkelXforms.cdata(), points->data(), numPoints);
    } else {
        TF_WARN("jointSkelXforms.size() [%zu] != number of joints [%zu].",
                jointSkelXforms.size(), topology.GetNumJoints());
        return false;
    }
}


bool
UsdSkelImagingComputeBonePoints(const UsdSkelTopology& topology,
                                const GfMatrix4d* jointSkelXforms,
                                GfVec3f* points,
                                size_t numPoints)
{
    if(numPoints > 0 && !points) {
        TF_CODING_ERROR("'points' pointer is null.");
        return false;
    }

    int numJoints = static_cast<int>(topology.GetNumJoints());

    // Compute bone indices per joint.
    std::vector<int> boneIndices(topology.GetNumJoints(), -1);
    int boneIndex = 0;
    size_t actualNumPoints = 0;
    for(size_t i = 0; i < topology.GetNumJoints(); ++i) {
        int parent = topology.GetParent(i);
        if(parent >= 0 && parent < numJoints) {
            boneIndices[i] = boneIndex++;
            actualNumPoints += _boneNumPoints;
        }
    }
    if(actualNumPoints != numPoints) {
        TF_WARN("number of points [%zu] does not match the size of "
                "the input point array [%zu].", actualNumPoints, numPoints);
        return false;
    }

    // XXX: This is threaded for the sake of vectorized models,
    // where a bones are being computed for many skels.
    // (This is a known bottleneck in some imaging scenarios).
    _ParallelForN(
        topology.GetNumJoints(),
        [&](size_t start, size_t end) {
            for(size_t i = start; i < end; ++i) {
                int boneIndex = boneIndices[i];
                if(boneIndex >= 0) {
                    size_t offset =
                        static_cast<size_t>(boneIndex)*_boneNumPoints;
                    
                    TF_DEV_AXIOM((offset+_boneNumPoints) <= numPoints);

                    int parent = topology.GetParent(i);
                    TF_DEV_AXIOM(parent >= 0 && parent < numJoints);

                    _ComputePointsForSingleBone(points + offset,
                                                jointSkelXforms[i],
                                                jointSkelXforms[parent]);
                }
            }
        });

    return true;
}


bool
UsdSkelImagingComputeBoneJointIndices(const UsdSkelTopology& topology,
                                      VtIntArray* jointIndices,
                                      size_t numPoints)
{
    if(!jointIndices) {
        TF_CODING_ERROR("'jointIndices' pointer is null.");
        return false;
    }

    jointIndices->resize(numPoints);
    return UsdSkelImagingComputeBoneJointIndices(
        topology, jointIndices->data(), numPoints);
}


bool
UsdSkelImagingComputeBoneJointIndices(const UsdSkelTopology& topology,
                                      int* jointIndices,
                                      size_t numPoints)
{
    if(numPoints > 0 && !jointIndices) {
        TF_CODING_ERROR("'jointIndices' pointer is null.");
        return false;
    }

    int numJoints = static_cast<int>(topology.GetNumJoints());

    size_t offset = 0;
    for(size_t i = 0; i < topology.GetNumJoints(); ++i) {
        int parent = topology.GetParent(i);
        if(parent >= 0 && parent < numJoints) {

            if((offset+_boneNumPoints) <= numPoints) {
                // Each bone is defined as a pyramid shaped object,
                // with the tip at a joint, and a square base at
                // the parent.

                // First point (tip) belongs to this joint.
                jointIndices[offset] = static_cast<int>(i);

                // The rest of the points (the base) belong to the parent.
                for(int j = 1; j < _boneNumPoints; ++j) {
                    jointIndices[offset + j] = parent;
                }

                offset += _boneNumPoints;
            } else {
                TF_WARN("Incorrect number of points for bone "
                        "mesh [%zu].", numPoints);
                return false;
            }
        }
    }
    return true;
}

                                 
PXR_NAMESPACE_CLOSE_SCOPE
