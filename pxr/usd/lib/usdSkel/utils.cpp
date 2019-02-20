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

#include "pxr/base/arch/hints.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/range3f.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/work/loops.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/relationship.h"

#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usdGeom/modelAPI.h"
#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/usd/usdSkel/animation.h"
#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usdSkel/blendShapeQuery.h"
#include "pxr/usd/usdSkel/cache.h"
#include "pxr/usd/usdSkel/debugCodes.h"
#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/skeletonQuery.h"
#include "pxr/usd/usdSkel/skinningQuery.h"
#include "pxr/usd/usdSkel/topology.h"

#include <atomic>


PXR_NAMESPACE_OPEN_SCOPE


bool
UsdSkelIsSkelAnimationPrim(const UsdPrim& prim)
{
    return prim.IsA<UsdSkelAnimation>();
}


bool
UsdSkelIsSkinnablePrim(const UsdPrim& prim)
{
    // XXX: Note that UsdGeomPointBased prims are boundable prims,
    // so no need to explicitly check for UsdGeomPointBased.
    return prim.IsA<UsdGeomBoundable>() &&
          !prim.IsA<UsdSkelSkeleton>() &&
          !prim.IsA<UsdSkelRoot>();
}


namespace {


/// Wrapper for parallel loops that execs in serial based on the
/// \p inSerial flag, as well as the grain size.
template <typename Fn>
void
_ParallelForN(size_t count, bool inSerial, Fn&& callback, size_t grainSize=1000)
{
    if (inSerial || count < grainSize) {
        WorkSerialForN(count, callback);
    } else {
        WorkParallelForN(count, callback, grainSize);
    }
}


template <typename Matrix4>
void
_InvertTransforms(TfSpan<const Matrix4> xforms, TfSpan<Matrix4> inverseXforms)
{
    TF_DEV_AXIOM(xforms.size() == inverseXforms.size());

    _ParallelForN(xforms.size(), false,
                  [&](size_t start, size_t end)
                  { 
                      for (size_t i = start; i < end; ++i) {
                          inverseXforms[i] = xforms[i].GetInverse();
                      }
                  }, /*grainSize*/ 1000);
}


template <typename Matrix4>
bool
UsdSkel_ConcatJointTransforms(const UsdSkelTopology& topology,
                              TfSpan<const Matrix4> jointLocalXforms,
                              TfSpan<Matrix4> xforms,
                              const Matrix4* rootXform)
{
    TRACE_FUNCTION();

    if (ARCH_UNLIKELY(jointLocalXforms.size() != topology.size())) {
        TF_WARN("Size of jointLocalXforms [%td] != number of joints [%zu]",
                jointLocalXforms.size(), topology.size());
        return false;
    }
    if (ARCH_UNLIKELY(xforms.size() != topology.size())) {
        TF_WARN("Size of xforms [%td] != number of joints [%zu]",
                xforms.size(), topology.size());
        return false;
    }

    for (size_t i = 0; i < topology.size(); ++i) {
        const int parent = topology.GetParent(i);
        if (parent >= 0) {
            if (static_cast<size_t>(parent) < i) {
                xforms[i] = jointLocalXforms[i] * xforms[parent];
            } else {
                if (static_cast<size_t>(parent) == i) {
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
            if (rootXform) {
                xforms[i] *= (*rootXform);
            }
        }
    }
    return true;
}


} // namespace


bool
UsdSkelConcatJointTransforms(const UsdSkelTopology& topology,
                             TfSpan<const GfMatrix4d> jointLocalXforms,
                             TfSpan<GfMatrix4d> xforms,
                             const GfMatrix4d* rootXform)
{
    return UsdSkel_ConcatJointTransforms(topology, jointLocalXforms,
                                         xforms, rootXform);
}


bool
UsdSkelConcatJointTransforms(const UsdSkelTopology& topology,
                             TfSpan<const GfMatrix4f> jointLocalXforms,
                             TfSpan<GfMatrix4f> xforms,
                             const GfMatrix4f* rootXform)
{
    return UsdSkel_ConcatJointTransforms(topology, jointLocalXforms,
                                         xforms, rootXform);
}


// deprecated
bool
UsdSkelConcatJointTransforms(const UsdSkelTopology& topology,
                             const VtMatrix4dArray& localXforms,
                             VtMatrix4dArray* xforms,
                             const GfMatrix4d* rootXform)
{
    if (!xforms) {
        TF_CODING_ERROR("'xforms' is null");
        return false;
    }
    xforms->resize(topology.size());
    return UsdSkelConcatJointTransforms(
        topology, localXforms, *xforms, rootXform);
}


// deprecated
bool
UsdSkelConcatJointTransforms(const UsdSkelTopology& topology,
                             const GfMatrix4d* jointLocalXforms,
                             GfMatrix4d* xforms,
                             const GfMatrix4d* rootXform)
{
    return UsdSkelConcatJointTransforms(
        topology,
        TfSpan<const GfMatrix4d>(jointLocalXforms, topology.size()),
        TfSpan<GfMatrix4d>(xforms, topology.size()),
        rootXform);
}


namespace {


template <typename Matrix4>
bool
UsdSkel_ComputeJointLocalTransforms(const UsdSkelTopology& topology,
                                    TfSpan<const Matrix4> xforms,
                                    TfSpan<const Matrix4> inverseXforms,
                                    TfSpan<Matrix4> jointLocalXforms,
                                    const Matrix4* rootInverseXform)
{
    TRACE_FUNCTION();

    if (ARCH_UNLIKELY(xforms.size() != topology.size())) {
        TF_WARN("Size of xforms [%td] != number of joints [%zu]",
                xforms.size(), topology.size());
        return false;
    }
    if (ARCH_UNLIKELY(inverseXforms.size() != topology.size())) {
        TF_WARN("Size of inverseXforms [%td] != number of joints [%zu]",
                inverseXforms.size(), topology.size());
        return false;
    }
    if (ARCH_UNLIKELY(jointLocalXforms.size() != topology.size())) {
        TF_WARN("Size of jointLocalXforms [%td] != number of joints [%zu]",
                jointLocalXforms.size(), topology.size());
        return false;
    }

    // Skel-space transforms are computed as:
    //     skelXform = jointLocalXform*parentSkelXform
    // So we want:
    //     jointLocalXform = skelXform*inv(parentSkelXform)

    for (size_t i = 0; i < topology.size(); ++i) {
        const int parent = topology.GetParent(i);
        if (parent >= 0) {
            if (static_cast<size_t>(parent) < i) {
                jointLocalXforms[i] = xforms[i]*inverseXforms[parent];
            } else {
                if (static_cast<size_t>(parent) == i) {
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
            if (rootInverseXform) {
                jointLocalXforms[i] *= (*rootInverseXform);
            }
        }
    }
    return true;
}


template <typename Matrix4>
bool
UsdSkel_ComputeJointLocalTransforms(const UsdSkelTopology& topology,
                                    TfSpan<const Matrix4> xforms,
                                    TfSpan<Matrix4> jointLocalXforms,
                                    const Matrix4* rootInverseXform)
{
    TRACE_FUNCTION();

    std::vector<Matrix4> inverseXforms(xforms.size());
    _InvertTransforms<Matrix4>(xforms, inverseXforms);
    return UsdSkel_ComputeJointLocalTransforms<Matrix4>(
        topology, xforms, inverseXforms, jointLocalXforms, rootInverseXform);
}


} // namespace


bool
UsdSkelComputeJointLocalTransforms(const UsdSkelTopology& topology,
                                   TfSpan<const GfMatrix4d> xforms,
                                   TfSpan<const GfMatrix4d> inverseXforms,
                                   TfSpan<GfMatrix4d> jointLocalXforms,
                                   const GfMatrix4d* rootInverseXform)
{
    return UsdSkel_ComputeJointLocalTransforms(
        topology, xforms, inverseXforms, jointLocalXforms, rootInverseXform);
}


bool
UsdSkelComputeJointLocalTransforms(const UsdSkelTopology& topology,
                                   TfSpan<const GfMatrix4f> xforms,
                                   TfSpan<const GfMatrix4f> inverseXforms,
                                   TfSpan<GfMatrix4f> jointLocalXforms,
                                   const GfMatrix4f* rootInverseXform)
{
    return UsdSkel_ComputeJointLocalTransforms(
        topology, xforms, inverseXforms, jointLocalXforms, rootInverseXform);
}


bool
UsdSkelComputeJointLocalTransforms(const UsdSkelTopology& topology,
                                   TfSpan<const GfMatrix4d> xforms,
                                   TfSpan<GfMatrix4d> jointLocalXforms,
                                   const GfMatrix4d* rootInverseXform)
{
    return UsdSkel_ComputeJointLocalTransforms(
        topology, xforms, jointLocalXforms, rootInverseXform);
}


bool
UsdSkelComputeJointLocalTransforms(const UsdSkelTopology& topology,
                                   TfSpan<const GfMatrix4f> xforms,
                                   TfSpan<GfMatrix4f> jointLocalXforms,
                                   const GfMatrix4f* rootInverseXform)
{
    return UsdSkel_ComputeJointLocalTransforms(
        topology, xforms, jointLocalXforms, rootInverseXform);
}


// deprecated
bool
UsdSkelComputeJointLocalTransforms(const UsdSkelTopology& topology,
                                   const VtMatrix4dArray& xforms,
                                   const VtMatrix4dArray& inverseXforms,
                                   VtMatrix4dArray* jointLocalXforms,
                                   const GfMatrix4d* rootInverseXform)
{
    if (!jointLocalXforms) {
        TF_CODING_ERROR("'jointLocalXforms' is null");
        return false;
    }
    jointLocalXforms->resize(topology.size());
    return UsdSkelComputeJointLocalTransforms(
        topology, xforms, inverseXforms, *jointLocalXforms, rootInverseXform);
}


// deprecated
bool
UsdSkelComputeJointLocalTransforms(const UsdSkelTopology& topology,
                                   const VtMatrix4dArray& xforms,
                                   VtMatrix4dArray* jointLocalXforms,
                                   const GfMatrix4d* rootInverseXform)
{
    if (!jointLocalXforms) {
        TF_CODING_ERROR("'jointLocalXforms' is null");
        return false;
    }
    jointLocalXforms->resize(topology.size());
    return UsdSkelComputeJointLocalTransforms(
        topology, xforms, *jointLocalXforms, rootInverseXform);
}


// deprecated
bool
UsdSkelComputeJointLocalTransforms(const UsdSkelTopology& topology,
                                   const GfMatrix4d* xforms,
                                   const GfMatrix4d* inverseXforms,
                                   GfMatrix4d* jointLocalXforms,
                                   const GfMatrix4d* rootInverseXform)
{
    return UsdSkelComputeJointLocalTransforms(
        topology,
        TfSpan<const GfMatrix4d>(xforms, topology.size()),
        TfSpan<const GfMatrix4d>(inverseXforms, topology.size()),
        TfSpan<GfMatrix4d>(jointLocalXforms, topology.size()),
        rootInverseXform);
}


namespace {


/// Helper to return the GfVec3 type with the same precision
/// as the given matrix type.
template <typename T>
struct _Vec3MatchingMatrix4 {};

template <>
struct _Vec3MatchingMatrix4<GfMatrix4d>
{   
    using type = GfVec3d;
};

template <>
struct _Vec3MatchingMatrix4<GfMatrix4f>
{   
    using type = GfVec3f;
};


template <typename Matrix4>
bool
_DecomposeTransform(const Matrix4& xform,
                    GfVec3f* translate,
                    GfRotation* rotate,
                    GfVec3h* scale)
{
    // XXX: GfMatrix4x::Factor() may crash if the value isn't properly aligned.
    TF_DEV_AXIOM(size_t(&xform)%alignof(Matrix4) == 0);
    
    // Decomposition must account for handedness changes due to negative scales.
    // This is similar to GfMatrix4d::RemoveScaleShear().
    Matrix4 scaleOrient, factoredRot, perspMat;

    using Vec3 = typename _Vec3MatchingMatrix4<Matrix4>::type;
    
    Vec3 factoredScale, factoredTranslate;;
    if (xform.Factor(&scaleOrient, &factoredScale,
                    &factoredRot, &factoredTranslate, &perspMat)) {

        if (factoredRot.Orthonormalize()) {
            *rotate = factoredRot.ExtractRotation();
            *scale = GfVec3h(factoredScale);
            *translate = GfVec3f(factoredTranslate);
            return true;
        }
    }
    return false;
}


template <typename Matrix4>
bool
_DecomposeTransform(const Matrix4& xform,
                    GfVec3f* translate,
                    GfQuatf* rotate,
                    GfVec3h* scale)
{
    GfRotation r;
    if (_DecomposeTransform(xform, translate, &r, scale)) {
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


template <typename Matrix4>
bool
UsdSkelDecomposeTransform(const Matrix4& xform,
                          GfVec3f* translate,  
                          GfRotation* rotate,
                          GfVec3h* scale)
{
    TRACE_FUNCTION();

    if (!translate) {
        TF_CODING_ERROR("'translate' pointer is null.");
        return false;
    }
    if (!rotate) {
        TF_CODING_ERROR("'rotate' pointer is null.");
        return false;
    }
    if (!scale) {
        TF_CODING_ERROR("'scale' pointer is null.");
        return false;
    }
    return _DecomposeTransform(xform, translate, rotate, scale);
}


template
USDSKEL_API bool UsdSkelDecomposeTransform(const GfMatrix4d&,
                                           GfVec3f*, GfRotation*, GfVec3h*);

template
USDSKEL_API bool UsdSkelDecomposeTransform(const GfMatrix4f&,
                                           GfVec3f*, GfRotation*, GfVec3h*);


template <typename Matrix4>
bool
UsdSkelDecomposeTransform(const Matrix4& xform,
                          GfVec3f* translate,  
                          GfQuatf* rotate,
                          GfVec3h* scale)
{
    TRACE_FUNCTION();

    if (!translate) {
        TF_CODING_ERROR("'translate' pointer is null.");
        return false;
    }
    if (!rotate) {
        TF_CODING_ERROR("'rotate' pointer is null.");
        return false;
    }
    if (!scale) {
        TF_CODING_ERROR("'scale' pointer is null.");
        return false;
    }
    return _DecomposeTransform(xform, translate, rotate, scale);
}


template
USDSKEL_API bool UsdSkelDecomposeTransform(const GfMatrix4d&,
                                           GfVec3f*, GfQuatf*, GfVec3h*);

template
USDSKEL_API bool UsdSkelDecomposeTransform(const GfMatrix4f&,
                                           GfVec3f*, GfQuatf*, GfVec3h*);


namespace {


template <typename Matrix4>
bool
UsdSkel_DecomposeTransforms(TfSpan<const Matrix4> xforms,
                            TfSpan<GfVec3f> translations,
                            TfSpan<GfQuatf> rotations,
                            TfSpan<GfVec3h> scales)
{
    TRACE_FUNCTION();

    if (translations.size() != xforms.size()) {
        TF_WARN("Size of translations [%td] != size of xforms [%td]",
                translations.size(), xforms.size());
        return false;
    }
    if (rotations.size() != xforms.size()) {
        TF_WARN("Size of rotations [%td] != size of xforms [%td]",
                rotations.size(), xforms.size());
        return false;
    }
    if (scales.size() != xforms.size()) {
        TF_WARN("Size of scales [%td] != size of xforms [%td]",
                scales.size(), xforms.size());
        return false;
    }

    // Flag for marking error state from within threads.
    std::atomic_bool errors(false);

    _ParallelForN(
        xforms.size(), /*inSerial*/ false,
        [&](size_t start, size_t end)
        {
            for (size_t i = start; i < end; ++i) {
                
                if (!_DecomposeTransform(xforms[i], &translations[i],
                                         &rotations[i], &scales[i])) {

                    TF_WARN("Failed decomposing transform %zu. "
                            "The source transform may be singular.", i);
                    errors = true;
                    return;
                }
            }
        });

    return !errors;
}


} // namespace


bool
UsdSkelDecomposeTransforms(TfSpan<const GfMatrix4d> xforms,
                           TfSpan<GfVec3f> translations,
                           TfSpan<GfQuatf> rotations,
                           TfSpan<GfVec3h> scales)
{
    return UsdSkel_DecomposeTransforms(xforms, translations, rotations, scales);
}


bool
UsdSkelDecomposeTransforms(TfSpan<const GfMatrix4f> xforms,
                           TfSpan<GfVec3f> translations,
                           TfSpan<GfQuatf> rotations,
                           TfSpan<GfVec3h> scales)
{
    return UsdSkel_DecomposeTransforms(xforms, translations, rotations, scales);
}



// deprecated
bool
UsdSkelDecomposeTransforms(const GfMatrix4d* xforms,
                           GfVec3f* translations,
                           GfQuatf* rotations,
                           GfVec3h* scales,
                           size_t count)
{
    return UsdSkelDecomposeTransforms(
        TfSpan<const GfMatrix4d>(xforms, count),
        TfSpan<GfVec3f>(translations, count),
        TfSpan<GfQuatf>(rotations, count),
        TfSpan<GfVec3h>(scales, count));
}


// deprecated
bool
UsdSkelDecomposeTransforms(const VtMatrix4dArray& xforms,
                           VtVec3fArray* translations,
                           VtQuatfArray* rotations,
                           VtVec3hArray* scales)
{
    if (!translations) {
        TF_CODING_ERROR("'translations' pointer is null.");
        return false;
    }
    if (!rotations) {
        TF_CODING_ERROR("'rotations' pointer is null.");
        return false;
    }
    if (!scales) {
        TF_CODING_ERROR("'scales' pointer is null.");
        return false;
    }

    translations->resize(xforms.size());
    rotations->resize(xforms.size());
    scales->resize(xforms.size());

    return UsdSkelDecomposeTransforms(xforms, *translations,
                                      *rotations, *scales);
}


template <typename Matrix4>
void
UsdSkelMakeTransform(const GfVec3f& translate,
                     const GfMatrix3f& rotate,
                     const GfVec3h& scale,
                     Matrix4* xform)
{
    if (xform) {
        // Order is scale*rotate*translate
        *xform = Matrix4(rotate[0][0]*scale[0],
                         rotate[0][1]*scale[0],
                         rotate[0][2]*scale[0], 0,

                         rotate[1][0]*scale[1],
                         rotate[1][1]*scale[1],
                         rotate[1][2]*scale[1], 0,

                         rotate[2][0]*scale[2],
                         rotate[2][1]*scale[2],
                         rotate[2][2]*scale[2], 0,

                         translate[0], translate[1], translate[2], 1);
    } else {
        TF_CODING_ERROR("'xform' is null");
    }
}


template USDSKEL_API void
UsdSkelMakeTransform(const GfVec3f&, const GfMatrix3f&,
                                 const GfVec3h&, GfMatrix4d*);

template USDSKEL_API void
UsdSkelMakeTransform(const GfVec3f&, const GfMatrix3f&,
                                 const GfVec3h&, GfMatrix4f*);


template <typename Matrix4>
void
UsdSkelMakeTransform(const GfVec3f& translate,
                     const GfQuatf& rotate,
                     const GfVec3h& scale,
                     Matrix4* xform)
{
    UsdSkelMakeTransform(translate, GfMatrix3f(rotate), scale, xform);
}


template USDSKEL_API void
UsdSkelMakeTransform(const GfVec3f&, const GfQuatf&,
                                 const GfVec3h&, GfMatrix4d*);

template USDSKEL_API void
UsdSkelMakeTransform(const GfVec3f&, const GfQuatf&,
                                 const GfVec3h&, GfMatrix4f*);


namespace {


template <typename Matrix4>
bool
UsdSkel_MakeTransforms(TfSpan<const GfVec3f> translations,
                       TfSpan<const GfQuatf> rotations,
                       TfSpan<const GfVec3h> scales,
                       TfSpan<Matrix4> xforms)
{
    TRACE_FUNCTION();

    if (ARCH_UNLIKELY(translations.size() != xforms.size())) {
        TF_WARN("Size of translations [%td] != size of xforms [%td]",
                translations.size(), xforms.size());
        return false;
    }
    if (ARCH_UNLIKELY(rotations.size() != xforms.size())) {
        TF_WARN("Size of rotations [%td] != size of xforms [%td]",
                rotations.size(), xforms.size());
        return false;
    }
    if (ARCH_UNLIKELY(scales.size() != xforms.size())) {
        TF_WARN("Size of scales [%td] != size of xforms [%td]",
                scales.size(), xforms.size());
        return false;
    }

    for (ptrdiff_t i = 0; i < xforms.size(); ++i) {
        UsdSkelMakeTransform(translations[i], rotations[i],
                             scales[i], &xforms[i]);
    }
    return true;
}


} // namespace


bool
UsdSkelMakeTransforms(TfSpan<const GfVec3f> translations,
                      TfSpan<const GfQuatf> rotations,
                      TfSpan<const GfVec3h> scales,
                      TfSpan<GfMatrix4d> xforms)
{
    return UsdSkel_MakeTransforms(translations, rotations, scales, xforms);
}


bool
UsdSkelMakeTransforms(TfSpan<const GfVec3f> translations,
                      TfSpan<const GfQuatf> rotations,
                      TfSpan<const GfVec3h> scales,
                      TfSpan<GfMatrix4f> xforms)
{
    return UsdSkel_MakeTransforms(translations, rotations, scales, xforms);
}


// deprecated
bool
UsdSkelMakeTransforms(const GfVec3f* translations,
                      const GfQuatf* rotations,
                      const GfVec3h* scales,
                      GfMatrix4d* xforms,
                      size_t count)
{
    return UsdSkelMakeTransforms(
        TfSpan<const GfVec3f>(translations, count),
        TfSpan<const GfQuatf>(rotations, count),
        TfSpan<const GfVec3h>(scales, count),
        TfSpan<GfMatrix4d>(xforms, count));
        
}


// deprecated
bool
UsdSkelMakeTransforms(const VtVec3fArray& translations,
                      const VtQuatfArray& rotations,
                      const VtVec3hArray& scales,
                      VtMatrix4dArray* xforms)
{
    if (!xforms) {
        TF_CODING_ERROR("'xforms' pointer is null.");
        return false;
    }
    xforms->resize(translations.size());
    
    return UsdSkelMakeTransforms(
        translations, rotations, scales, *xforms);
}


template <typename Matrix4>
bool
UsdSkelComputeJointsExtent(TfSpan<const Matrix4> xforms,
                           GfRange3f* extent,
                           float pad,
                           const Matrix4* rootXform)
{
    TRACE_FUNCTION();

    if (!extent) {
        TF_CODING_ERROR("'extent' pointer is null.");
        return false;
    }

    for (size_t i = 0; i < xforms.size(); ++i) {
        const GfVec3f pivot(xforms[i].ExtractTranslation());
        extent->UnionWith(rootXform ?
                          rootXform->TransformAffine(pivot) : pivot);
    }
    const GfVec3f padVec(pad);
    extent->SetMin(extent->GetMin()-padVec);
    extent->SetMax(extent->GetMax()+padVec);
    return true;
}


template USDSKEL_API bool
UsdSkelComputeJointsExtent(TfSpan<const GfMatrix4d>,
                           GfRange3f*, float, const GfMatrix4d*);

template USDSKEL_API bool
UsdSkelComputeJointsExtent(TfSpan<const GfMatrix4f>,
                           GfRange3f*, float, const GfMatrix4f*);



// deprecated
bool
UsdSkelComputeJointsExtent(const VtMatrix4dArray& joints,
                           VtVec3fArray* extent,
                           float pad,
                           const GfMatrix4d* rootXform)
{
    GfRange3f range;
    if (UsdSkelComputeJointsExtent<GfMatrix4d>(
            joints, &range, pad, rootXform)) {

        extent->resize(2);
        (*extent)[0] = range.GetMin();
        (*extent)[1] = range.GetMax();
        return true;
    }
    return false;
}


// deprecated
bool
UsdSkelComputeJointsExtent(const GfMatrix4d* xforms,
                           size_t count,
                           VtVec3fArray* extent,
                           float pad,
                           const GfMatrix4d* rootXform)
{
    GfRange3f range;
    if (UsdSkelComputeJointsExtent<GfMatrix4d>(
            TfSpan<const GfMatrix4d>(xforms, count),
            &range, pad, rootXform)) {
        extent->resize(2);
        (*extent)[0] = range.GetMin();
        (*extent)[1] = range.GetMax();
        return true;
    }
    return false;
}


namespace {

/// Validate the size of a weight/index array for a given
/// number of influences per component.
/// Throws a warning for failed validation.
bool
_ValidateArrayShape(ptrdiff_t size, int numInfluencesPerComponent)
{
    if (numInfluencesPerComponent > 0) {
        if (size%numInfluencesPerComponent == 0) {
            return true;
        } else {
            TF_WARN("Unexpected array size [%td]: Size must be a multiple of "
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

} // namespace


bool
UsdSkelNormalizeWeights(TfSpan<float> weights,
                        int numInfluencesPerComponent)
{
    TRACE_FUNCTION();
    
    if (!_ValidateArrayShape(weights.size(), numInfluencesPerComponent)) {
        return false;
    }

    const ptrdiff_t numComponents = weights.size()/numInfluencesPerComponent;

    _ParallelForN(
        numComponents, /* inSerial = */ false,
        [&](size_t start, size_t end)
        {
            for (size_t i = start; i < end; ++i) {
                
                float* weightSet = weights.data() + i*numInfluencesPerComponent;

                float sum = 0.0f;
                for(int j = 0; j < numInfluencesPerComponent; ++j) {
                    sum += weightSet[j];
                }

                if (std::abs(sum) > std::numeric_limits<float>::epsilon()) {
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


// deprecated
bool
UsdSkelNormalizeWeights(VtFloatArray* weights, int numInfluencesPerComponent)
{
    if (!weights) {
        TF_CODING_ERROR("'weights' pointer is null.");
        return false;
    }
    return UsdSkelNormalizeWeights(*weights, numInfluencesPerComponent);
}


bool
UsdSkelSortInfluences(TfSpan<int> indices,
                      TfSpan<float> weights,
                      int numInfluencesPerComponent)
{
    TRACE_FUNCTION();

    if (indices.size() != weights.size()) {
        TF_WARN("Size of 'indices' [%td] != size of 'weights' [%td].",
                indices.size(), weights.size());
        return false;
    }
    if (!_ValidateArrayShape(indices.size(), numInfluencesPerComponent)) {
        return false;
    }

    if (numInfluencesPerComponent < 2) {
        // Nothing to do.
        return true;
    }

    const ptrdiff_t numComponents = indices.size()/numInfluencesPerComponent;

    _ParallelForN(
        numComponents, /* inSerial = */ false,
        [&](size_t start, size_t end)
        {
            std::vector<std::pair<float,int> > influences;
            for (size_t i = start; i < end; ++i) {
                const size_t offset = i*numInfluencesPerComponent;
                float* weightsSet = weights.data() + offset;
                int *indexSet = indices.data() + offset;

                influences.resize(numInfluencesPerComponent);
                for (int j = 0; j < numInfluencesPerComponent; ++j) {
                    influences[j] = std::make_pair(weightsSet[j], indexSet[j]);
                }
                std::sort(influences.begin(), influences.end(),
                          std::greater<std::pair<float,int> >());
                for (int j = 0; j < numInfluencesPerComponent; ++j) {
                    const auto& pair = influences[j];
                    weightsSet[j] = pair.first;
                    indexSet[j] = pair.second;
                }
            }
        });

    return true;
}


// deprecated
bool
UsdSkelSortInfluences(VtIntArray* indices,
                      VtFloatArray* weights,
                      int numInfluencesPerComponent)
{
    if (!indices) {
        TF_CODING_ERROR("'indices' pointer is null.");
        return false;
    }
    if (!weights) {
        TF_CODING_ERROR("'weights' pointer is null.");
        return false;
    }

    return UsdSkelSortInfluences(*indices, *weights, numInfluencesPerComponent);
}


namespace {

template <typename T>
bool
_ExpandConstantArray(T* array, size_t size)
{
    if (!array) {
        TF_CODING_ERROR("'array' pointer is null.");
        return false;
    }

    if (size == 0) {
        array->clear();
    } else {
        size_t numInfluencesPerComponent = array->size();
        array->resize(numInfluencesPerComponent*size);

        auto* data = array->data();
        for (size_t i = 1; i < size; ++i) {
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
    if (srcNumInfluencesPerComponent == newNumInfluencesPerComponent)
        return true;

    if (!array) {
        TF_CODING_ERROR("'array' pointer is null.");
        return false;
    }

    if (!_ValidateArrayShape(array->size(), srcNumInfluencesPerComponent))
        return false;

    size_t numComponents = array->size()/srcNumInfluencesPerComponent;
    if (numComponents == 0)
        return true;

    if (newNumInfluencesPerComponent < srcNumInfluencesPerComponent) {
        // Truncate influences in-place.
        auto* data = array->data();
        for (size_t i = 1; i < numComponents; ++i) {
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
        for (size_t i = 0; i < numComponents; ++i) { 
            // Reverse the order.
            size_t idx = numComponents-i-1;

            // Copy source values (*reverse order*)
            for (int j = (srcNumInfluencesPerComponent-1); j >= 0; --j) {
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

    if (_ResizeInfluences(weights, srcNumInfluencesPerComponent,
                         newNumInfluencesPerComponent, 0.0f)) {
        if (newNumInfluencesPerComponent < srcNumInfluencesPerComponent) {
            // Some weights have been stripped off. Need to renormalize.
            return UsdSkelNormalizeWeights(
                weights, newNumInfluencesPerComponent);
        }
        return true;
    }
    return false;
}


namespace {


template <typename Matrix4>
bool
_SkinPointsLBS(const Matrix4& geomBindTransform,
               TfSpan<const Matrix4> jointXforms,
               TfSpan<const int> jointIndices,
               TfSpan<const float> jointWeights,
               int numInfluencesPerPoint,
               TfSpan<GfVec3f> points,
               bool inSerial)
{
    TRACE_FUNCTION();

    if (jointIndices.size() != jointWeights.size()) {
        TF_WARN("Size of jointIndices [%td] != size of jointWeights [%td]",
                jointIndices.size(), jointWeights.size());
        return false;
    }
    
    if (jointIndices.size() != (points.size()*numInfluencesPerPoint)) {
        TF_WARN("Size of jointIndices [%td] != "
                "(points.size() [%td] * numInfluencesPerPoint [%d]).",
                jointIndices.size(), points.size(), numInfluencesPerPoint);
        return false;
    }

    // Flag for marking error state from within threads.
    std::atomic_bool errors(false);

    _ParallelForN(
        points.size(), /* inSerial = */ inSerial,
        [&](size_t start, size_t end)
        {
            for (size_t pi = start; pi < end; ++pi) {

                const GfVec3f initialP = geomBindTransform.Transform(points[pi]);
                GfVec3f p(0,0,0);
                
                for (int wi = 0; wi < numInfluencesPerPoint; ++wi) {
                    const size_t influenceIdx = pi*numInfluencesPerPoint + wi;
                    const int jointIdx = jointIndices[influenceIdx];

                    if (jointIdx >= 0 &&
                       static_cast<ptrdiff_t>(jointIdx) < jointXforms.size()) {

                        float w = jointWeights[influenceIdx];
                        if (w != 0.0f) {
                            // Since joint transforms are encoded in terms of
                            // t,r,s components, it shouldn't be possible to
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
                        // if (weightIsNull)
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
                                " (num joints = %td).",
                                jointIdx, influenceIdx, jointXforms.size());
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


} // namespace


bool
UsdSkelSkinPointsLBS(const GfMatrix4d& geomBindTransform,
                     TfSpan<const GfMatrix4d> jointXforms,
                     TfSpan<const int> jointIndices,
                     TfSpan<const float> jointWeights,
                     int numInfluencesPerPoint,
                     TfSpan<GfVec3f> points,
                     bool inSerial)
{
    return _SkinPointsLBS(geomBindTransform, jointXforms,
                          jointIndices, jointWeights, numInfluencesPerPoint,
                          points, inSerial);
}


bool
UsdSkelSkinPointsLBS(const GfMatrix4f& geomBindTransform,
                     TfSpan<const GfMatrix4f> jointXforms,
                     TfSpan<const int> jointIndices,
                     TfSpan<const float> jointWeights,
                     int numInfluencesPerPoint,
                     TfSpan<GfVec3f> points,
                     bool inSerial)
{
    return _SkinPointsLBS(geomBindTransform, jointXforms,
                          jointIndices, jointWeights, numInfluencesPerPoint,
                          points, inSerial);
}


// deprecated
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
                     bool inSerial)
{
    return UsdSkelSkinPointsLBS(
        geomBindTransform,
        TfSpan<const GfMatrix4d>(jointXforms, numJoints),
        TfSpan<const int>(jointIndices, numInfluences),
        TfSpan<const float>(jointWeights, numInfluences),
        numInfluencesPerPoint,
        TfSpan<GfVec3f>(points, numPoints),
        inSerial);
}


// deprecated
bool
UsdSkelSkinPointsLBS(const GfMatrix4d& geomBindTransform,
                     const VtMatrix4dArray& jointXforms,
                     const VtIntArray& jointIndices,
                     const VtFloatArray& jointWeights,
                     int numInfluencesPerPoint,
                     VtVec3fArray* points)
{
    if (!points) {
        TF_CODING_ERROR("'points' pointer is null.");
        return false;
    }

    return UsdSkelSkinPointsLBS(
        geomBindTransform, jointXforms,
        jointIndices, jointWeights,
        numInfluencesPerPoint, *points);
}


namespace {


template <typename Matrix4>
bool
UsdSkel_SkinTransformLBS(const Matrix4& geomBindTransform,
                         TfSpan<const Matrix4> jointXforms,
                         TfSpan<const int> jointIndices,
                         TfSpan<const float> jointWeights,
                         Matrix4* xform)
{
    TRACE_FUNCTION();

    if (!xform) {
        TF_CODING_ERROR("'xform' is null");
        return false;
    }

    if (jointIndices.size() != jointWeights.size()) {
        TF_WARN("Size of jointIndices [%td] != size of jointWeights [%td]",
                jointIndices.size(), jointWeights.size());
        return false;
    }

    // Early-out for the common case where an object is rigidly
    // bound to a single joint.
    if (jointIndices.size() == 1 && GfIsClose(jointWeights[0], 1.0f, 1e-6)) {
        const int jointIdx = jointIndices[0];
        if (jointIdx >= 0 &&
            static_cast<ptrdiff_t>(jointIdx) < jointXforms.size()) {
            *xform = geomBindTransform*jointXforms[jointIdx];
            return true;
        } else {
            TF_WARN("Out of range joint index %d at index 0"
                    " (num joints = %td).", jointIdx, jointXforms.size());
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

    const GfVec3f pivot(geomBindTransform.ExtractTranslation());

    // XXX: Note that if precision becomes an issue, the offset applied to
    // produce the points that represent each of the basis vectors can be scaled
    // up to improve precision, provided that the inverse scale is applied when
    // constructing the final matrix.
    GfVec3f framePoints[4] = {
        pivot + GfVec3f(geomBindTransform.GetRow3(0)), // i basis
        pivot + GfVec3f(geomBindTransform.GetRow3(1)), // j basis
        pivot + GfVec3f(geomBindTransform.GetRow3(2)), // k basis
        pivot, // translate
    };

    for (int pi = 0; pi < 4; ++pi) {
        const GfVec3f initialP = framePoints[pi];

        GfVec3f p(0,0,0);
        for (ptrdiff_t wi = 0; wi < jointIndices.size(); ++wi) {
            const int jointIdx = jointIndices[wi];
            if (jointIdx >= 0 &&
                static_cast<ptrdiff_t>(jointIdx) < jointXforms.size()) {
                const float w = jointWeights[wi];
                if (w != 0.0f) {
                    // XXX: See the notes from _SkinPointsLBS():
                    // affine transforms should be okay.
                    p += jointXforms[jointIdx].TransformAffine(initialP)*w;
                }
            } else {
                TF_WARN("Out of range joint index %d at index %zu"
                        " (num joints = %td).",
                        jointIdx, wi, jointXforms.size());
                return false;
            }
        }
        framePoints[pi] = p;
    }

    const GfVec3f skinnedPivot = framePoints[3];
    xform->SetTranslate(skinnedPivot);
    for (int i = 0; i < 3; ++i) {
        xform->SetRow3(i, (framePoints[i]-skinnedPivot));
    }
    return true;
}


} // namespace


bool
UsdSkelSkinTransformLBS(const GfMatrix4d& geomBindTransform,
                        TfSpan<const GfMatrix4d> jointXforms,
                        TfSpan<const int> jointIndices,
                        TfSpan<const float> jointWeights,
                        GfMatrix4d* xform)
{
    return UsdSkel_SkinTransformLBS(geomBindTransform, jointXforms,
                                    jointIndices, jointWeights, xform);
}


bool
UsdSkelSkinTransformLBS(const GfMatrix4f& geomBindTransform,
                        TfSpan<const GfMatrix4f> jointXforms,
                        TfSpan<const int> jointIndices,
                        TfSpan<const float> jointWeights,
                        GfMatrix4f* xform)
{
    return UsdSkel_SkinTransformLBS(geomBindTransform, jointXforms,
                                    jointIndices, jointWeights, xform);
}


/// \deprecated
bool
UsdSkelSkinTransformLBS(const GfMatrix4d& geomBindTransform,
                        const GfMatrix4d* jointXforms,
                        size_t numJoints,
                        const int* jointIndices,
                        const float* jointWeights,
                        size_t numInfluences,
                        GfMatrix4d* xform)
{
    return UsdSkel_SkinTransformLBS(
        geomBindTransform,
        TfSpan<const GfMatrix4d>(jointXforms, numJoints),
        TfSpan<const int>(jointIndices, numInfluences),
        TfSpan<const float>(jointWeights, numInfluences),
        xform);
}


/// \deprecated
bool
UsdSkelSkinTransformLBS(const GfMatrix4d& geomBindTransform,
                        const VtMatrix4dArray& jointXforms,
                        const VtIntArray& jointIndices,
                        const VtFloatArray& jointWeights,
                        GfMatrix4d* xform)
{
    return UsdSkel_SkinTransformLBS<GfMatrix4d>(
        geomBindTransform, jointXforms,
        jointIndices, jointWeights, xform);
}


namespace {


/// Apply indexed offsets to \p points.
bool
UsdSkel_ApplyIndexedBlendShape(const float weight,
                               const TfSpan<const GfVec3f> offsets,
                               const TfSpan<const unsigned> indices,
                               TfSpan<GfVec3f> points)
{
    TRACE_FUNCTION();

    // Flag for marking error state from within threads.
    std::atomic_bool errors(false);

    _ParallelForN(
        offsets.size(), /*inSerial*/ false,
        [&](size_t start, size_t end)
        {  
            for (size_t i = start; i < end; ++i) {
                const unsigned index = indices[i];
                if (static_cast<ptrdiff_t>(index) < points.size()) { 
                    points[index] += offsets[i]*weight;
                }  else {
                    // XXX: If one offset index is bad, an asset has probably
                    // gotten out of sync, and probably many other indices
                    // will be invalid, too. Bail out early.
                    TF_WARN("Out of range point index %d (num points = %td).",
                            index, points.size());
                    errors = true;
                    return;
                }
            }
        });

    return !errors;
}


/// Apply non-indexed offsets to \p points.
void
UsdSkel_ApplyNonIndexedBlendShape(const float weight,
                                  const TfSpan<const GfVec3f> offsets,
                                  TfSpan<GfVec3f> points)
{
    TRACE_FUNCTION();

    _ParallelForN(
        points.size(), /*inSerial*/ false,
        [&](size_t start, size_t end)
        {
            for (size_t i = start; i < end; ++i) {
                points[i] += offsets[i]*weight;
            }
        });
}


} // namespace
                        

bool
UsdSkelApplyBlendShape(const float weight,
                       const TfSpan<const GfVec3f> offsets,
                       const TfSpan<const unsigned> indices,
                       TfSpan<GfVec3f> points)
{
    // Early out if weights are zero.
    if (GfIsClose(weight, 0.0, 1e-6)) {
        return true;
    }

    if (indices.empty()) {

        if (offsets.size() == points.size()) {
            UsdSkel_ApplyNonIndexedBlendShape(weight, offsets, points);
        } else {
            TF_WARN("Size of non-indexed offsets [%zu] != size of points [%zu]",
                    offsets.size(), points.size());
            return false;
        }

    } else {
        
        if (offsets.size() == indices.size()) {
            
            return UsdSkel_ApplyIndexedBlendShape(
                weight, offsets, indices, points);
        } else {
            TF_WARN("Size of indexed offsets [%zu] != size of indices [%zu]",
                    offsets.size(), indices.size());
            return false;
        }
    }
    return true;
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
    for (UsdPrim p = prim; p; p = p.GetParent()) {
        if (p.IsA<UsdGeomXformable>()) {
            UsdGeomXformable::XformQuery(
                UsdGeomXformable(p)).GetTimeSamplesInInterval(
                    interval, &xformTimeSamples);
            _MergeTimeSamples(times, xformTimeSamples, &tmpTimes);
        }
    }
    return true;
}


/// Populate \p times with time samples in the range [rangeStart, rangeEnd].
/// The samples are added based on the expected sampling rate for playback.
/// I.e., the exact set of time codes that we expect to be queried when
/// the stage is played back at its configured
/// timeCodesPerSecond/framesPerSecond.
bool
_GetScenePlaybackTimeCodesInRange(const UsdStagePtr& stage,
                                  double rangeStart,
                                  double rangeEnd,
                                  std::vector<double>* times)
{
    if (!stage->HasAuthoredTimeCodeRange())
        return false;

    if (rangeStart > rangeEnd)
        return false;

    double timeCodesPerSecond = stage->GetTimeCodesPerSecond();
    double framesPerSecond = stage->GetFramesPerSecond();
    if (GfIsClose(timeCodesPerSecond, 0.0, 1e-6) ||
        GfIsClose(framesPerSecond, 0.0, 1e-6)) {
        return false;
    }
    
    // Compute the expected per-frame time step for playback.
    double playbackTimeStep = std::abs(timeCodesPerSecond/framesPerSecond);

    double stageStart = stage->GetStartTimeCode();  
    double stageEnd = stage->GetEndTimeCode();

    double start = std::min(rangeStart, std::min(stageStart, stageEnd));
    double end = std::max(rangeEnd, std::max(stageEnd, stageStart));

    // Fit the bounding time codes of this start,end region,
    // where t = start+playbackTimeStep*I, I being an integer.
    int64_t frameOffsetToStart =
        std::floor((stageStart-start)/playbackTimeStep);
    start = stageStart + playbackTimeStep*frameOffsetToStart;
    int64_t frameOffsetToEnd = std::floor((stageEnd-end)/playbackTimeStep);
    end = stageEnd + playbackTimeStep*frameOffsetToEnd;

    size_t numTimeCodeSamples =
        (end-start)/playbackTimeStep + 1; // +1 for an inclusive range.
    times->resize(numTimeCodeSamples);
    // Add samples based on integer multiples of the time step to reduce error.
    for (size_t i = 0; i < numTimeCodeSamples; ++i) {
        (*times)[i] = start + playbackTimeStep*i;
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
    if (UsdSkelAnimQuery animQuery = skelQuery.GetAnimQuery()) {
        if (animQuery.GetJointTransformTimeSamplesInInterval(
               interval, &propertyTimes)) {
            _MergeTimeSamples(times, propertyTimes, &tmpTimes);
        }
        if (animQuery.GetBlendShapeWeightTimeSamplesInInterval(
                interval, &propertyTimes)) {
            _MergeTimeSamples(times, propertyTimes, &tmpTimes);
        }
    }

    // Include time samples that affect the local-to-world transform
    // (necessary because world space transforms are used to push
    //  deformations in skeleton-space back into normal prim space.
    //  See the notes in the deformation methods for more on why.
    if (_GetWorldTransformTimeSamples(prim, interval, &propertyTimes)) {
        _MergeTimeSamples(times, propertyTimes, &tmpTimes);
    }

    if (!skinningQuery.IsRigidlyDeformed() && prim.IsA<UsdGeomPointBased>()) {
        if (UsdGeomPointBased(prim).GetPointsAttr().GetTimeSamplesInInterval(
               interval, &propertyTimes)) {
            _MergeTimeSamples(times, propertyTimes, &tmpTimes);
        }
    }

    // XXX: Skinned meshes are baked at each time sample at which joint
    // transforms are authored. If the joint transforms are authored at sparse
    // time samples, then the resulting skinned meshes will be linearly
    // interpolated on sub-frames. But linearly interpolating skinned meshes is
    // not equivalent to linearly interpolating the driving joints prior to
    // skinning: parts of meshes will undergo smooth rotations in the latter,
    // but never in the former.
    // It's impossible to get a perfect match at every possible sub-frame,
    // but we can at least make sure that the samples are correct when
    // not inspecting sub-frames. In other words, we wish to bake skinned
    // meshes at every time ordinate at which the unbaked meshes would have
    // been viewed.

    // Joint transforms only interpolate inbetween different time samples
    // at which they're authored, so we can limit our sampling range to
    // the min,max range of the samples queried above.
    if (times->size() < 2) {
        // No values to interpolate, so we're done.
        return;
    }
    double rangeStart = times->front();
    double rangeEnd = times->back();
    
    if (_GetScenePlaybackTimeCodesInRange(prim.GetStage(),
                                          rangeStart, rangeEnd,
                                          &propertyTimes)) {
        // Merge these with the time samples of the related properties.
        // The result is to bake deformations both at the sampling rate
        // of the stage, and at any additional sub-frame times that
        // joint transforms are authored at.
        _MergeTimeSamples(times, propertyTimes, &tmpTimes);
    }
}


bool
_BakeSkinnedPoints(const UsdPrim& prim,
                   const UsdSkelSkeletonQuery& skelQuery,
                   const UsdSkelSkinningQuery& skinningQuery,
                   const std::vector<UsdTimeCode>& times,
                   UsdGeomXformCache* xfCache)
{
    const UsdGeomPointBased pointBased(prim);
    if (!pointBased) {
        TF_CODING_ERROR("%s -- Attempted varying deformation of a non "
                        "point-based prim. Skinning currently only understands "
                        "varying deformations on UsdGeomPointBased types.",
                        prim.GetPath().GetText());
        return false;
    }

    const UsdAttribute pointsAttr = pointBased.GetPointsAttr();

    // Pre-sample all point values.    
    std::vector<VtValue> pointsValues(times.size());
    for (size_t i = 0; i < times.size(); ++i) {
        if (!pointsAttr.Get(&pointsValues[i], times[i])) {
            return false;
        }
    }

    const UsdAttribute extentAttr = pointBased.GetExtentAttr();

    // Pre-compute all blend shape offsets/indices.
    const UsdSkelBindingAPI binding(prim);
    const UsdSkelBlendShapeQuery blendShapeQuery(binding);
    // Cache the offsets and point indices of all blend shapes.
    const std::vector<VtUIntArray> blendShapePointIndices =
        blendShapeQuery.ComputeBlendShapePointIndices();
    const std::vector<VtVec3fArray> subShapePointOffsets =
        blendShapeQuery.ComputeSubShapePointOffsets();

    // Compute mapper for remapping blend shape weights.
    UsdSkelAnimMapper blendShapeMapper;
    bool haveBlendShapes = false;

    if (skinningQuery.HasBlendShapes()) {

        // We have bindings for blend shapes, but these only mean
        // something if we have an animation source to provide weight values.
        if (const auto& animQuery = skelQuery.GetAnimQuery()) {
            VtTokenArray blendShapeOrder;
            if (skinningQuery.GetBlendShapesAttr().Get(&blendShapeOrder)) {
                blendShapeMapper =
                    UsdSkelAnimMapper(animQuery.GetBlendShapeOrder(),
                                      blendShapeOrder);
                haveBlendShapes = true;
            }
        }
    }

    for (size_t i = 0; i < times.size(); ++i) {
        const VtValue& points = pointsValues[i];
        if (!points.IsHolding<VtVec3fArray>()) {
            // Could have been a blocked sample. Skip it.
            continue;
        }

        const UsdTimeCode time = times[i];

        TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
            "[UsdSkelBakeSkinning]   Skinning points at time %s "
            "(sample %zu of %zu)\n",
            TfStringify(time).c_str(), i, times.size());

        // XXX: More complete and sophisticated skinning code would compute
        // skinning transforms and blend shape weights once for all prims
        // deformed by a single skeleton, instead of recomputing them for each
        // individual prim skinned.
        // However, since this method is intended only for testing, simplicity
        // and correctness are greater priorities than performance.

        VtMatrix4dArray xforms;
        if (!skelQuery.ComputeSkinningTransforms(&xforms, time)) {
            TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                "[UsdSkelBakeSkinning]   Failed computing "
                "skinning transforms\n");
            return false;
        }

        VtVec3fArray skinnedPoints(points.UncheckedGet<VtVec3fArray>());

        // Apply blend shapes before skinning.
        if (haveBlendShapes) {

            TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                "[UsdSkelBakeSkinning]    Applying blend shapes\n");

            VtFloatArray weights;
            if (!skelQuery.GetAnimQuery().ComputeBlendShapeWeights(
                    &weights, time)) {
                TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                    "[UsdSkelBakeSkinning]    Failed computing "
                    "blend shape weights\n");
                return false;
            }

            // Remap the weights from the order on the animation source
            // to the order of the shapes bound to this skinnable prim.
            VtFloatArray weightsForPrim;
            if (!blendShapeMapper.Remap(weights, &weightsForPrim)) {
                return false;
            }

            // Compute resolved sub-shapes.
            VtFloatArray subShapeWeights;
            VtUIntArray blendShapeIndices, subShapeIndices;
            if (!blendShapeQuery.ComputeSubShapeWeights(
                    weightsForPrim, &subShapeWeights,
                    &blendShapeIndices, &subShapeIndices)) {
                return false;
            }

            if (!blendShapeQuery.ComputeDeformedPoints(
                    subShapeWeights, blendShapeIndices, subShapeIndices,
                    blendShapePointIndices, subShapePointOffsets,
                    skinnedPoints)) {
                return false;
            }
        }
        if (skinningQuery.HasJointInfluences()) {

            TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                "[UsdSkelBakeSkinning]    Applying linear blend skinning\n");

            if (!skinningQuery.ComputeSkinnedPoints(
                    xforms, &skinnedPoints, time)) {
                TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                    "[UsdSkelBakeSkinning]   Failed skinning points\n");
                return false;
            }

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

            GfMatrix4d skelLocalToWorld =
                xfCache->GetLocalToWorldTransform(skelQuery.GetPrim());

            GfMatrix4d skelToGprimXf =
                skelLocalToWorld*gprimLocalToWorld.GetInverse();

            for (auto& pt : skinnedPoints) {
                pt = skelToGprimXf.Transform(pt);
            }
       }

        pointsAttr.Set(skinnedPoints, time);

        // Update point extent.
        VtVec3fArray extent;
        if (UsdGeomBoundable::ComputeExtentFromPlugins(
               pointBased, time, &extent)) {
            extentAttr.Set(extent, time);
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
    if (!xformable) {
        TF_CODING_ERROR("%s -- Attempted rigid deformation of a non-xformable. "
                        "Skinning currently only understands rigid deformations "
                        "on UsdGeomXformable types.",
                        prim.GetPrim().GetPath().GetText());
        return false;
    }

    UsdAttribute xformAttr = xformable.MakeMatrixXform();

    for (size_t i = 0; i < times.size(); ++i) {
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
        if (!skelQuery.ComputeSkinningTransforms(&xforms, time)) {
            TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                "[UsdSkelBakeSkinning]   Failed computing "
                "skinning transforms\n");
            return false;
        }

        GfMatrix4d skinnedXform;
        if (skinningQuery.ComputeSkinnedTransform(xforms, &skinnedXform, time)) {

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

            GfMatrix4d skelLocalToWorld =
                xfCache->GetLocalToWorldTransform(skelQuery.GetPrim());

            GfMatrix4d newLocalXform;
            
            if (xfCache->GetResetXformStack(prim) ||
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
    for (const auto& p : UsdPrimRange(prim)) {
        if (p.IsModel()) {
            UsdGeomModelAPI model(p);
            if (auto attr = model.GetExtentsHintAttr()) {
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

    if (modelsToUpdate.size() > 0) {
        UsdGeomBBoxCache cache(UsdTimeCode(0),
                               UsdGeomImageable::GetOrderedPurposeTokens(),
                               /*useExtentsHint*/ false);

        for (UsdTimeCode time : times) {
            cache.SetTime(time);
            for (auto& model : modelsToUpdate) {
                model.SetExtentsHint(model.ComputeExtentsHint(cache), time);
            }
        }
    }
}


} // namespace


bool
UsdSkelBakeSkinning(const UsdSkelRoot& root, const GfInterval& interval)
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
    if (!skelCache.Populate(root))
        return false;

    // Resolve the skeletal bindings.
    std::vector<UsdSkelBinding> bindings;
    if (!skelCache.ComputeSkelBindings(root, &bindings))
        return false;

    if (bindings.size() == 0) {
        TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
            "[UsdSkelBakeSkinning] No skinnable prims with valid influences "
            "found for <%s>\n", root.GetPrim().GetPath().GetText());
        return true;
    }

    UsdGeomXformCache xfCache;

    // Track the union of time codes samples across all prims.
    std::vector<double> allPrimTimes;
    std::vector<double> tmpTimes;

    for (const UsdSkelBinding& binding : bindings) {

        if (binding.GetSkinningTargets().empty()) {
            // Nothing to do.
            continue;
        }
        
        UsdSkelSkeletonQuery skelQuery =
            skelCache.GetSkelQuery(binding.GetSkeleton());
        if (!TF_VERIFY(skelQuery))
            return false;

        TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
            "[UsdSkelBakeSkinning]: Processing %zu candidate "
            "prims for skinning, using skel <%s>\n",
            binding.GetSkinningTargets().size(),
            binding.GetSkeleton().GetPath().GetText());

        for (const auto& skinningQuery : binding.GetSkinningTargets()) {
            
            const UsdPrim& skinnedPrim = skinningQuery.GetPrim();

            if (!skinningQuery) {   
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
                                    skinningQuery, interval, &times);
            _MergeTimeSamples(&allPrimTimes, times, &tmpTimes);

            // Get times in terms of time codes, so that defaults
            // can be sampled, if necessary.
            std::vector<UsdTimeCode> timeCodes(times.begin(), times.end());
            if (timeCodes.size() == 0) {
                timeCodes.push_back(UsdTimeCode::Default());
            }

            if (!skinningQuery.HasJointInfluences() &&
                !skinningQuery.HasBlendShapes()) {

                TF_DEBUG(USDSKEL_BAKESKINNING).Msg(
                    "   Skipping skinning "
                    "(prim has no joint influences or blend shapes)\n");
                continue;
            }

            if (skinningQuery.IsRigidlyDeformed() &&
                !skinningQuery.HasBlendShapes()) {

                if (!_BakeSkinnedTransform(skinnedPrim, skelQuery,
                                           skinningQuery, timeCodes,
                                           &xfCache)) {
                    return false;
                }
            } else {
                if (!skinnedPrim.IsA<UsdGeomPointBased>()) {
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

                if (!_BakeSkinnedPoints(skinnedPrim, skelQuery,
                                        skinningQuery, timeCodes,
                                        &xfCache)) {
                    return false;
                }
            }
        }
    }

    // Re-define the skel root as a transform.
    // This disables skeletal processing for the scope.
    // (I.e., back to normal mesh land!)
    UsdGeomXform::Define(root.GetPrim().GetStage(), root.GetPrim().GetPath());

    // If any prims are storing extents hints, update the hints now,
    // against the union of all times.
    std::vector<UsdTimeCode> allPrimTimeCodes(allPrimTimes.begin(),
                                              allPrimTimes.end());
    if (allPrimTimeCodes.size() == 0) {
        allPrimTimeCodes.push_back(UsdTimeCode::Default());
    }
    
    _UpdateExtentsHints(root.GetPrim(), allPrimTimeCodes);
    return true;
}


bool
UsdSkelBakeSkinning(const UsdPrimRange& range, const GfInterval& interval)
{
    bool success = true;
    
    for (auto it = range.begin(); it != range.end(); ++it) {
        if (it->IsA<UsdSkelRoot>()) {
            success &= UsdSkelBakeSkinning(UsdSkelRoot(*it), interval);
            it.PruneChildren();
        }
    }
    return success;
}    


PXR_NAMESPACE_CLOSE_SCOPE
