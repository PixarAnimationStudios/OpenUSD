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
#include "pxr/usd/usdSkel/utils.h"

#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/range3f.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/work/loops.h"

#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/relationship.h"

#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/modelAPI.h"
#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/usd/usdSkel/cache.h"
#include "pxr/usd/usdSkel/debugCodes.h"
#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/skeletonQuery.h"
#include "pxr/usd/usdSkel/skinningQuery.h"
#include "pxr/usd/usdSkel/topology.h"

#include <atomic>


PXR_NAMESPACE_OPEN_SCOPE


namespace {

/// Wrapper for parallel loops that execs in serial if \p count
/// is below a reasonable threading threshold.
template <typename Fn>
void
_ParallelForN(size_t count, bool forceSerial, Fn&& callback)
{
    // XXX: Profiling shows that most of our loops only benefit
    // from parallelism past this threshold.
    const int threshold = 1000;

    if(count < threshold || forceSerial) {
        WorkSerialForN(count, callback);
    } else {
        WorkParallelForN(count, callback);
    }
}

} // namespace


bool
UsdSkelConcatJointTransforms(const UsdSkelTopology& topology,
                             const GfMatrix4d* jointLocalXforms,
                             GfMatrix4d* xforms,
                             const GfMatrix4d* rootXform)
{
    if(topology.GetNumJoints() > 0) {
        if(!jointLocalXforms) {
            TF_CODING_ERROR("'jointLocalXforms' pointer is null.");
            return false;
        }
        if(!xforms) {
            TF_CODING_ERROR("'xforms' pointer is null.");
            return false;
        }
    }

    for(size_t i = 0; i < topology.GetNumJoints(); ++i) {
        int parent = topology.GetParent(i);
        if(parent >= 0) {
            if(static_cast<size_t>(parent) < i) {
                xforms[i] = jointLocalXforms[i] * xforms[parent];
            } else {
                if(static_cast<size_t>(parent) == i) {
                    TF_WARN("Joint %zu has itself as its parent.", i);
                } else {
                    TF_WARN("Joint %zu has mis-ordered parent %d. Joints are "
                            "expected to be ordered with parent joints always "
                            "coming before children.", i, parent);
                }
                return false;
            }
        } else {
            // Root joint.
            xforms[i] = jointLocalXforms[i];
            if(rootXform) {
                xforms[i] *= (*rootXform);
            }
        }
    }
    return true;
}


bool
UsdSkelConcatJointTransforms(const UsdSkelTopology& topology,
                             const VtMatrix4dArray& jointLocalXforms,
                             VtMatrix4dArray* xforms,
                             const GfMatrix4d* rootXform)
{
    TRACE_FUNCTION();

    if(jointLocalXforms.size() == topology.GetNumJoints()) {
        xforms->resize(topology.GetNumJoints());
        return UsdSkelConcatJointTransforms(topology, jointLocalXforms.cdata(),
                                            xforms->data(), rootXform);
    } else {
        TF_WARN("jointLocalXforms.size() [%zu] != number of joints [%zu].",
                jointLocalXforms.size(), topology.GetNumJoints());
    }
    return false;
}


bool
UsdSkelComputeJointLocalTransforms(const UsdSkelTopology& topology,
                                   const GfMatrix4d* xforms,
                                   const GfMatrix4d* inverseXforms,
                                   GfMatrix4d* jointLocalXforms,
                                   const GfMatrix4d* rootInverseXform)
{
    if(topology.GetNumJoints() > 0) {
        if(!xforms) {
            TF_CODING_ERROR("'xforms' pointer is null.");
            return false;
        }
        if(!inverseXforms) {
            TF_CODING_ERROR("'inverseXforms' pointer is null.");
            return false;
        }
        if(!jointLocalXforms) {
            TF_CODING_ERROR("'jointLocalXforms' pointer is null.");
            return false;
        }
    }


    // Skel-space transforms are computed as:
    //     skelXform = jointLocalXform*parentSkelXform
    // So we want:
    //     jointLocalXform = skelXform*inv(parentSkelXform)

    for(size_t i = 0; i < topology.GetNumJoints(); ++i) {
        int parent = topology.GetParent(i);
        if(parent >= 0) {
            if(static_cast<size_t>(parent) < i) {
                jointLocalXforms[i] = xforms[i]*inverseXforms[parent];
            } else {
                if(static_cast<size_t>(parent) == i) {
                    TF_WARN("Joint %zu has itself as its parent.", i);
                    return false;
                }
                TF_WARN("Joint %zu has mis-ordered parent %d. Joints are "
                        "expected to be ordered with parent joints always "
                        "coming before children.", i, parent);
                return false;
            }
        } else {
            // Root joint.
            jointLocalXforms[i] = xforms[i];
            if(rootInverseXform) {
                jointLocalXforms[i] *= (*rootInverseXform);
            }
        }
    }
    return true;
}


bool
UsdSkelComputeJointLocalTransforms(const UsdSkelTopology& topology,
                                   const VtMatrix4dArray& xforms,
                                   const VtMatrix4dArray& inverseXforms,
                                   VtMatrix4dArray* jointLocalXforms,
                                   const GfMatrix4d* rootInverseXform)
{
    TRACE_FUNCTION();

    if(!jointLocalXforms) {
        TF_CODING_ERROR("'jointLocalXforms' pointer is null.");
        return false;
    }

    if(xforms.size() == topology.GetNumJoints()) {
        if(inverseXforms.size() == topology.GetNumJoints()) {
            jointLocalXforms->resize(xforms.size());
            return UsdSkelComputeJointLocalTransforms(
                topology, xforms.cdata(), inverseXforms.cdata(),
                jointLocalXforms->data(), rootInverseXform);
        } else {
            TF_WARN("inverseXforms.size() [%zu] != number of joints [%zu].",
                    inverseXforms.size(), topology.GetNumJoints());
        }
    } else {
        TF_WARN("xforms.size() [%zu] != number of joints [%zu].",
                xforms.size(), topology.GetNumJoints());
    }
    return false;
}


