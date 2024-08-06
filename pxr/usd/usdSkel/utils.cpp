//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/utils.h"

#include "pxr/base/arch/hints.h"
#include "pxr/base/gf/range3f.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/dualQuatd.h"
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
        TF_WARN("Size of jointLocalXforms [%zu] != number of joints [%zu]",
                jointLocalXforms.size(), topology.size());
        return false;
    }
    if (ARCH_UNLIKELY(xforms.size() != topology.size())) {
        TF_WARN("Size of xforms [%zu] != number of joints [%zu]",
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
        TF_WARN("Size of xforms [%zu] != number of joints [%zu]",
                xforms.size(), topology.size());
        return false;
    }
    if (ARCH_UNLIKELY(inverseXforms.size() != topology.size())) {
        TF_WARN("Size of inverseXforms [%zu] != number of joints [%zu]",
                inverseXforms.size(), topology.size());
        return false;
    }
    if (ARCH_UNLIKELY(jointLocalXforms.size() != topology.size())) {
        TF_WARN("Size of jointLocalXforms [%zu] != number of joints [%zu]",
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


template <class Matrix4>
bool
_DecomposeTransform(const Matrix4& xform,
                    GfVec3f* translate,
                    Matrix4* rotate,
                    GfVec3h* scale)
{
    // XXX: GfMatrix4x::Factor() may crash if the value isn't properly aligned.
    TF_DEV_AXIOM(size_t(&xform)%alignof(Matrix4) == 0);
    
    // Decomposition must account for handedness changes due to negative scales.
    // This is similar to GfMatrix4d::RemoveScaleShear().
    Matrix4 scaleOrient, perspMat;

    using Vec3 = typename _Vec3MatchingMatrix4<Matrix4>::type;
    
    Vec3 factoredScale, factoredTranslate;;
    if (xform.Factor(&scaleOrient, &factoredScale,
                     rotate, &factoredTranslate, &perspMat)) {

        if (rotate->Orthonormalize()) {
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
    Matrix4 rotateMx;
    if (_DecomposeTransform(xform, translate, &rotateMx, scale)) {
        *rotate = rotateMx.ExtractRotationQuat();
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
    Matrix4 rotateMx;
    if (_DecomposeTransform(xform, translate, &rotateMx, scale)) {
        *rotate = rotateMx.ExtractRotation();
        return true;
    }
    return false;
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
    Matrix4 rotateMx;
    if (_DecomposeTransform(xform, translate, &rotateMx, scale)) {
        *rotate = GfQuatf(rotateMx.ExtractRotationQuat());
        return true;
    }
    return false;
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
        TF_WARN("Size of translations [%zu] != size of xforms [%zu]",
                translations.size(), xforms.size());
        return false;
    }
    if (rotations.size() != xforms.size()) {
        TF_WARN("Size of rotations [%zu] != size of xforms [%zu]",
                rotations.size(), xforms.size());
        return false;
    }
    if (scales.size() != xforms.size()) {
        TF_WARN("Size of scales [%zu] != size of xforms [%zu]",
                scales.size(), xforms.size());
        return false;
    }

    // Flag for marking error state from within threads.
    std::atomic_bool errors(false);

    _ParallelForN(
        xforms.size(), /*inSerial*/ false,
        [&](size_t start, size_t end)
        {
            Matrix4 rotateMx;
            for (size_t i = start; i < end; ++i) {

                if (_DecomposeTransform(xforms[i], &translations[i],
                                        &rotateMx, &scales[i])) {
                    rotations[i] = GfQuatf(rotateMx.ExtractRotationQuat());
                } else {
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
        TF_WARN("Size of translations [%zu] != size of xforms [%zu]",
                translations.size(), xforms.size());
        return false;
    }
    if (ARCH_UNLIKELY(rotations.size() != xforms.size())) {
        TF_WARN("Size of rotations [%zu] != size of xforms [%zu]",
                rotations.size(), xforms.size());
        return false;
    }
    if (ARCH_UNLIKELY(scales.size() != xforms.size())) {
        TF_WARN("Size of scales [%zu] != size of xforms [%zu]",
                scales.size(), xforms.size());
        return false;
    }

    for (size_t i = 0; i < xforms.size(); ++i) {
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
                          GfVec3f(rootXform->TransformAffine(pivot)) : pivot);
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
_ValidateArrayShape(size_t size, int numInfluencesPerComponent)
{
    if (numInfluencesPerComponent > 0) {
        if (size%numInfluencesPerComponent == 0) {
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

} // namespace


bool
UsdSkelNormalizeWeights(TfSpan<float> weights,
                        int numInfluencesPerComponent,
                        float eps)
{
    TRACE_FUNCTION();
    
    if (!_ValidateArrayShape(weights.size(), numInfluencesPerComponent)) {
        return false;
    }

    const size_t numComponents = weights.size()/numInfluencesPerComponent;

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

                if (std::abs(sum) > eps) {
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
        TF_WARN("Size of 'indices' [%zu] != size of 'weights' [%zu].",
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

    const size_t numComponents = indices.size()/numInfluencesPerComponent;

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


bool
UsdSkelInterleaveInfluences(const TfSpan<const int>& indices,
                            const TfSpan<const float>& weights,
                            TfSpan<GfVec2f> interleavedInfluences)
{
    TRACE_FUNCTION();

    if (weights.size() != indices.size()) {
        TF_WARN("Size of weights [%zu] != size of indices [%zu]",
                weights.size(), indices.size());
        return false;
    }

    if (interleavedInfluences.size() != indices.size()) {
        TF_WARN("Size of interleavedInfluences [%zu] != size of indices [%zu]",
                interleavedInfluences.size(), indices.size());
        return false;
    }

    for (size_t i = 0; i < indices.size(); ++i) {
        interleavedInfluences[i] =
            GfVec2f(static_cast<float>(indices[i]), weights[i]);
    }
    return true;
}


namespace {


/// Functor for extracting influence indices and weights from influences
/// stored on separate index and weight arrays.
struct _NonInterleavedInfluencesFn {
    TfSpan<const int> indices;
    TfSpan<const float> weights;

    int         GetIndex(size_t index) const { return indices[index]; }
    float       GetWeight(size_t index) const { return weights[index]; }
    size_t      size() const { return indices.size(); }
};


/// Functor for extracting influence indices and weights from
/// interleaved influences, stored as an array of (index,weight) vectors.
struct _InterleavedInfluencesFn {
    TfSpan<const GfVec2f> influences;

    int         GetIndex(size_t index) const
                { return static_cast<int>(influences[index][0]); }

    float       GetWeight(size_t index) const { return influences[index][1]; }

    size_t      size() const { return influences.size(); }
};

/// Functor which returns the element index unchanged.
/// Use when the attribute to deform has vertex or varying interpolation.
struct _IdentityPointIndexFn
{
    size_t GetPointIndex(size_t index) const { return index; }
};

/// Functor which uses the faceVertexIndices attribute to find the
/// corresponding point index. Use for deforming faceVarying normals.
struct _FaceVaryingPointIndexFn
{
    TfSpan<const int> faceVertexIndices;
    const int numPoints;

    size_t GetPointIndex(size_t index) const
    {
        const int pointIndex = faceVertexIndices[index];
        if (pointIndex < 0 || pointIndex >= numPoints)
        {
            TF_WARN("faceVertexIndices is out of range [%d] at index [%zu]",
                    pointIndex, index);
            return 0;
        }

        return pointIndex;
    }
};

template <typename Matrix4, typename Matrix3>
void
_ConvertToDualQuaternions(TfSpan<const Matrix4> jointXforms,
                          TfSpan<GfDualQuatd> jointDualQuats,
                          TfSpan<Matrix3> jointScales,
                          bool *hasJointScale=nullptr)
{
    TF_DEV_AXIOM(jointXforms.size() == jointDualQuats.size());
    TF_DEV_AXIOM(jointXforms.size() == jointScales.size());

    GfMatrix4d scaleOrientMat, factoredRotMat, perspMat;
    GfVec3d scale, translation;

    // when given, set *hasJointScale to true if any jointScales is not identity
    if (hasJointScale) {
        *hasJointScale = false;
    }

    for (size_t ji = 0; ji < jointXforms.size(); ++ji) {
        const GfMatrix4d matrix = GfMatrix4d(jointXforms[ji]);

        if (!matrix.Factor(&scaleOrientMat, &scale, &factoredRotMat,
                           &translation, &perspMat)) {
            // unable to decompose, set to zero
            jointDualQuats[ji] = GfDualQuatd::GetZero();
            jointScales[ji] = Matrix3(1);
            continue;
        }

        // Remove shear & extract rotation
        factoredRotMat.Orthonormalize();
        const GfQuaternion rotationQ = factoredRotMat.ExtractRotationMatrix()
            .ExtractRotationQuaternion();

        // Construct dual quaternion from rotation & translation
        jointDualQuats[ji] =
            GfDualQuatd(GfQuatd(rotationQ.GetReal(), rotationQ.GetImaginary()),
                        translation);

        // Calculate jointScales by removing jointDualQuats from jointXforms
        const GfMatrix4d tmpNonScaleXform =
            factoredRotMat * GfMatrix4d(1.0).SetTranslate(translation);
        // Extract the upper-left 3x3 matrix;
        jointScales[ji] = Matrix3((matrix * tmpNonScaleXform.GetInverse()).
                                  ExtractRotationMatrix());

        // If jointScales[ji] is not an identity matrix, need to set the flag
        if (hasJointScale && !*hasJointScale &&
            !GfIsClose(jointScales[ji], Matrix3(1), 1e-6)) {
            *hasJointScale = true;
        }
    }
}


template <typename InfluenceFn>
int
_GetPivotJointIndex(const size_t pointIdx,
                    const size_t jointArraySize,
                    const InfluenceFn& influenceFn,
                    const int numInfluencesPerPoint)
{
    // pivot joint index is set to the joint with the max influence/weight
    int pivotIdx = -1;
    float maxw = -1;
    for (int wi = 0; wi < numInfluencesPerPoint; ++wi) {
        const size_t influenceIdx = pointIdx*numInfluencesPerPoint + wi;
        const int jointIdx = influenceFn.GetIndex(influenceIdx);

        if (jointIdx >= 0 && static_cast<size_t>(jointIdx) < jointArraySize) {

            const float w = influenceFn.GetWeight(influenceIdx);
            if (pivotIdx < 0 || maxw < w) {
                maxw = w;
                pivotIdx = jointIdx;
            }
        }
    }

    return pivotIdx;
}


template <typename Matrix4, typename InfluenceFn>
bool
_SkinPointsLBS(const Matrix4& geomBindTransform,
               TfSpan<const Matrix4> jointXforms,
               const InfluenceFn& influenceFn,
               const int numInfluencesPerPoint,
               TfSpan<GfVec3f> points,
               const bool inSerial)
{
    TRACE_FUNCTION();

    // Flag for marking error state from within threads.
    std::atomic_bool errors(false);

    _ParallelForN(
        points.size(), /* inSerial = */ inSerial,
        [&](size_t start, size_t end)
        {
            for (size_t pi = start; pi < end; ++pi) {

                const GfVec3f initialP(geomBindTransform.Transform(points[pi]));
                GfVec3f p(0,0,0);
                
                for (int wi = 0; wi < numInfluencesPerPoint; ++wi) {
                    const size_t influenceIdx = pi*numInfluencesPerPoint + wi;
                    const int jointIdx = influenceFn.GetIndex(influenceIdx);

                    if (jointIdx >= 0 &&
                       static_cast<size_t>(jointIdx) < jointXforms.size()) {

                        const float w = influenceFn.GetWeight(influenceIdx);
                        if (w != 0.0f) {
                            // Since joint transforms are encoded in terms of
                            // t,r,s components, it shouldn't be possible to
                            // encode non-affine transforms, except for the rest
                            // pose (which, according to the schema, should
                            // be affine!). Safe to assume affine transforms.
                            p += GfVec3f(jointXforms[jointIdx].TransformAffine(
                                             initialP))*w;
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
                                " (num joints = %zu).",
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


template <typename Matrix4, typename InfluenceFn>
bool
_SkinPointsDQS(const Matrix4& geomBindTransform,
               TfSpan<const Matrix4> jointXforms,
               const InfluenceFn& influenceFn,
               const int numInfluencesPerPoint,
               TfSpan<GfVec3f> points,
               const bool inSerial)
{
    TRACE_FUNCTION();

    // Flag for marking error state from within threads.
    std::atomic_bool errors(false);

    // Convert joint transformation matrices to dual quaternions
    std::vector<GfDualQuatd> jointDualQuats(jointXforms.size());
    std::vector<GfMatrix3f> jointScales(jointXforms.size());
    bool hasJointScale(false);
    _ConvertToDualQuaternions(jointXforms,
                              TfSpan<GfDualQuatd>(jointDualQuats),
                              TfSpan<GfMatrix3f>(jointScales),
                              &hasJointScale);

    _ParallelForN(
        points.size(), /* inSerial = */ inSerial,
        [&](size_t start, size_t end)
        {
            for (size_t pi = start; pi < end; ++pi) {

                const GfVec3f initialP(geomBindTransform.Transform(points[pi]));
                GfVec3f scaledP(0, 0, 0);

                GfQuatd pivotQuat(0.0);
                const int pivotIdx =
                    _GetPivotJointIndex(pi, jointDualQuats.size(),
                                        influenceFn, numInfluencesPerPoint);
                if (pivotIdx >= 0)
                    pivotQuat = jointDualQuats[pivotIdx].GetReal();

                GfDualQuatd weightedSumDQ = GfDualQuatd::GetZero();

                for (int wi = 0; wi < numInfluencesPerPoint; ++wi) {
                    const size_t influenceIdx = pi*numInfluencesPerPoint + wi;
                    const int jointIdx = influenceFn.GetIndex(influenceIdx);

                    if (jointIdx >= 0 &&
                       static_cast<size_t>(jointIdx) < jointDualQuats.size()) {

                        float w = influenceFn.GetWeight(influenceIdx);
                        if (w != 0.0f) {
                            // Since joint transforms are encoded in terms of
                            // t,r,s components, it shouldn't be possible to
                            // encode non-affine transforms, except for the rest
                            // pose (which, according to the schema, should
                            // be affine!). Safe to assume affine transforms.

                            // Apply scale using LBS, if any of jointScales is not identity
                            if (hasJointScale) {
                                scaledP += (initialP * jointScales[jointIdx]) * w;
                            }

                            // Apply rotation & translation using DQS
                            const GfDualQuatd &jointDQ = jointDualQuats[jointIdx];
                            // Flip the dual quaternion, if necessary, to make it
                            // on the same hemisphere as the pivotQuat.
                            if ( GfDot(jointDQ.GetReal(), pivotQuat) < 0.0 )
                                w = -w;
                            weightedSumDQ += (jointDQ * w);
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
                                " (num joints = %zu).",
                                jointIdx, influenceIdx, jointDualQuats.size());
                        errors = true;
                        return;
                    }
                }
                if (!hasJointScale) {
                    scaledP = initialP;
                }

                weightedSumDQ.Normalize();
                points[pi] = GfVec3f(weightedSumDQ.Transform(scaledP));
            }
        }
        );

    return !errors;
}


template <typename Matrix4>
bool
_InterleavedSkinPoints(const TfToken& skinningMethod,
                       const Matrix4& geomBindTransform,
                       TfSpan<const Matrix4> jointXforms,
                       TfSpan<const GfVec2f> influences,
                       const int numInfluencesPerPoint,
                       TfSpan<GfVec3f> points,
                       const bool inSerial)
{
    if (influences.size() != (points.size()*numInfluencesPerPoint)) {
        TF_WARN("Size of influences [%zu] != "
                "(points.size() [%zu] * numInfluencesPerPoint [%d]).",
                influences.size(), points.size(), numInfluencesPerPoint);
        return false;
    }

    const _InterleavedInfluencesFn influenceFn{influences};
    if (skinningMethod == UsdSkelTokens->classicLinear) {
        return _SkinPointsLBS(geomBindTransform, jointXforms, influenceFn,
                              numInfluencesPerPoint, points, inSerial);
    } else if (skinningMethod == UsdSkelTokens->dualQuaternion) {
        return _SkinPointsDQS(geomBindTransform, jointXforms, influenceFn,
                              numInfluencesPerPoint, points, inSerial);
    } else {
        TF_WARN("Unknown skinning method: '%s' ", skinningMethod.GetText());
        return false;
    }
}


template <typename Matrix4>
bool
_NonInterleavedSkinPoints(const TfToken& skinningMethod,
                          const Matrix4& geomBindTransform,
                          TfSpan<const Matrix4> jointXforms,
                          TfSpan<const int> jointIndices,
                          TfSpan<const float> jointWeights,
                          const int numInfluencesPerPoint,
                          TfSpan<GfVec3f> points,
                          const bool inSerial)
{
    if (jointIndices.size() != jointWeights.size()) {
        TF_WARN("Size of jointIndices [%zu] != size of jointWeights [%zu]",
                jointIndices.size(), jointWeights.size());
        return false;
    }
    
    if (jointIndices.size() != (points.size()*numInfluencesPerPoint)) {
        TF_WARN("Size of jointIndices [%zu] != "
                "(points.size() [%zu] * numInfluencesPerPoint [%d]).",
                jointIndices.size(), points.size(), numInfluencesPerPoint);
        return false;
    }

    const _NonInterleavedInfluencesFn influenceFn{jointIndices, jointWeights};
    if (skinningMethod == UsdSkelTokens->classicLinear) {
        return _SkinPointsLBS(geomBindTransform, jointXforms, influenceFn,
                              numInfluencesPerPoint, points, inSerial);
    } else if (skinningMethod == UsdSkelTokens->dualQuaternion) {
        return _SkinPointsDQS(geomBindTransform, jointXforms, influenceFn,
                              numInfluencesPerPoint, points, inSerial);
    } else {
        TF_WARN("Unknown skinning method: '%s' ", skinningMethod.GetText());
        return false;
    }
}


} // namespace


bool
UsdSkelSkinPoints(const TfToken& skinningMethod,
                  const GfMatrix4d& geomBindTransform,
                  TfSpan<const GfMatrix4d> jointXforms,
                  TfSpan<const int> jointIndices,
                  TfSpan<const float> jointWeights,
                  const int numInfluencesPerPoint,
                  TfSpan<GfVec3f> points,
                  const bool inSerial)
{
    return _NonInterleavedSkinPoints(
        skinningMethod, geomBindTransform, jointXforms,
        jointIndices, jointWeights, numInfluencesPerPoint,
        points, inSerial);
}


bool
UsdSkelSkinPoints(const TfToken& skinningMethod,
                  const GfMatrix4f& geomBindTransform,
                  TfSpan<const GfMatrix4f> jointXforms,
                  TfSpan<const int> jointIndices,
                  TfSpan<const float> jointWeights,
                  const int numInfluencesPerPoint,
                  TfSpan<GfVec3f> points,
                  const bool inSerial)
{
    return _NonInterleavedSkinPoints(
        skinningMethod, geomBindTransform, jointXforms,
        jointIndices, jointWeights, numInfluencesPerPoint,
        points, inSerial);
}


bool
UsdSkelSkinPoints(const TfToken& skinningMethod,
                  const GfMatrix4d& geomBindTransform,
                  TfSpan<const GfMatrix4d> jointXforms,
                  TfSpan<const GfVec2f> influences,
                  const int numInfluencesPerPoint,
                  TfSpan<GfVec3f> points,
                  const bool inSerial)
{
    return _InterleavedSkinPoints(skinningMethod, geomBindTransform, jointXforms,
                                  influences, numInfluencesPerPoint,
                                  points, inSerial);
}


bool
UsdSkelSkinPoints(const TfToken& skinningMethod,
                  const GfMatrix4f& geomBindTransform,
                  TfSpan<const GfMatrix4f> jointXforms,
                  TfSpan<const GfVec2f> influences,
                  const int numInfluencesPerPoint,
                  TfSpan<GfVec3f> points,
                  const bool inSerial)
{
    return _InterleavedSkinPoints(skinningMethod, geomBindTransform, jointXforms,
                                  influences, numInfluencesPerPoint,
                                  points, inSerial);
}


// deprecated
bool
UsdSkelSkinPoints(const TfToken& skinningMethod,
                  const GfMatrix4d& geomBindTransform,
                  const GfMatrix4d* jointXforms,
                  const size_t numJoints,
                  const int* jointIndices,
                  const float* jointWeights,
                  const size_t numInfluences,
                  const int numInfluencesPerPoint,
                  GfVec3f* points,
                  const size_t numPoints,
                  const bool inSerial)
{
    return UsdSkelSkinPoints(
        skinningMethod, geomBindTransform,
        TfSpan<const GfMatrix4d>(jointXforms, numJoints),
        TfSpan<const int>(jointIndices, numInfluences),
        TfSpan<const float>(jointWeights, numInfluences),
        numInfluencesPerPoint,
        TfSpan<GfVec3f>(points, numPoints),
        inSerial);
}


// deprecated
bool
UsdSkelSkinPoints(const TfToken& skinningMethod,
                  const GfMatrix4d& geomBindTransform,
                  const VtMatrix4dArray& jointXforms,
                  const VtIntArray& jointIndices,
                  const VtFloatArray& jointWeights,
                  const int numInfluencesPerPoint,
                  VtVec3fArray* points)
{
    if (!points) {
        TF_CODING_ERROR("'points' pointer is null.");
        return false;
    }

    return UsdSkelSkinPoints(
        skinningMethod, geomBindTransform, jointXforms,
        jointIndices, jointWeights,
        numInfluencesPerPoint, *points);
}


bool
UsdSkelSkinPointsLBS(const GfMatrix4d& geomBindTransform,
                     TfSpan<const GfMatrix4d> jointXforms,
                     TfSpan<const int> jointIndices,
                     TfSpan<const float> jointWeights,
                     const int numInfluencesPerPoint,
                     TfSpan<GfVec3f> points,
                     const bool inSerial)
{
    return UsdSkelSkinPoints(UsdSkelTokens->classicLinear,
                             geomBindTransform,
                             jointXforms,
                             jointIndices,
                             jointWeights,
                             numInfluencesPerPoint,
                             points,
                             inSerial);
}


bool
UsdSkelSkinPointsLBS(const GfMatrix4f& geomBindTransform,
                     TfSpan<const GfMatrix4f> jointXforms,
                     TfSpan<const int> jointIndices,
                     TfSpan<const float> jointWeights,
                     const int numInfluencesPerPoint,
                     TfSpan<GfVec3f> points,
                     const bool inSerial)
{
    return UsdSkelSkinPoints(UsdSkelTokens->classicLinear,
                             geomBindTransform,
                             jointXforms,
                             jointIndices,
                             jointWeights,
                             numInfluencesPerPoint,
                             points,
                             inSerial);
}


bool
UsdSkelSkinPointsLBS(const GfMatrix4d& geomBindTransform,
                     TfSpan<const GfMatrix4d> jointXforms,
                     TfSpan<const GfVec2f> influences,
                     const int numInfluencesPerPoint,
                     TfSpan<GfVec3f> points,
                     const bool inSerial)
{
    return UsdSkelSkinPoints(UsdSkelTokens->classicLinear,
                             geomBindTransform,
                             jointXforms,
                             influences,
                             numInfluencesPerPoint,
                             points,
                             inSerial);
}


bool
UsdSkelSkinPointsLBS(const GfMatrix4f& geomBindTransform,
                     TfSpan<const GfMatrix4f> jointXforms,
                     TfSpan<const GfVec2f> influences,
                     const int numInfluencesPerPoint,
                     TfSpan<GfVec3f> points,
                     const bool inSerial)
{
    return UsdSkelSkinPoints(UsdSkelTokens->classicLinear,
                             geomBindTransform,
                             jointXforms,
                             influences,
                             numInfluencesPerPoint,
                             points,
                             inSerial);
}


// deprecated
bool
UsdSkelSkinPointsLBS(const GfMatrix4d& geomBindTransform,
                     const GfMatrix4d* jointXforms,
                     const size_t numJoints,
                     const int* jointIndices,
                     const float* jointWeights,
                     const size_t numInfluences,
                     const int numInfluencesPerPoint,
                     GfVec3f* points,
                     const size_t numPoints,
                     const bool inSerial)
{
    return UsdSkelSkinPoints(UsdSkelTokens->classicLinear,
                             geomBindTransform,
                             jointXforms,
                             numJoints,
                             jointIndices,
                             jointWeights,
                             numInfluences,
                             numInfluencesPerPoint,
                             points,
                             numPoints,
                             inSerial);
}


// deprecated
bool
UsdSkelSkinPointsLBS(const GfMatrix4d& geomBindTransform,
                     const VtMatrix4dArray& jointXforms,
                     const VtIntArray& jointIndices,
                     const VtFloatArray& jointWeights,
                     const int numInfluencesPerPoint,
                     VtVec3fArray* points)
{
    return UsdSkelSkinPoints(UsdSkelTokens->classicLinear,
                             geomBindTransform,
                             jointXforms,
                             jointIndices,
                             jointWeights,
                             numInfluencesPerPoint,
                             points);
}


namespace {


template <typename Matrix3>
void
_ConvertToQuaternions(TfSpan<const Matrix3> jointXforms,
                      TfSpan<GfQuatd> jointQuats,
                      TfSpan<Matrix3> jointScales,
                      bool *hasJointScale=nullptr)
{
    TF_DEV_AXIOM(jointXforms.size() == jointQuats.size());
    TF_DEV_AXIOM(jointXforms.size() == jointScales.size());

    // when given, set *hasJointScale to true if any jointScales is not identity
    if (hasJointScale) {
        *hasJointScale = false;
    }

    for (size_t ji = 0; ji < jointXforms.size(); ++ji) {
        const GfMatrix3d matrix = GfMatrix3d(jointXforms[ji]);
        const GfMatrix3d rotationMat = matrix.GetOrthonormalized();
        const GfQuaternion rotationQ = rotationMat.ExtractRotationQuaternion();
        jointQuats[ji] = GfQuatd(rotationQ.GetReal(),
                                 rotationQ.GetImaginary());
        jointScales[ji] = Matrix3(matrix * rotationMat.GetInverse());

        // If jointScales[ji] is not an identity matrix, need to set the flag
        if (hasJointScale && !*hasJointScale &&
            !GfIsClose(jointScales[ji], Matrix3(1), 1e-6)) {
            *hasJointScale = true;
        }
    }
}


template <typename Matrix3,
          typename InfluenceFn,
          typename PointIndexFn>
bool
_SkinNormalsLBS(const Matrix3& geomBindTransform,
                TfSpan<const Matrix3> jointXforms,
                const InfluenceFn& influenceFn,
                const PointIndexFn& pointIndexFn,
                const int numInfluencesPerPoint,
                TfSpan<GfVec3f> normals,
                const bool inSerial)
{
    TRACE_FUNCTION();

    // Flag for marking error state from within threads.
    std::atomic_bool errors(false);

    _ParallelForN(
        normals.size(), inSerial,
        [&](size_t start, size_t end)
        {   
            // XXX: We skin normals by summing the weighted normals as posed
            // for each influence, in the same manner as point skinning.
            // This is a very common, though flawed approach. There are more
            // accurate algorithms for skinning normals that should be
            // considered in the future (E.g, Accurate and Efficient
            // Lighting for Skinned Models, Tarini, et. al.)
            
            for (size_t ni = start; ni < end; ++ni) {
                
                const GfVec3f initialN(normals[ni]*geomBindTransform);
                // Determine the point to read the influences from. This is not
                // the same as the normal's index if there is faceVarying
                // interpolation.
                const size_t pi = pointIndexFn.GetPointIndex(ni);

                GfVec3f n(0,0,0);
                
                for (int wi = 0; wi < numInfluencesPerPoint; ++wi) {
                    const size_t influenceIdx = pi*numInfluencesPerPoint + wi;
                    const int jointIdx = influenceFn.GetIndex(influenceIdx);

                    if (jointIdx >= 0 &&
                        static_cast<size_t>(jointIdx) < jointXforms.size()) {

                        const float w = influenceFn.GetWeight(influenceIdx);
                        if (w != 0.0f) {
                            n += GfVec3f(initialN*jointXforms[jointIdx])*w;
                        }
                    } else {
                        // XXX: Generally, if one joint index is bad, an asset
                        // has probably gotten out of sync, and probably many
                        // other indices will be invalid, too.
                        // We could attempt to continue silently, but would
                        // likely end up with scrambled normals.
                        // Bail out early.

                        TF_WARN("Out of range joint index %d at index %zu"
                                " (num joints = %zu).",
                                jointIdx, influenceIdx, jointXforms.size());
                        errors = true;
                        return;
                    }
                }
                normals[ni] = n.GetNormalized();
            }
        });
        
    return !errors;
}


template <typename Matrix3,
          typename InfluenceFn,
          typename PointIndexFn>
bool
_SkinNormalsDQS(const Matrix3& geomBindTransform,
                TfSpan<const Matrix3> jointXforms,
                const InfluenceFn& influenceFn,
                const PointIndexFn& pointIndexFn,
                const int numInfluencesPerPoint,
                TfSpan<GfVec3f> normals,
                const bool inSerial)
{
    TRACE_FUNCTION();

    // Flag for marking error state from within threads.
    std::atomic_bool errors(false);

    // Convert joint rotation matrices to quaternions
    std::vector<GfQuatd> jointQuats(jointXforms.size());
    std::vector<Matrix3> jointScales(jointXforms.size());
    bool hasJointScale(false);
    _ConvertToQuaternions(jointXforms,
                          TfSpan<GfQuatd>(jointQuats),
                          TfSpan<Matrix3>(jointScales),
                          &hasJointScale);

    _ParallelForN(
        normals.size(), inSerial,
        [&](size_t start, size_t end)
        {
            // XXX: We skin normals by summing the weighted normals as posed
            // for each influence, in the same manner as point skinning.
            // This is a very common, though flawed approach. There are more
            // accurate algorithms for skinning normals that should be
            // considered in the future (E.g, Accurate and Efficient
            // Lighting for Skinned Models, Tarini, et. al.)

            for (size_t ni = start; ni < end; ++ni) {

                const GfVec3f initialN(normals[ni]*geomBindTransform);
                // Determine the point to read the influences from. This is not
                // the same as the normal's index if there is faceVarying
                // interpolation.
                const size_t pi = pointIndexFn.GetPointIndex(ni);

                // Find pivot quaternion (with max influence)
                GfQuatd pivotQuat(0.0);
                const int pivotIdx =
                    _GetPivotJointIndex(pi, jointQuats.size(),
                                        influenceFn, numInfluencesPerPoint);
                if (pivotIdx >= 0)
                    pivotQuat = jointQuats[pivotIdx];

                GfVec3f scaledN(0, 0, 0);
                GfQuatd weightedSumQuat = GfQuatd::GetZero();

                for (int wi = 0; wi < numInfluencesPerPoint; ++wi) {
                    const size_t influenceIdx = pi*numInfluencesPerPoint + wi;
                    const int jointIdx = influenceFn.GetIndex(influenceIdx);

                    if (jointIdx >= 0 &&
                        static_cast<size_t>(jointIdx) < jointQuats.size()) {

                        float w = influenceFn.GetWeight(influenceIdx);
                        if (w != 0.0f) {
                            // Apply scale using LBS, if any of jointScales is not identity
                            if (hasJointScale) {
                                scaledN +=
                                    GfVec3f(initialN*jointScales[jointIdx])*w;
                            }

                            // Apply rotation using DQS
                            const GfQuatd &jointQuat = jointQuats[jointIdx];
                            // Flip the quaternion, if necessary, to make it
                            // on the same hemisphere as the pivotQuat.
                            if ( GfDot(jointQuat, pivotQuat) < 0.0 )
                                w = -w;
                            weightedSumQuat += (jointQuat * w);
                        }
                    } else {
                        // XXX: Generally, if one joint index is bad, an asset
                        // has probably gotten out of sync, and probably many
                        // other indices will be invalid, too.
                        // We could attempt to continue silently, but would
                        // likely end up with scrambled normals.
                        // Bail out early.

                        TF_WARN("Out of range joint index %d at index %zu"
                                " (num joints = %zu).",
                                jointIdx, influenceIdx, jointQuats.size());
                        errors = true;
                        return;
                    }
                }
                if (!hasJointScale) {
                    scaledN = initialN;
                }

                weightedSumQuat.Normalize();
                normals[ni] = GfVec3f(weightedSumQuat.Transform(scaledN).GetNormalized());
            }
        });

    return !errors;
}


template <typename Matrix3>
bool
_InterleavedSkinNormals(const TfToken& skinningMethod,
                        const Matrix3& geomBindTransform,
                        TfSpan<const Matrix3> jointXforms,
                        TfSpan<const GfVec2f> influences,
                        const int numInfluencesPerPoint,
                        TfSpan<GfVec3f> normals,
                        const bool inSerial)
{
    if (influences.size() != (normals.size()*numInfluencesPerPoint)) {
        TF_WARN("Size of influences [%zu] != "
                "(normals.size() [%zu] * numInfluencesPerPoint [%d]).",
                influences.size(), normals.size(), numInfluencesPerPoint);
        return false;
    }

    const _InterleavedInfluencesFn influenceFn{influences};
    if (skinningMethod == UsdSkelTokens->classicLinear) {
        return _SkinNormalsLBS(geomBindTransform, jointXforms, influenceFn,
                               _IdentityPointIndexFn(), numInfluencesPerPoint,
                               normals, inSerial);
    } else if (skinningMethod == UsdSkelTokens->dualQuaternion) {
        return _SkinNormalsDQS(geomBindTransform, jointXforms, influenceFn,
                               _IdentityPointIndexFn(), numInfluencesPerPoint,
                               normals, inSerial);
    } else {
        TF_WARN("Unknown skinning method: '%s' ", skinningMethod.GetText());
        return false;
    }
}


template <typename Matrix3>
bool
_NonInterleavedSkinNormals(const TfToken& skinningMethod,
                           const Matrix3& geomBindTransform,
                           TfSpan<const Matrix3> jointXforms,
                           TfSpan<const int> jointIndices,
                           TfSpan<const float> jointWeights,
                           const int numInfluencesPerPoint,
                           TfSpan<GfVec3f> normals,
                           const bool inSerial)
{
    if (jointIndices.size() != jointWeights.size()) {
        TF_WARN("Size of jointIndices [%zu] != size of jointWeights [%zu]",
                jointIndices.size(), jointWeights.size());
        return false;
    }
    
    if (jointIndices.size() != (normals.size()*numInfluencesPerPoint)) {
        TF_WARN("Size of jointIndices [%zu] != "
                "(normals.size() [%zu] * numInfluencesPerPoint [%d]).",
                jointIndices.size(), normals.size(), numInfluencesPerPoint);
        return false;
    }

    const _NonInterleavedInfluencesFn influenceFn{jointIndices, jointWeights};
    if (skinningMethod == UsdSkelTokens->classicLinear) {
        return _SkinNormalsLBS(geomBindTransform, jointXforms, influenceFn,
                               _IdentityPointIndexFn(), numInfluencesPerPoint,
                               normals, inSerial);
    } else if (skinningMethod == UsdSkelTokens->dualQuaternion) {
        return _SkinNormalsDQS(geomBindTransform, jointXforms, influenceFn,
                               _IdentityPointIndexFn(), numInfluencesPerPoint,
                               normals, inSerial);
    } else {
        TF_WARN("Unknown skinning method: '%s' ", skinningMethod.GetText());
        return false;
    }

}


template <typename Matrix3>
bool
_SkinFaceVaryingNormals(const TfToken& skinningMethod,
                        const Matrix3& geomBindTransform,
                        TfSpan<const Matrix3> jointXforms,
                        TfSpan<const int> jointIndices,
                        TfSpan<const float> jointWeights,
                        const int numInfluencesPerPoint,
                        TfSpan<const int> faceVertexIndices,
                        TfSpan<GfVec3f> normals,
                        const bool inSerial)
{
    if (jointIndices.size() != jointWeights.size()) {
        TF_WARN("Size of jointIndices [%zu] != size of jointWeights [%zu]",
                jointIndices.size(), jointWeights.size());
        return false;
    }

    if (jointIndices.size() % numInfluencesPerPoint != 0) {
        TF_WARN("Size of jointIndices [%zu] is not a multiple of "
                "numInfluencesPerPoint [%d]",
                jointIndices.size(), numInfluencesPerPoint);
        return false;
    }

    if (faceVertexIndices.size() != normals.size()) {
        TF_WARN("Size of faceVertexIndices [%zu] != size of normals [%zu]",
                faceVertexIndices.size(), normals.size());
        return false;
    }

    const _NonInterleavedInfluencesFn influenceFn{jointIndices, jointWeights};

    const int numPoints = jointIndices.size() / numInfluencesPerPoint;
    const _FaceVaryingPointIndexFn indexFn{faceVertexIndices, numPoints};

    if (skinningMethod == UsdSkelTokens->classicLinear) {
        return _SkinNormalsLBS(geomBindTransform, jointXforms, influenceFn, indexFn,
                               numInfluencesPerPoint, normals, inSerial);
    } else if (skinningMethod == UsdSkelTokens->dualQuaternion) {
        return _SkinNormalsDQS(geomBindTransform, jointXforms, influenceFn, indexFn,
                               numInfluencesPerPoint, normals, inSerial);
    } else {
        TF_WARN("Unknown skinning method: '%s' ", skinningMethod.GetText());
        return false;
    }
}


} // namespace


bool
UsdSkelSkinNormals(const TfToken& skinningMethod,
                   const GfMatrix3d& geomBindTransform,
                   TfSpan<const GfMatrix3d> jointXforms,
                   TfSpan<const int> jointIndices,
                   TfSpan<const float> jointWeights,
                   const int numInfluencesPerPoint,
                   TfSpan<GfVec3f> normals,
                   const bool inSerial)
{
    return _NonInterleavedSkinNormals(
        skinningMethod, geomBindTransform, jointXforms,
        jointIndices, jointWeights, numInfluencesPerPoint,
        normals, inSerial);
}


bool
UsdSkelSkinNormals(const TfToken& skinningMethod,
                   const GfMatrix3f& geomBindTransform,
                   TfSpan<const GfMatrix3f> jointXforms,
                   TfSpan<const int> jointIndices,
                   TfSpan<const float> jointWeights,
                   const int numInfluencesPerPoint,
                   TfSpan<GfVec3f> normals,
                   const bool inSerial)
{
    return _NonInterleavedSkinNormals(
        skinningMethod, geomBindTransform, jointXforms,
        jointIndices, jointWeights, numInfluencesPerPoint,
        normals, inSerial);
}


bool
UsdSkelSkinNormals(const TfToken& skinningMethod,
                   const GfMatrix3d& geomBindTransform,
                   TfSpan<const GfMatrix3d> jointXforms,
                   TfSpan<const GfVec2f> influences,
                   const int numInfluencesPerPoint,
                   TfSpan<GfVec3f> points,
                   const bool inSerial)
{
    return _InterleavedSkinNormals(skinningMethod, geomBindTransform, jointXforms,
                                   influences, numInfluencesPerPoint,
                                   points, inSerial);
}


bool
UsdSkelSkinNormals(const TfToken& skinningMethod,
                   const GfMatrix3f& geomBindTransform,
                   TfSpan<const GfMatrix3f> jointXforms,
                   TfSpan<const GfVec2f> influences,
                   const int numInfluencesPerPoint,
                   TfSpan<GfVec3f> points,
                   const bool inSerial)
{
    return _InterleavedSkinNormals(skinningMethod, geomBindTransform, jointXforms,
                                   influences, numInfluencesPerPoint,
                                   points, inSerial);
}


bool
UsdSkelSkinFaceVaryingNormals(const TfToken& skinningMethod,
                              const GfMatrix3d& geomBindTransform,
                              TfSpan<const GfMatrix3d> jointXforms,
                              TfSpan<const int> jointIndices,
                              TfSpan<const float> jointWeights,
                              const int numInfluencesPerPoint,
                              TfSpan<const int> faceVertexIndices,
                              TfSpan<GfVec3f> normals,
                              const bool inSerial)
{
    return _SkinFaceVaryingNormals(
        skinningMethod, geomBindTransform, jointXforms, jointIndices, jointWeights,
        numInfluencesPerPoint, faceVertexIndices, normals, inSerial);
}


bool
UsdSkelSkinFaceVaryingNormals(const TfToken& skinningMethod,
                              const GfMatrix3f& geomBindTransform,
                              TfSpan<const GfMatrix3f> jointXforms,
                              TfSpan<const int> jointIndices,
                              TfSpan<const float> jointWeights,
                              const int numInfluencesPerPoint,
                              TfSpan<const int> faceVertexIndices,
                              TfSpan<GfVec3f> normals,
                              const bool inSerial)
{
    return _SkinFaceVaryingNormals(
        skinningMethod, geomBindTransform, jointXforms, jointIndices, jointWeights,
        numInfluencesPerPoint, faceVertexIndices, normals, inSerial);
}


bool
UsdSkelSkinNormalsLBS(const GfMatrix3d& geomBindTransform,
                      TfSpan<const GfMatrix3d> jointXforms,
                      TfSpan<const int> jointIndices,
                      TfSpan<const float> jointWeights,
                      const int numInfluencesPerPoint,
                      TfSpan<GfVec3f> normals,
                      const bool inSerial)
{
    return UsdSkelSkinNormals(UsdSkelTokens->classicLinear,
                              geomBindTransform,
                              jointXforms,
                              jointIndices,
                              jointWeights,
                              numInfluencesPerPoint,
                              normals,
                              inSerial);
}


bool
UsdSkelSkinNormalsLBS(const GfMatrix3f& geomBindTransform,
                      TfSpan<const GfMatrix3f> jointXforms,
                      TfSpan<const int> jointIndices,
                      TfSpan<const float> jointWeights,
                      const int numInfluencesPerPoint,
                      TfSpan<GfVec3f> normals,
                      const bool inSerial)
{
    return UsdSkelSkinNormals(UsdSkelTokens->classicLinear,
                              geomBindTransform,
                              jointXforms,
                              jointIndices,
                              jointWeights,
                              numInfluencesPerPoint,
                              normals,
                              inSerial);
}


bool
UsdSkelSkinNormalsLBS(const GfMatrix3d& geomBindTransform,
                      TfSpan<const GfMatrix3d> jointXforms,
                      TfSpan<const GfVec2f> influences,
                      const int numInfluencesPerPoint,
                      TfSpan<GfVec3f> normals,
                      const bool inSerial)
{
    return UsdSkelSkinNormals(UsdSkelTokens->classicLinear,
                              geomBindTransform,
                              jointXforms,
                              influences,
                              numInfluencesPerPoint,
                              normals,
                              inSerial);
}


bool
UsdSkelSkinNormalsLBS(const GfMatrix3f& geomBindTransform,
                      TfSpan<const GfMatrix3f> jointXforms,
                      TfSpan<const GfVec2f> influences,
                      const int numInfluencesPerPoint,
                      TfSpan<GfVec3f> normals,
                      const bool inSerial)
{
    return UsdSkelSkinNormals(UsdSkelTokens->classicLinear,
                              geomBindTransform,
                              jointXforms,
                              influences,
                              numInfluencesPerPoint,
                              normals,
                              inSerial);
}


bool
UsdSkelSkinFaceVaryingNormalsLBS(const GfMatrix3d& geomBindTransform,
                                 TfSpan<const GfMatrix3d> jointXforms,
                                 TfSpan<const int> jointIndices,
                                 TfSpan<const float> jointWeights,
                                 const int numInfluencesPerPoint,
                                 TfSpan<const int> faceVertexIndices,
                                 TfSpan<GfVec3f> normals,
                                 const bool inSerial)
{
    return _SkinFaceVaryingNormals(
        UsdSkelTokens->classicLinear, geomBindTransform, jointXforms, jointIndices, jointWeights,
        numInfluencesPerPoint, faceVertexIndices, normals, inSerial);
}


bool
UsdSkelSkinFaceVaryingNormalsLBS(const GfMatrix3f& geomBindTransform,
                                 TfSpan<const GfMatrix3f> jointXforms,
                                 TfSpan<const int> jointIndices,
                                 TfSpan<const float> jointWeights,
                                 const int numInfluencesPerPoint,
                                 TfSpan<const int> faceVertexIndices,
                                 TfSpan<GfVec3f> normals,
                                 const bool inSerial)
{
    return _SkinFaceVaryingNormals(
        UsdSkelTokens->classicLinear, geomBindTransform, jointXforms, jointIndices, jointWeights,
        numInfluencesPerPoint, faceVertexIndices, normals, inSerial);
}


namespace {


template <typename Matrix4, typename InfluencesFn>
bool
UsdSkel_SkinTransformLBS(const Matrix4& geomBindTransform,
                         TfSpan<const Matrix4> jointXforms,
                         const InfluencesFn& influencesFn,
                         Matrix4* xform)
{
    TRACE_FUNCTION();

    if (!xform) {
        TF_CODING_ERROR("'xform' is null");
        return false;
    }

    // Early-out for the common case where an object is rigidly
    // bound to a single joint.
    if (influencesFn.size() == 1 &&
        GfIsClose(influencesFn.GetWeight(0), 1.0f, 1e-6)) {
        const int jointIdx = influencesFn.GetIndex(0);
        if (jointIdx >= 0 &&
            static_cast<size_t>(jointIdx) < jointXforms.size()) {
            *xform = geomBindTransform*jointXforms[jointIdx];
            return true;
        } else {
            TF_WARN("Out of range joint index %d at index 0"
                    " (num joints = %zu).", jointIdx, jointXforms.size());
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
        for (size_t wi = 0; wi < influencesFn.size(); ++wi) {
            const int jointIdx = influencesFn.GetIndex(wi);
            if (jointIdx >= 0 &&
                static_cast<size_t>(jointIdx) < jointXforms.size()) {
                const float w = influencesFn.GetWeight(wi);
                if (w != 0.0f) {
                    // XXX: See the notes from _SkinPointsLBS():
                    // affine transforms should be okay.
                    p += GfVec3f(
                        jointXforms[jointIdx].TransformAffine(initialP))*w;
                }
            } else {
                TF_WARN("Out of range joint index %d at index %zu"
                        " (num joints = %zu).",
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


template <typename Matrix4, typename InfluencesFn>
bool
UsdSkel_SkinTransformDQS(const Matrix4& geomBindTransform,
                         TfSpan<const Matrix4> jointXforms,
                         const InfluencesFn& influencesFn,
                         Matrix4* xform)
{
    TRACE_FUNCTION();

    if (!xform) {
        TF_CODING_ERROR("'xform' is null");
        return false;
    }

    // Early-out for the common case where an object is rigidly
    // bound to a single joint.
    if (influencesFn.size() == 1 &&
        GfIsClose(influencesFn.GetWeight(0), 1.0f, 1e-6)) {
        const int jointIdx = influencesFn.GetIndex(0);
        if (jointIdx >= 0 &&
            static_cast<size_t>(jointIdx) < jointXforms.size()) {
            *xform = geomBindTransform*jointXforms[jointIdx];
            return true;
        } else {
            TF_WARN("Out of range joint index %d at index 0"
                    " (num joints = %zu).", jointIdx, jointXforms.size());
            return false;
        }
    }

    // Convert joint transformation matrices to dual quaternions
    std::vector<GfDualQuatd> jointDualQuats(jointXforms.size());
    std::vector<GfMatrix3f> jointScales(jointXforms.size());
    bool hasJointScale(false);
    _ConvertToDualQuaternions(jointXforms,
                              TfSpan<GfDualQuatd>(jointDualQuats),
                              TfSpan<GfMatrix3f>(jointScales),
                              &hasJointScale);

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

    GfQuatd pivotQuat(0.0);
    const int pivotIdx = _GetPivotJointIndex(0, jointDualQuats.size(),
                                             influencesFn, influencesFn.size());
    if (pivotIdx >= 0)
        pivotQuat = jointDualQuats[pivotIdx].GetReal();

    std::vector<GfVec3f> scaledPoints(4, GfVec3f(0));
    GfDualQuatd weightedSumDQ = GfDualQuatd::GetZero();

    for (size_t wi = 0; wi < influencesFn.size(); ++wi) {
        const int jointIdx = influencesFn.GetIndex(wi);
        if (jointIdx >= 0 &&
            static_cast<size_t>(jointIdx) < jointDualQuats.size()) {
            float w = influencesFn.GetWeight(wi);
            if (w != 0.0f) {
                // XXX: See the notes from _SkinPointsDQS():

                // Apply scale using LBS, if any of jointScales is not identity
                if (hasJointScale) {
                    for (int pi = 0; pi < 4; ++pi) {
                        const GfVec3f &initialP = framePoints[pi];
                        scaledPoints[pi] += (initialP * jointScales[jointIdx]) * w;
                    }
                }

                // Apply rotation & translation using DQS
                const GfDualQuatd &jointDQ = jointDualQuats[jointIdx];
                // Flip the dual quaternion, if necessary, to make it
                // on the same hemisphere as the pivotQuat.
                if ( GfDot(jointDQ.GetReal(), pivotQuat) < 0.0 )
                    w = -w;
                weightedSumDQ += (jointDQ * w);
            }
        } else {
            TF_WARN("Out of range joint index %d at index %zu"
                    " (num joints = %zu).",
                    jointIdx, wi, jointDualQuats.size());
            return false;
        }
    }

    weightedSumDQ.Normalize();
    for (int pi = 0; pi < 4; ++pi) {
        const GfVec3f &scaledP = (hasJointScale) ? scaledPoints[pi] : framePoints[pi];
        framePoints[pi] = GfVec3f(weightedSumDQ.Transform(scaledP));
    }

    const GfVec3f skinnedPivot = framePoints[3];
    xform->SetTranslate(skinnedPivot);
    for (int i = 0; i < 3; ++i) {
        xform->SetRow3(i, (framePoints[i]-skinnedPivot));
    }
    return true;
}


template <typename Matrix4>
bool
UsdSkel_NonInterleavedSkinTransform(const TfToken& skinningMethod,
                                    const Matrix4& geomBindTransform,
                                    TfSpan<const Matrix4> jointXforms,
                                    TfSpan<const int> jointIndices,
                                    TfSpan<const float> jointWeights,
                                    Matrix4* xform)
{
    if (jointIndices.size() != jointWeights.size()) {
        TF_WARN("Size of jointIndices [%zu] != size of jointWeights [%zu]",
                jointIndices.size(), jointWeights.size());
        return false;
    }

    const _NonInterleavedInfluencesFn influencesFn{jointIndices, jointWeights};
    if (skinningMethod == UsdSkelTokens->classicLinear) {
        return UsdSkel_SkinTransformLBS(geomBindTransform, jointXforms,
                                        influencesFn, xform);
    } else if (skinningMethod == UsdSkelTokens->dualQuaternion) {
        return UsdSkel_SkinTransformDQS(geomBindTransform, jointXforms,
                                        influencesFn, xform);
    } else {
        TF_WARN("Unknown skinning method: '%s' ", skinningMethod.GetText());
        return false;
    }
}


} // namespace


bool
UsdSkelSkinTransform(const TfToken& skinningMethod,
                     const GfMatrix4d& geomBindTransform,
                     TfSpan<const GfMatrix4d> jointXforms,
                     TfSpan<const int> jointIndices,
                     TfSpan<const float> jointWeights,
                     GfMatrix4d* xform)
{
    return UsdSkel_NonInterleavedSkinTransform(
        skinningMethod, geomBindTransform, jointXforms, jointIndices, jointWeights, xform);
}


bool
UsdSkelSkinTransform(const TfToken& skinningMethod,
                     const GfMatrix4f& geomBindTransform,
                     TfSpan<const GfMatrix4f> jointXforms,
                     TfSpan<const int> jointIndices,
                     TfSpan<const float> jointWeights,
                     GfMatrix4f* xform)
{
    return UsdSkel_NonInterleavedSkinTransform(
        skinningMethod, geomBindTransform, jointXforms, jointIndices, jointWeights, xform);
}


bool
UsdSkelSkinTransform(const TfToken& skinningMethod,
                     const GfMatrix4d& geomBindTransform,
                     TfSpan<const GfMatrix4d> jointXforms,
                     TfSpan<const GfVec2f> influences,
                     GfMatrix4d* xform)
{
    const _InterleavedInfluencesFn influencesFn{influences};
    if (skinningMethod == UsdSkelTokens->classicLinear) {
        return UsdSkel_SkinTransformLBS(geomBindTransform, jointXforms,
                                        influencesFn, xform);
    } else if (skinningMethod == UsdSkelTokens->dualQuaternion) {
        return UsdSkel_SkinTransformDQS(geomBindTransform, jointXforms,
                                        influencesFn, xform);
    } else {
        return false;
    }
}


bool
UsdSkelSkinTransform(const TfToken& skinningMethod,
                     const GfMatrix4f& geomBindTransform,
                     TfSpan<const GfMatrix4f> jointXforms,
                     TfSpan<const GfVec2f> influences,
                     GfMatrix4f* xform)
{
    const _InterleavedInfluencesFn influencesFn{influences};
    if (skinningMethod == UsdSkelTokens->classicLinear) {
        return UsdSkel_SkinTransformLBS(geomBindTransform, jointXforms,
                                        influencesFn, xform);
    } else if (skinningMethod == UsdSkelTokens->dualQuaternion) {
        return UsdSkel_SkinTransformDQS(geomBindTransform, jointXforms,
                                        influencesFn, xform);
    } else {
        return false;
    }
}


/// \deprecated
bool
UsdSkelSkinTransform(const TfToken& skinningMethod,
                     const GfMatrix4d& geomBindTransform,
                     const GfMatrix4d* jointXforms,
                     const size_t numJoints,
                     const int* jointIndices,
                     const float* jointWeights,
                     const size_t numInfluences,
                     GfMatrix4d* xform)
{
    return UsdSkel_NonInterleavedSkinTransform(
        skinningMethod, geomBindTransform,
        TfSpan<const GfMatrix4d>(jointXforms, numJoints),
        TfSpan<const int>(jointIndices, numInfluences),
        TfSpan<const float>(jointWeights, numInfluences),
        xform);
}



bool
UsdSkelSkinTransformLBS(const GfMatrix4d& geomBindTransform,
                        TfSpan<const GfMatrix4d> jointXforms,
                        TfSpan<const int> jointIndices,
                        TfSpan<const float> jointWeights,
                        GfMatrix4d* xform)
{
    return UsdSkelSkinTransform(UsdSkelTokens->classicLinear,
                                geomBindTransform,
                                jointXforms,
                                jointIndices,
                                jointWeights,
                                xform);
}


bool
UsdSkelSkinTransformLBS(const GfMatrix4f& geomBindTransform,
                        TfSpan<const GfMatrix4f> jointXforms,
                        TfSpan<const int> jointIndices,
                        TfSpan<const float> jointWeights,
                        GfMatrix4f* xform)
{
    return UsdSkelSkinTransform(UsdSkelTokens->classicLinear,
                                geomBindTransform,
                                jointXforms,
                                jointIndices,
                                jointWeights,
                                xform);
}


bool
UsdSkelSkinTransformLBS(const GfMatrix4d& geomBindTransform,
                        TfSpan<const GfMatrix4d> jointXforms,
                        TfSpan<const GfVec2f> influences,
                        GfMatrix4d* xform)
{
    return UsdSkelSkinTransform(UsdSkelTokens->classicLinear,
                                geomBindTransform,
                                jointXforms,
                                influences,
                                xform);
}


bool
UsdSkelSkinTransformLBS(const GfMatrix4f& geomBindTransform,
                        TfSpan<const GfMatrix4f> jointXforms,
                        TfSpan<const GfVec2f> influences,
                        GfMatrix4f* xform)
{
    return UsdSkelSkinTransform(UsdSkelTokens->classicLinear,
                                geomBindTransform,
                                jointXforms,
                                influences,
                                xform);
}


/// \deprecated
bool
UsdSkelSkinTransformLBS(const GfMatrix4d& geomBindTransform,
                        const GfMatrix4d* jointXforms,
                        const size_t numJoints,
                        const int* jointIndices,
                        const float* jointWeights,
                        const size_t numInfluences,
                        GfMatrix4d* xform)
{
    return UsdSkelSkinTransform(UsdSkelTokens->classicLinear,
                                geomBindTransform,
                                jointXforms,
                                numJoints,
                                jointIndices,
                                jointWeights,
                                numInfluences,
                                xform);
}


namespace {


/// Apply indexed offsets to \p points.
bool
UsdSkel_ApplyIndexedBlendShape(const float weight,
                               const TfSpan<const GfVec3f> offsets,
                               const TfSpan<const int> indices,
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
                const int index = indices[i];
                if (index >= 0 && static_cast<size_t>(index) < points.size()) {
                    points[index] += offsets[i]*weight;
                }  else {
                    // XXX: If one offset index is bad, an asset has probably
                    // gotten out of sync, and probably many other indices
                    // will be invalid, too. Bail out early.
                    TF_WARN("Out of range point index %d (num points = %zu).",
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
                       const TfSpan<const int> indices,
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


PXR_NAMESPACE_CLOSE_SCOPE
