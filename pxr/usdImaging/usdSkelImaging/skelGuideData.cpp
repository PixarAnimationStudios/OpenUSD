//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdSkelImaging/skelGuideData.h"

#include "pxr/usdImaging/usdSkelImaging/skelData.h"
#include "pxr/usdImaging/usdSkelImaging/utils.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// From usdSkelImaging/utils.cpp

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

constexpr int _boneVerts[] = {0,2,1, 0,3,2, 0,4,3, 0,1,4};
constexpr int _boneNumVerts = std::size(_boneVerts);
constexpr int _boneNumVertsPerFace = 3;
constexpr int _boneNumFaces = 4;
constexpr int _boneNumPoints = 5;

static_assert(_boneNumVerts == _boneNumVertsPerFace * _boneNumFaces);

size_t
_NumBones(const UsdSkelImagingSkelData &skelData)
{
    const int numJoints(skelData.topology.size());

    size_t result = 0;

    for (size_t joint = 0; joint < skelData.topology.size(); ++joint) {
        const int parent = skelData.topology.GetParent(joint);
        if (parent < 0) {
            continue;
        }
        if (parent >= numJoints) {
            TF_CODING_ERROR("Bad index for parent joint");
            continue;
        }

        result++;
    }

    return result;
}

}

UsdSkelImagingSkelGuideData
UsdSkelImagingComputeSkelGuideData(
    const UsdSkelImagingSkelData &skelData)
{
    TRACE_FUNCTION();

    UsdSkelImagingSkelGuideData result;

    result.primPath = skelData.primPath;

    result.numJoints = skelData.topology.size();

    if (result.numJoints != skelData.bindTransforms.size()) {
        TF_WARN(
            "Number of bind transforms does not match number of joints for "
            "skeleton %s.", skelData.primPath.GetText());
        return result;
    }

    const int numJoints(skelData.topology.size());
    const size_t numBones = _NumBones(skelData);

    // Compute boneJointIndices and boneMeshPoints at the same time.
    //
    // From UsdSkelImagingComputeBonePoints and
    // UsdSkelImagingComputeBoneJointIndices.

    result.boneJointIndices.resize(_boneNumPoints * numBones);
    int * boneJointIndices = result.boneJointIndices.data();

    result.boneMeshPoints.resize(_boneNumPoints * numBones);
    GfVec3f * boneMeshPoints = result.boneMeshPoints.data();

    for (size_t joint = 0; joint < skelData.topology.size(); ++joint) {
        const int parent = skelData.topology.GetParent(joint);
        if (parent < 0) {
            continue;
        }
        if (parent >= numJoints) {
            // Coding error already raised above.
            continue;
        }

        for (int i = 0; i < _boneNumPoints; ++i) {
            // Tip (first point) is transformed by this joint.
            // Base (remaining points) is transformed by parent joint.
            *boneJointIndices = (i == 0) ? joint : parent;
            ++boneJointIndices;
        }

        UsdSkelImagingComputePointsForSingleBone(
            GfMatrix4d(skelData.bindTransforms[joint]),
            GfMatrix4d(skelData.bindTransforms[parent]),
            boneMeshPoints);
        boneMeshPoints += _boneNumPoints;
    }

    return result;
}

static
size_t
_NumBones(const UsdSkelImagingSkelGuideData &skelGuideData)
{
    return skelGuideData.boneMeshPoints.size() / _boneNumPoints;
}

static
VtIntArray
_ComputeSkelGuideFaceVertexCounts(const size_t numBones)
{
    return VtIntArray(_boneNumFaces * numBones, _boneNumVertsPerFace);
}

VtIntArray
UsdSkelImagingComputeSkelGuideFaceVertexCounts(
    const UsdSkelImagingSkelGuideData &skelGuideData)
{
    return _ComputeSkelGuideFaceVertexCounts(_NumBones(skelGuideData));
}

static
VtIntArray
_ComputeSkelGuideFaceVertexIndices(const size_t numBones)
{
    VtIntArray result(numBones * _boneNumVerts);
    int * data = result.data();

    // numBones copies of _boneVerts, each increased by _boneNumPoints.
    for (size_t i = 0; i < numBones; ++i) {
        for (int j = 0; j < _boneNumVerts; ++j) {
            *data = _boneVerts[j] + i * _boneNumPoints;
            ++data;
        }
    }

    return result;
}

VtIntArray
UsdSkelImagingComputeSkelGuideFaceVertexIndices(
    const UsdSkelImagingSkelGuideData &skelGuideData)
{
    return _ComputeSkelGuideFaceVertexIndices(_NumBones(skelGuideData));
}

VtVec3fArray
UsdSkelImagingComputeSkelGuidePoints(
    const UsdSkelImagingSkelGuideData &skelGuideData,
    const VtArray<GfMatrix4f> &skinningTransforms)
{
    TRACE_FUNCTION();

    const size_t numPoints = skelGuideData.boneMeshPoints.size();

    VtVec3fArray result;

    if (!TF_VERIFY(skelGuideData.boneJointIndices.size() == numPoints)) {
        return result;
    }

    if (skelGuideData.numJoints != skinningTransforms.size()) {
        TF_WARN(
            "Number of skinning transforms did not match number of joints for "
            "skeleton %s.", skelGuideData.primPath.GetText());
        return result;
    }

    // Point i is computed by applying skinningTransforms[boneJointIndices[i]]
    // to boneMeshPoints[i].

    result.resize(
        numPoints,
        [&skelGuideData, &skinningTransforms](
                    GfVec3f * const begin, GfVec3f * const end) {
            const GfVec3f * boneMeshPoints =
                skelGuideData.boneMeshPoints.data();
            const int * boneJointIndices =
                skelGuideData.boneJointIndices.data();
            const GfMatrix4f * const skinningTransformsData =
                skinningTransforms.data();
            for (GfVec3f * data = begin; data < end; ++data) {
                const GfMatrix4f &m = skinningTransformsData[*boneJointIndices];
                new (data) GfVec3f(m.TransformAffine(*boneMeshPoints));
                ++boneMeshPoints;
                ++boneJointIndices;
            }});
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