namespace {


bool
_DecomposeTransform(const GfMatrix4d& xform,
                    GfVec3f* translate,
                    GfRotation* rotate,
                    GfVec3h* scale)
{
    // XXX: GfMatrix4d::Factor() may crash if the value isn't properly aligned.
    TF_DEV_AXIOM(size_t(&xform)%alignof(GfMatrix4d) == 0);
    
    // Decomposition must account for handedness changes due to negative scales.
    // This is similar to GfMatrix4d::RemoveScaleShear().
    GfMatrix4d scaleOrient, factoredRot, perspMat;
    GfVec3d scaleD, translateD;
    if(xform.Factor(&scaleOrient, &scaleD,
                    &factoredRot, &translateD, &perspMat)) {

        if(factoredRot.Orthonormalize()) {
            *rotate = factoredRot.ExtractRotation();
            *scale = GfVec3h(scaleD);
            *translate = GfVec3f(translateD);
            return true;
        }
    }
    return false;
}


bool
_DecomposeTransform(const GfMatrix4d& xform,
                    GfVec3f* translate,
                    GfQuatf* rotate,
                    GfVec3h* scale)
{
    GfRotation r;
    if(_DecomposeTransform(xform, translate, &r, scale)) {
        *rotate = GfQuatf(r.GetQuat());
        // XXX: Note that even if GfRotation() produces a normal
        // quaternion, casting down to a lesser precision may
        // require us to re-normalize.
        rotate->Normalize();
        return true;
    }
    return false;
}


} // namespace


bool
UsdSkelDecomposeTransform(const GfMatrix4d& xform,
                          GfVec3f* translate,  
                          GfRotation* rotate,
                          GfVec3h* scale)
{
    if(!translate) {
        TF_CODING_ERROR("'translate' pointer is null.");
        return false;
    }
    if(!rotate) {
        TF_CODING_ERROR("'rotate' pointer is null.");
        return false;
    }
    if(!scale) {
        TF_CODING_ERROR("'scale' pointer is null.");
        return false;
    }
    return _DecomposeTransform(xform, translate, rotate, scale);
}


bool
UsdSkelDecomposeTransform(const GfMatrix4d& xform,
                          GfVec3f* translate,  
                          GfQuatf* rotate,
                          GfVec3h* scale)
{
    TRACE_FUNCTION();

    if(!translate) {
        TF_CODING_ERROR("'translate' pointer is null.");
        return false;
    }
    if(!rotate) {
        TF_CODING_ERROR("'rotate' pointer is null.");
        return false;
    }
    if(!scale) {
        TF_CODING_ERROR("'scale' pointer is null.");
        return false;
    }
    return _DecomposeTransform(xform, translate, rotate, scale);
}


bool
UsdSkelDecomposeTransforms(const GfMatrix4d* xforms,
                           GfVec3f* translations,
                           GfQuatf* rotations,
                           GfVec3h* scales,
                           size_t count)
{
    if(count > 0) {
        if(!xforms) {
            TF_CODING_ERROR("'xforms' pointer is null.");
            return false;
        }
        if(!translations) {
           TF_CODING_ERROR("'translations' pointer is null.");
           return false;
        }
        if(!rotations) {
           TF_CODING_ERROR("'rotations' pointer is null.");
           return false;
        }
        if(!scales) {
           TF_CODING_ERROR("'scales' pointer is null.");
           return false;
        }

        for(size_t i = 0; i < count; ++i) {
            if(!_DecomposeTransform(xforms[i], translations+i,
                                    rotations+i, scales+i)) {
                // XXX: Should this be a coding error, or a warning?
                TF_WARN("Failed decomposing transform %zu. "
                        "The source transform may be singular.", i);
                return false;
            }
        }
    }
    return true;
}


bool
UsdSkelDecomposeTransforms(const VtMatrix4dArray& xforms,
                           VtVec3fArray* translations,
                           VtQuatfArray* rotations,
                           VtVec3hArray* scales)
{
    TRACE_FUNCTION();

    if(!translations) {
        TF_CODING_ERROR("'translations' pointer is null.");
        return false;
    }
    if(!rotations) {
        TF_CODING_ERROR("'rotations' pointer is null.");
        return false;
    }
    if(!scales) {
        TF_CODING_ERROR("'scales' pointer is null.");
        return false;
    }

    translations->resize(xforms.size());
    rotations->resize(xforms.size());
    scales->resize(xforms.size());

    return UsdSkelDecomposeTransforms(xforms.cdata(),
                                      translations->data(),
                                      rotations->data(), scales->data(),
                                      xforms.size());
}


GfMatrix4d
UsdSkelMakeTransform(const GfVec3f& translate,
                     const GfMatrix3f& rotate,
                     const GfVec3h& scale)
{
    // Order is scale*rotate*translate
    return GfMatrix4d(rotate[0][0]*scale[0],
                      rotate[0][1]*scale[0],
                      rotate[0][2]*scale[0], 0,

                      rotate[1][0]*scale[1],
                      rotate[1][1]*scale[1],
                      rotate[1][2]*scale[1], 0,

                      rotate[2][0]*scale[2],
                      rotate[2][1]*scale[2],
                      rotate[2][2]*scale[2], 0,

                      translate[0], translate[1], translate[2], 1);
}


GfMatrix4d
UsdSkelMakeTransform(const GfVec3f& translate,
                     const GfQuatf& rotate,
                     const GfVec3h& scale)
{
    return UsdSkelMakeTransform(translate, GfMatrix3f(rotate), scale);
}


bool
UsdSkelMakeTransforms(const GfVec3f* translations,
                      const GfQuatf* rotations,
                      const GfVec3h* scales,
                      GfMatrix4d* xforms,
                      size_t count)
{
    if(count > 0) {
        if(!xforms) {
            TF_CODING_ERROR("'xforms' pointer is null.");
            return false;
        }
        if(!translations) {
           TF_CODING_ERROR("'translations' pointer is null.");
           return false;
        }
        if(!rotations) {
           TF_CODING_ERROR("'rotations' pointer is null.");
           return false;
        }
        if(!scales) {
           TF_CODING_ERROR("'scales' pointer is null.");
           return false;
        }

        for(size_t i = 0; i < count; ++i) {
            xforms[i] = UsdSkelMakeTransform(
                translations[i], rotations[i], scales[i]);
        }
    }
    return true;
}


bool
UsdSkelMakeTransforms(const VtVec3fArray& translations,
                      const VtQuatfArray& rotations,
                      const VtVec3hArray& scales,
                      VtMatrix4dArray* xforms)
{
    TRACE_FUNCTION();

    if(!xforms) {
        TF_CODING_ERROR("'xforms' pointer is null.");
        return false;
    }

    if(translations.size() == rotations.size()) {
        if(translations.size() == scales.size()) {

            xforms->resize(translations.size());
            
            return UsdSkelMakeTransforms(
                translations.cdata(), rotations.cdata(), scales.cdata(),
                xforms->data(), xforms->size());
        } else {
            TF_WARN("Size of translations [%zu] != size of scales [%zu].",
                    translations.size(), scales.size()); 
       }
    } else {
            TF_WARN("Size of translations [%zu] != size of rotations [%zu].",
                    translations.size(), rotations.size());
    }
    return false;
}


bool
UsdSkelComputeJointsExtent(const GfMatrix4d* xforms,
                           size_t count,
                           VtVec3fArray* extent,
                           float pad,
                           const GfMatrix4d* rootXform)
{
    if(!extent) {
        TF_CODING_ERROR("'extent' pointer is null.");
        return false;
    }

    if(count > 0 && !xforms) {
        TF_CODING_ERROR("'xforms' pointer is null.");
        return false;
    }

    GfRange3f range;
    if(count > 0) {
        for(size_t i = 0; i < count; ++i) {
            GfVec3f pivot(xforms[i].ExtractTranslation());
            range.UnionWith(rootXform ?
                            rootXform->TransformAffine(pivot) : pivot);
        }

        const GfVec3f padVec(pad);
        range.SetMin(range.GetMin()-padVec);
        range.SetMax(range.GetMax()+padVec);
    }


    extent->resize(2);
    (*extent)[0] = range.GetMin();
    (*extent)[1] = range.GetMax();
    return true;
}


bool
UsdSkelComputeJointsExtent(const VtMatrix4dArray& xforms,
                           VtVec3fArray* extent,
                           float pad,
                           const GfMatrix4d* rootXform)
{
    TRACE_FUNCTION();

    return UsdSkelComputeJointsExtent(xforms.cdata(), xforms.size(),
                                      extent, pad, rootXform);
}


namespace {

/// Validate the size of a weight/index array for a given
/// number of influences per component.
/// Throws a warning for failed validation.
bool
_ValidateArrayShape(size_t size, int numInfluencesPerComponent)
{
    if(numInfluencesPerComponent > 0) {
        if(size%numInfluencesPerComponent == 0) {
            return true;
        } else {
            TF_WARN("Unexpected array size [%zu]: Size must be a multiple of "
                    "the number of influences per component [%d].",
                    size, numInfluencesPerComponent);
        }
    } else {
        TF_WARN("Invalid number of influences per component (%d): "
                "number of influences must be greater than zero.",
                numInfluencesPerComponent);
    }
    return false;
}


bool
_NormalizeWeights(float* weights, size_t numWeights,
                  int numInfluencesPerComponent)
{
    size_t numComponents = numWeights/numInfluencesPerComponent;

    _ParallelForN(
        numComponents,
        /* forceSerial = */ false,
        [&](size_t start, size_t end)
        {
            for(size_t i = 0; i < end; ++i) {
                float* weightSet = weights + i*numInfluencesPerComponent;

                float sum = 0.0f;
                for(int j = 0; j < numInfluencesPerComponent; ++j) {
                    sum += weightSet[j];
                }

                if(std::abs(sum) > std::numeric_limits<float>::epsilon()) {
                    for(int j = 0; j < numInfluencesPerComponent; ++j) {
                        weightSet[j] /= sum;
                    }
                } else {
                    for(int j = 0; j < numInfluencesPerComponent; ++j) {
                        weightSet[j] = 0.0f;
                    }
                }
            }
        });

    return true;
}


} // namespace


bool
UsdSkelNormalizeWeights(VtFloatArray* weights,
                        int numInfluencesPerComponent)
{
    TRACE_FUNCTION();

    if(!weights) {
        TF_CODING_ERROR("'weights' pointer is null.");
        return false;
    }

    if(!_ValidateArrayShape(weights->size(), numInfluencesPerComponent))
        return false;

    return _NormalizeWeights(weights->data(), weights->size(),
                             numInfluencesPerComponent);
}


namespace {

bool
_SortInfluences(int* indices, float* weights,
                size_t numInfluences,
                int numInfluencesPerComponent)
{
    if(numInfluencesPerComponent < 2) {
        // Nothing to do.
        return true;
    }

    size_t numComponents = numInfluences/numInfluencesPerComponent;

    _ParallelForN(
        numComponents,
        /* forceSerial = */ false,
        [&](size_t start, size_t end)
        {
            std::vector<std::pair<float,int> > influences;
            for(size_t i = start; i < end; ++i) {
                float* weightsSet = weights + i*numInfluencesPerComponent;
                int *indexSet = indices + i*numInfluencesPerComponent;

                influences.resize(numInfluencesPerComponent);
                for(int j = 0; j < numInfluencesPerComponent; ++j) {
                    influences[j] = std::make_pair(weightsSet[j], indexSet[j]);
                }
                std::sort(influences.begin(), influences.end(),
                          std::greater<std::pair<float,int> >());
                for(int j = 0; j < numInfluencesPerComponent; ++j) {
                    const auto& pair = influences[j];
                    weightsSet[j] = pair.first;
                    indexSet[j] = pair.second;
                }
            }
        });

    return true;
}

} // namespace


bool
UsdSkelSortInfluences(VtIntArray* indices,
                      VtFloatArray* weights,
                      int numInfluencesPerComponent)
{
    TRACE_FUNCTION();

    if(!indices) {
        TF_CODING_ERROR("'indices' pointer is null.");
        return false;
    }
    if(!weights) {
        TF_CODING_ERROR("'weights' pointer is null.");
        return false;
    }

    if(indices->size() != weights->size()) {
        TF_WARN("Size of 'indices' [%zu] != size of 'weights' [%zu].",
                indices->size(), weights->size());
        return false;
    }

    if(!_ValidateArrayShape(weights->size(), numInfluencesPerComponent)) {
        return false;
    }

    return _SortInfluences(indices->data(), weights->data(),
                           indices->size(), numInfluencesPerComponent);
}


namespace {

template <typename T>
bool
_ExpandConstantArray(T* array, size_t size)
{
    if(!array) {
        TF_CODING_ERROR("'array' pointer is null.");
        return false;
    }

    if(size == 0) {
        array->clear();
    } else {
        size_t numInfluencesPerComponent = array->size();
        array->resize(numInfluencesPerComponent*size);

        auto* data = array->data();
        for(size_t i = 1; i < size; ++i) {
            std::copy(data, data + numInfluencesPerComponent,
                      data + i*numInfluencesPerComponent);
        }
    }
    return true;
}

} // namespace


bool
UsdSkelExpandConstantInfluencesToVarying(VtIntArray* indices, size_t size)
{
    return _ExpandConstantArray(indices, size);
}


bool
UsdSkelExpandConstantInfluencesToVarying(VtFloatArray* weights, size_t size)
{
    return _ExpandConstantArray(weights, size);
}


namespace {

template <typename T>
bool
_ResizeInfluences(VtArray<T>* array, int srcNumInfluencesPerComponent,
                  int newNumInfluencesPerComponent, T defaultVal)
{
    if(srcNumInfluencesPerComponent == newNumInfluencesPerComponent)
        return true;

    if(!array) {
        TF_CODING_ERROR("'array' pointer is null.");
        return false;
    }

    if(!_ValidateArrayShape(array->size(), srcNumInfluencesPerComponent))
        return false;

    size_t numComponents = array->size()/srcNumInfluencesPerComponent;
    if(numComponents == 0)
        return true;

    if(newNumInfluencesPerComponent < srcNumInfluencesPerComponent) {
        // Truncate influences in-place.
        auto* data = array->data();
        for(size_t i = 1; i < numComponents; ++i) {
            size_t srcStart = i*srcNumInfluencesPerComponent;
            size_t srcEnd = srcStart + newNumInfluencesPerComponent;
            size_t dstStart = i*newNumInfluencesPerComponent;

            TF_DEV_AXIOM(srcEnd <= array->size());
            TF_DEV_AXIOM((dstStart + (srcEnd-srcStart)) <= array->size());
            std::copy(data + srcStart, data + srcEnd, data + dstStart);
        }
        array->resize(numComponents*newNumInfluencesPerComponent);
    } else {
        // Expand influences in-place.
        // This is possible IFF all elements are copied in *reverse order*
        array->resize(numComponents*newNumInfluencesPerComponent);

        auto* data = array->data();
        for(size_t i = 0; i < numComponents; ++i) { 
            // Reverse the order.
            size_t idx = numComponents-i-1;

            // Copy source values (*reverse order*)
            for(int j = (srcNumInfluencesPerComponent-1); j >= 0; --j) {
                TF_DEV_AXIOM(
                    (idx*newNumInfluencesPerComponent + j) < array->size());

                data[idx*newNumInfluencesPerComponent + j] =
                    data[idx*srcNumInfluencesPerComponent + j];
            }
            // Initialize values not filled by copying from src.
            TF_DEV_AXIOM((idx+1)*newNumInfluencesPerComponent <= array->size());
            std::fill(data + idx*newNumInfluencesPerComponent +
                      srcNumInfluencesPerComponent,
                      data + (idx+1)*newNumInfluencesPerComponent, defaultVal);

        }
    }
    return true;
}

} // namespace


bool
UsdSkelResizeInfluences(VtIntArray* indices,
                        int srcNumInfluencesPerComponent,
                        int newNumInfluencesPerComponent)
{
    TRACE_FUNCTION();
    return _ResizeInfluences(indices, srcNumInfluencesPerComponent,
                             newNumInfluencesPerComponent, 0);
}


bool
UsdSkelResizeInfluences(VtFloatArray* weights,
                        int srcNumInfluencesPerComponent,
                        int newNumInfluencesPerComponent)
{
    TRACE_FUNCTION();

    if(_ResizeInfluences(weights, srcNumInfluencesPerComponent,
                         newNumInfluencesPerComponent, 0.0f)) {
        if(newNumInfluencesPerComponent < srcNumInfluencesPerComponent) {
            // Some weights have been stripped off. Need to renormalize.
            return UsdSkelNormalizeWeights(
                weights, newNumInfluencesPerComponent);
        }
        return true;
    }
    return false;
}


bool
UsdSkelSkinPointsLBS(const GfMatrix4d& geomBindTransform,
               const GfMatrix4d* jointXforms,
               size_t numJoints,
               const int* jointIndices,
               const float* jointWeights,
               size_t numInfluences,
               int numInfluencesPerPoint,
               GfVec3f* points,
               size_t numPoints,
               bool forceSerial)
{
    TRACE_FUNCTION();
    
    if(numInfluences != (numPoints*numInfluencesPerPoint)) {
        TF_WARN("numInfluences [%zu] != "
                "(numPoints [%zu] * numInfluencesPerPoint [%d]).",
                numInfluences, numPoints, numInfluencesPerPoint);
        return false;
    }

    // Flag for marking error state from within threads.
    std::atomic_bool errors(false);

    _ParallelForN(
        numPoints,
        /* forceSerial = */ forceSerial,
        [&](size_t start, size_t end)
        {
            for(size_t pi = start; pi < end; ++pi) {

                GfVec3f initialP = geomBindTransform.Transform(points[pi]);
                GfVec3f p(0,0,0);
                
                for(int wi = 0; wi < numInfluencesPerPoint; ++wi) {
                    size_t influenceIdx = pi*numInfluencesPerPoint + wi;
                    TF_DEV_AXIOM(influenceIdx < numInfluences);

                    int jointIdx = jointIndices[influenceIdx];

                    if(jointIdx >= 0 &&
                       static_cast<size_t>(jointIdx) < numJoints) {

                        float w = jointWeights[influenceIdx];
                        if(w != 0.0f) {

                            // Since joint transforms are encoded in terms of
                            //t,r,s components, it shouldn't be possible to
                            // encode non-affine transforms, except for the rest
                            // pose (which, according to the schema, should
                            // be affine!). Safe to assume affine transforms.
                            p += jointXforms[jointIdx].TransformAffine(
                                initialP)*w;
                        }

                        // XXX: Possible optimization at this point:
                        // If joint weights were required to be ordered, and
                        // null weights are encountered, we can break out of
                        // the inner loop early. I.e.,
                        //
                        // if(weightIsNull)
                        //     break;
                        //
                        // This can potentially greatly reduce the number of
                        // operations when the number of influences is high, but
                        // most points have few influences.
                        // This optimization is not being applied now because
                        // the schema does not (yet) require sorted influences.
                    } else {

                        // XXX: Generally, if one joint index is bad, an asset
                        // has probably gotten out of sync, and probably many
                        // other indices will be invalid, too.
                        // We could attempt to continue silently, but would
                        // likely end up with scrambled points.
                        // Bail out early.

                        TF_WARN("Out of range joint index %d at index %zu"
                                " (num joints = %zu).",
                                jointIdx, influenceIdx, numJoints);
                        errors = true;
                        return;
                    }
                }

                points[pi] = p;
            }
        }
        );

    return !errors;
}


bool
UsdSkelSkinPointsLBS(const GfMatrix4d& geomBindTransform,
                     const VtMatrix4dArray& jointXforms,
                     const VtIntArray& jointIndices,
                     const VtFloatArray& jointWeights,
                     int numInfluencesPerPoint,
                     VtVec3fArray* points)
{
    if(!points) {
        TF_CODING_ERROR("'points' pointer is null.");
        return false;
    }

    if(jointIndices.size() == jointWeights.size()) {
        return UsdSkelSkinPointsLBS(geomBindTransform,
                                    jointXforms.cdata(),
                                    jointXforms.size(),
                                    jointIndices.cdata(),
                                    jointWeights.cdata(),
                                    jointIndices.size(),
                                    numInfluencesPerPoint,
                                    points->data(), points->size());
    } else {
        TF_WARN("jointIndices.size() [%zu]! = jointWeights.size() [%zu].",
                jointIndices.size(), jointWeights.size());
    }
    return false;
}


bool
UsdSkelSkinTransformLBS(const GfMatrix4d& geomBindTransform,
                        const GfMatrix4d* jointXforms,
                        size_t numJoints,
                        const int* jointIndices,
                        const float* jointWeights,
                        size_t numInfluences,
                        GfMatrix4d* xform)
{
    TRACE_FUNCTION();
    //
    // Early-out for the common case where an object is rigidly
    // bound to a single joint.
    if(numInfluences == 1 && GfIsClose(jointWeights[0], 1.0f, 1e-6)) {
        int jointIdx = jointIndices[0];
        if(jointIdx >= 0 && static_cast<size_t>(jointIdx) < numJoints) {
            *xform = geomBindTransform*jointXforms[jointIdx];
            return true;
        } else {
            TF_WARN("Out of range joint index %d at index 0"
                    " (num joints = %zu).", jointIdx, numJoints);
            return false;
        }
    }

    // One option for skinning transforms would be to decompose the transforms
    // into translate,rotate,scale components, and compute the weighted
    // combination of those components.
    // The transformation decomposition that this requires, however, 
    // is relatively expensive.
    // What we do instead is compute a 4-point frame to describe the transform,
    // apply normal point deformations, and then derive a skinned transform
    // from the deformed frame points.

    // Scaling factor for how far to offset each basis vector.
    // This should be a small number, but not so small that
    // that the inversion (end of this function) has too much float error.
    float size = 1e-3;

    GfVec3f pivot(geomBindTransform.ExtractTranslation());

    GfVec3f framePoints[4] = {
        pivot + GfVec3f(geomBindTransform.GetRow3(0))*size, // i basis
        pivot + GfVec3f(geomBindTransform.GetRow3(1))*size, // j basis
        pivot + GfVec3f(geomBindTransform.GetRow3(2))*size, // k basis
        pivot, // translate
    };

    for(int pi = 0; pi < 4; ++pi) {
        GfVec3f initialP = framePoints[pi];

        GfVec3f p(0,0,0);
        for(size_t wi = 0; wi < numInfluences; ++wi) {
            int jointIdx = jointIndices[wi];
            if(jointIdx >= 0 && static_cast<size_t>(jointIdx) < numJoints) {
                float w = jointWeights[wi];
                if(w != 0.0f) {
                    // XXX: See the notes from _SkinPointsLBS():
                    // affine transforms should be okay.
                    p += jointXforms[jointIdx].TransformAffine(
                        initialP)*w;
                }
            } else {
                TF_WARN("Out of range joint index %d at index %zu"
                        " (num joints = %zu).",
                        jointIdx, wi, numJoints);
                return false;
            }
        }
        framePoints[pi] = p;
    }

    GfVec3f skinnedPivot = framePoints[3];
    xform->SetTranslate(skinnedPivot);
    float sizeInv = 1/size;
    for(int i = 0; i < 3; ++i) {
        xform->SetRow3(i, (framePoints[i]-skinnedPivot)*sizeInv);
    }
    return true;
}


bool
UsdSkelSkinTransformLBS(const GfMatrix4d& geomBindTransform,
                        const VtMatrix4dArray& jointXforms,
                        const VtIntArray& jointIndices,
                        const VtFloatArray& jointWeights,
                        GfMatrix4d* xform)
{
    if(!xform) {
        TF_CODING_ERROR("'xform' pointer is null.");
        return false;
    }

    if(jointIndices.size() == jointWeights.size()) {
        return UsdSkelSkinTransformLBS(geomBindTransform, jointXforms.cdata(),
                                      jointXforms.size(), jointIndices.cdata(),
                                      jointWeights.cdata(), jointIndices.size(),
                                      xform);
    } else {
        TF_WARN("jointIndices.size() [%zu]! = jointWeights.size() [%zu].",
                jointIndices.size(), jointWeights.size());
    }
    return false;
}


namespace {


/// XXX: Copied from Usd_MergeTimeSamples()
/// Replace with a public method when and if one becomes available.
void
_MergeTimeSamples(std::vector<double> * const timeSamples, 
                  const std::vector<double> &additionalTimeSamples,
                  std::vector<double> * tempUnionTimeSamples)
{
    std::vector<double> temp; 
    if (!tempUnionTimeSamples)
        tempUnionTimeSamples = &temp;

    tempUnionTimeSamples->resize(timeSamples->size() + additionalTimeSamples.size());

    const auto &it = std::set_union(timeSamples->begin(), timeSamples->end(), 
                              additionalTimeSamples.begin(), 
                              additionalTimeSamples.end(), 
                              tempUnionTimeSamples->begin());
    tempUnionTimeSamples->resize(std::distance(tempUnionTimeSamples->begin(), it));
    timeSamples->swap(*tempUnionTimeSamples);
}


/// Get the time smaples that affect the local-to-world transform of \p prim.
bool
_GetWorldTransformTimeSamples(const UsdPrim& prim,
                              const GfInterval& interval,
                              std::vector<double>* times)
{
    // TODO: Use Usd_MergeTimeSamples, if it becomes public.

    std::vector<double> tmpTimes;
    std::vector<double> xformTimeSamples;
    for(UsdPrim p = prim; p; p = p.GetParent()) {
        if(p.IsA<UsdGeomXformable>()) {
            UsdGeomXformable::XformQuery(
                UsdGeomXformable(p)).GetTimeSamplesInInterval(
                    interval, &xformTimeSamples);
            _MergeTimeSamples(times, xformTimeSamples, &tmpTimes);
        }
    }
    return true;
}


/// Get the time samples for skinning a primitive.
void
_GetSkinningTimeSamples(const UsdPrim& prim,
                        const UsdSkelSkeletonQuery& skelQuery,
                        const UsdSkelSkinningQuery& skinningQuery,
                        const GfInterval& interval,
                        std::vector<double>* times)
{
    std::vector<double> tmpTimes;
    std::vector<double> propertyTimes;

    // Start off with time samples that affect joint tranforms.
    if(UsdSkelAnimQuery animQuery = skelQuery.GetAnimQuery()) {
        if(animQuery.GetJointTransformTimeSamplesInInterval(
               interval, &propertyTimes)) {
            _MergeTimeSamples(times, propertyTimes, &tmpTimes);
        }
    }

    // Include time samples that affect the local-to-world transform
    // (necessary because world space transforms are used to push
    //  defomrations in skeleton-space back into normal prim space.
    //  See the notes in the deformation methods for more on why.
    if(_GetWorldTransformTimeSamples(prim, interval, &propertyTimes)) {
        _MergeTimeSamples(times, propertyTimes, &tmpTimes);
    }

    if(!skinningQuery.IsRigidlyDeformed() && prim.IsA<UsdGeomPointBased>()) {
        if(UsdGeomPointBased(prim).GetPointsAttr().GetTimeSamplesInInterval(
               interval, &propertyTimes)) {
            _MergeTimeSamples(times, propertyTimes, &tmpTimes);
        }
    }
}


bool
_BakeSkinnedPoints(const UsdPrim& prim,
                   const UsdSkelSkeletonQuery& skelQuery,
                   const UsdSkelSkinningQuery& skinningQuery,
                   const std::vector<UsdTimeCode>& times,
                   UsdGeomXformCache* xfCache)
{
    UsdGeomPointBased pointBased(prim);
    if(!pointBased) {
        TF_CODING_ERROR("%s -- Attempted varying deformation of a non "
                        "point-based prim. Skinning currently only understands "
                        "varying deformations on UsdGeomPointBased types.",
                        prim.GetPath().GetText());
        return false;
    }

    UsdAttribute pointsAttr = pointBased.GetPointsAttr();

    // Pre-sample all point values.    
    std::vector<VtValue> pointsValues(times.size());
    for(size_t i = 0; i < times.size(); ++i) {
        if(!pointsAttr.Get(&pointsValues[i], times[i])) {
            return false;
        }
    }

    UsdAttribute extentAttr = pointBased.GetExtentAttr();

    for(size_t i = 0; i < times.size(); ++i) {
        const VtValue& points = pointsValues[i];
        if(!points.IsHolding<VtVec3fArray>()) {
            // Could have been a blocked sample. Skip it.
            continue;
        }

        UsdTimeCode time = times[i];

        TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
            "[UsdSkelBakeSkinning]   Skinning points at time %s "
            "(sample %zu of %zu)\n",
            TfStringify(time).c_str(), i, times.size());

        // XXX: More complete and sophisticated skinning code would compute
        // xforms once for all prims deformed by a single skeleton, instead
        // of recomputing skinning transforms for each deformed prim.
        // However, since this method is intended only for testing, simplicity
        // and correctness are greater priorities than performance.

        VtMatrix4dArray xforms;
        if(!skelQuery.ComputeSkinningTransforms(&xforms, time)) {
            TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                "[UsdSkelBakeSkinning]   Failed computing skinning transforms\n");
            return false;
        }

        VtVec3fArray skinnedPoints(points.UncheckedGet<VtVec3fArray>());
        if(skinningQuery.ComputeSkinnedPoints(xforms, &skinnedPoints, time)) {

            // Skinning deforms points in *skel* space.
            // A world-space point is then computed as:
            //
            //    worldSkinnedPoint = skelSkinnedPoint * skelLocalToWorld
            //
            // Since we're baking points into a gprim, we must transform these
            // from skel space into gprim space, such that:
            //
            //    localSkinnedPoint * gprimLocalToWorld = worldSkinnedPoint
            //  
            // So the points we store must be transformed as:
            //
            //    localSkinnedPoint = skelSkinnedPoint *
            //       skelLocalToWorld * inv(gprimLocalToWorld)

            xfCache->SetTime(time);
            GfMatrix4d gprimLocalToWorld =
                xfCache->GetLocalToWorldTransform(prim);

            GfMatrix4d skelLocalToWorld;
            if(!skelQuery.ComputeLocalToWorldTransform(
                   &skelLocalToWorld, xfCache)) {
                TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                    "[UsdSkelBakeSkinning]   Failed computing "
                    "skel local-to-world transform\n");
                return false;
            }

            GfMatrix4d skelToGprimXf =
                skelLocalToWorld*gprimLocalToWorld.GetInverse();

            for(auto& pt : skinnedPoints) {
                pt = skelToGprimXf.Transform(pt);
            }
            pointsAttr.Set(skinnedPoints, time);

            // Update point extent.
            VtVec3fArray extent;
            if(UsdGeomBoundable::ComputeExtentFromPlugins(
                   pointBased, time, &extent)) {
                extentAttr.Set(extent, time);
            }
        } else {
            TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                "[UsdSkelBakeSkinning]   Failed skinning points\n");
            return false;
        }
    }
    return true;
}


bool
_BakeSkinnedTransform(const UsdPrim& prim,
                      const UsdSkelSkeletonQuery& skelQuery,
                      const UsdSkelSkinningQuery& skinningQuery,
                      const std::vector<UsdTimeCode>& times,
                      UsdGeomXformCache* xfCache)
{
    UsdGeomXformable xformable(prim);
    if(!xformable) {
        TF_CODING_ERROR("%s -- Attempted rigid deformation of a non-xformable. "
                        "Skinning currently only understands rigid deformations "
                        "on UsdGeomXformable types.",
                        prim.GetPrim().GetPath().GetText());
        return false;
    }

    UsdAttribute xformAttr = xformable.MakeMatrixXform();

    for(size_t i = 0; i < times.size(); ++i) {
        UsdTimeCode time = times[i];

        TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
            "[UsdSkelBakeSkinning]   Skinning transform at time %s "
            "(sample %zu of %zu)\n",
            TfStringify(time).c_str(), i, times.size());

        // XXX: More complete and sophisticated skinning code would compute
        // xforms once for all prims deformed by a single skeleton, instead
        // of recomputing skinning transforms for each deformed prim.
        // However, since this method is intended only for testing, simplicity
        // and correctness are greater priorities than performance.

        VtMatrix4dArray xforms;
        if(!skelQuery.ComputeSkinningTransforms(&xforms, time)) {
            TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                "[UsdSkelBakeSkinning]   Failed computing "
                "skinning transforms\n");
            return false;
        }

        GfMatrix4d skinnedXform;
        if(skinningQuery.ComputeSkinnedTransform(xforms, &skinnedXform, time)) {

            // Skinning a transform produces a new transform in *skel* space.
            // A world-space transform is then computed as:
            //
            //    worldSkinnedXform = skelSkinnedXform * skelLocalToWorld
            //
            // Since we're baking transforms into a prim, we must transform
            // from skel space into the space of that prim's parent, such that:
            //
            //    newLocalXform * parentToWorld = worldSkinnedXform
            //
            // So the skinned, local transform becomes:
            //
            //    newLocalXform = skelSkinnedXform *
            //        skelLocalToWorld * inv(parentToWorld)

            xfCache->SetTime(time);

            GfMatrix4d skelLocalToWorld;
            if(!skelQuery.ComputeLocalToWorldTransform(
                   &skelLocalToWorld, xfCache)) {
                TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                    "[UsdSkelBakeSkinning]   Failed computing "
                    "skel local-to-world transform\n");
                return false;
            }

            GfMatrix4d newLocalXform;
            
            if(xfCache->GetResetXformStack(prim) ||
               prim.GetPath().IsRootPrimPath()) {

                // No parent transform to account for.
                newLocalXform = skinnedXform * skelLocalToWorld;
            } else {
                GfMatrix4d parentToWorld =
                    xfCache->GetParentToWorldTransform(prim);
                newLocalXform = skinnedXform * skelLocalToWorld *
                    parentToWorld.GetInverse();
            }

            xformAttr.Set(newLocalXform, time);
        } else {
            TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                "[UsdSkelBakeSkinning]   Failed skinning transform\n");
            return false;
        }
    }
    return true;
}


/// Update any model extents hints at or beneath \p prim,
/// at times \p times, for any prims that already store hints.
void
_UpdateExtentsHints(const UsdPrim& prim,
                    const std::vector<UsdTimeCode>& times)
{
    std::vector<UsdGeomModelAPI> modelsToUpdate;
    for(const auto& p : UsdPrimRange(prim)) {
        if(p.IsModel()) {
            UsdGeomModelAPI model(p);
            if(auto attr = model.GetExtentsHintAttr()) {
                modelsToUpdate.push_back(model);
                // Clear any existing time samples, incase they
                // includes samples that differ from our sampling times.
                attr.Clear();
            }
        }
    }

    TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
        "[UsdSkelBakeSkinning] Update model extents hints for %zu models.\n",
        modelsToUpdate.size());

    if(modelsToUpdate.size() > 0) {
        UsdGeomBBoxCache cache(UsdTimeCode(0),
                               UsdGeomImageable::GetOrderedPurposeTokens(),
                               /*useExtentsHint*/ false);

        for(UsdTimeCode time : times) {
            cache.SetTime(time);
            for(auto& model : modelsToUpdate) {
                model.SetExtentsHint(model.ComputeExtentsHint(cache), time);
            }
        }
    }
}


} // namespace


bool
UsdSkelBakeSkinningLBS(const UsdSkelRoot& root, const GfInterval& interval)
{
    // Keep in mind that this method is intended for testing and validation.
    // Because of this, we do not try to be robust in the face of errors
    // Any errors means we bail!

    TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
        "[UsdSkelBakeSkinning] Baking skinning for <%s>\n",
        root.GetPrim().GetPath().GetText());

    // XXX: Since this method is primarily about validating correctness,
    // we construct any necessary cache data internally.
    // Normal consumers of skel data should instead hold a persistent
    // cache that is shared by all prims.
    UsdSkelCache skelCache;
    if(!skelCache.Populate(root))
        return false;

    UsdGeomXformCache xfCache;

    // Track the union of time codes samples across all prims.
    std::vector<double> allPrimTimes;
    std::vector<double> tmpTimes;

    auto range = UsdPrimRange(root.GetPrim());
    for(auto it = range.begin(); it != range.end(); ++it) {
        const UsdPrim& prim = *it;
        if(!prim.IsA<UsdGeomImageable>()) {
            // Non-imagables are not skinnable.
            TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                "[UsdSkelBakeSkinning]:  Pruning traversal at <%s> "
                "(prim types is not a UsdGeomImageable)\n",
                prim.GetPath().GetText());
            it.PruneChildren();
            continue;
        }

        if(UsdSkelSkeletonQuery skelQuery = skelCache.GetSkelQuery(prim)) {
            
            std::vector<std::pair<UsdPrim,UsdSkelSkinningQuery> > skinnedPrims;
            if(!skelCache.ComputeSkinnedPrims(prim, &skinnedPrims))
                return false;

            TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                "[UsdSkelBakeSkinning] Found bound skeleton with %zu candidate "
                "prims for skinning at <%s>\n", skinnedPrims.size(),
                prim.GetPath().GetText());

            for(const auto& pair : skinnedPrims) {
                const UsdPrim& skinnedPrim = pair.first;
                const UsdSkelSkinningQuery& skinningQuery = pair.second;
                
                if(!skinningQuery) {
                    TF_WARN("Skinnable prim <%s> had invalid joint influences.",
                            skinnedPrim.GetPath().GetText());
                    return false;
                }

                TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                    "[UsdSkelBakeSkinning]  Attempting to skin prim <%s>\n",
                    skinnedPrim.GetPath().GetText());

                // Determine what times to author deformed prim data on.
                std::vector<double> times;
                _GetSkinningTimeSamples(skinnedPrim, skelQuery,
                                        skinningQuery,
                                        interval, &times);
                _MergeTimeSamples(&allPrimTimes, times, &tmpTimes);

                // Get times in terms of time codes, so that defaults
                // can be sampled, if necessary.
                std::vector<UsdTimeCode> timeCodes(times.begin(), times.end());
                if(timeCodes.size() == 0) {
                    timeCodes.push_back(UsdTimeCode::Default());
                }

                if(skinningQuery.IsRigidlyDeformed()) {
                    if(!_BakeSkinnedTransform(skinnedPrim, skelQuery,
                                              skinningQuery, timeCodes,
                                              &xfCache)) {
                        return false;
                    }
                } else {
                    if(!skinnedPrim.IsA<UsdGeomPointBased>()) {
                        // XXX: This is not an error!
                        // There might be custom types that do not inherit
                        // from UsdGeomPointBased that some clients know
                        // how to apply varying deformations to.
                        // It is the responsibility of whomever is computing
                        // skinning to decide whether or not they know how
                        // to skin prims.

                        TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                            "   Skipping point skinning "
                            "(prim is not a UsdGeomPointBased)\n.");
                        continue;
                    }

                    if(!_BakeSkinnedPoints(skinnedPrim, skelQuery,
                                           skinningQuery, timeCodes,
                                           &xfCache)) {
                        return false;
                    }
                }
            }
        }
    }

    // Define a transform over the skel root.
    // This disables skeletal processing for the scope.
    // (I.e., back to normal mesh land!)
    UsdGeomXform::Define(root.GetPrim().GetStage(), root.GetPrim().GetPath());

    // If any prims are storing extents hints, update the hints now,
    // against the union of all times.
    std::vector<UsdTimeCode> allPrimTimeCodes(allPrimTimes.begin(),
                                              allPrimTimes.end());
    if(allPrimTimeCodes.size() == 0) {
        allPrimTimeCodes.push_back(UsdTimeCode::Default());
    }
    
    _UpdateExtentsHints(root.GetPrim(), allPrimTimeCodes);
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE
