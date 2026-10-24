//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/utils.h"

#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/range3f.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/relationship.h"

#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/topology.h"
#include "pxr/usd/usdSkel/utils.h"

#include <nanobind/nanobind.h>

namespace nb = nanobind;

namespace nanobind::detail {
template <typename T>
struct type_caster<PXR_NS::TfSpan<T>> {
    NB_TYPE_CASTER(PXR_NS::TfSpan<T>, const_name("TfSpan"))

    using ElemType = std::remove_const_t<T>;
    using ArrayType = PXR_NS::VtArray<ElemType>;
    std::optional<ArrayType> holder_;

    bool from_python(handle src, uint8_t flags, cleanup_list *cleanup) noexcept {
        if (!src.is_valid() || src.is_none()) {
            return false;
        }
        ArrayType *arr = nullptr;
        if (NB_CALL(nb_type_get)(NB_CTX_C(cleanup), &typeid(ArrayType),
                                 src.ptr(), flags, cleanup,
                                 reinterpret_cast<void **>(&arr)) &&
            arr) {
            value = PXR_NS::TfSpan<T>(*arr);
            return true;
        }
        if constexpr (std::is_const_v<T>) {
            if (flags & static_cast<uint8_t>(cast_flags::convert)) {
                make_caster<ArrayType> caster;
                if (caster.from_python(src, flags, cleanup)) {
                    holder_.emplace(static_cast<ArrayType &>(caster));
                    value = PXR_NS::TfSpan<T>(*holder_);
                    return true;
                }
            }
        }
        return false;
    }

    static handle from_cpp(PXR_NS::TfSpan<T> src, rv_policy, cleanup_list *) noexcept {
        ArrayType arr(src.begin(), src.end());
        return type_caster<ArrayType>::from_cpp(arr, rv_policy::copy, nullptr);
    }
};
} // namespace nanobind::detail

PXR_NAMESPACE_USING_DIRECTIVE


namespace {


// deprecated
VtMatrix4dArray
_ComputeJointLocalTransforms(const UsdSkelTopology& topology,
                             const VtMatrix4dArray& xforms,
                             const VtMatrix4dArray& inverseXforms,
                             const GfMatrix4d* rootInverseXform=nullptr)
{
    VtMatrix4dArray jointLocalXforms;
    UsdSkelComputeJointLocalTransforms(topology, xforms, inverseXforms,
                                       &jointLocalXforms, rootInverseXform);
    return jointLocalXforms;
}


// deprecated
VtMatrix4dArray
_ComputeJointLocalTransforms_NoInvXforms(
    const UsdSkelTopology& topology,
    const VtMatrix4dArray& xforms,
    const GfMatrix4d* rootInverseXform=nullptr)
{
    VtMatrix4dArray jointLocalXforms;
    UsdSkelComputeJointLocalTransforms(topology, xforms, &jointLocalXforms,
                                       rootInverseXform);
    return jointLocalXforms;
}


// depreacted
VtMatrix4dArray
_ConcatJointTransforms(const UsdSkelTopology& topology,
                       const VtMatrix4dArray& jointLocalXforms,
                       const GfMatrix4d* rootXform=nullptr)
{
    VtMatrix4dArray xforms;
    UsdSkelConcatJointTransforms(topology, jointLocalXforms,
                                 &xforms, rootXform);
    return xforms;
}


template <typename Matrix4>
nb::tuple
_DecomposeTransform(const Matrix4& mx)
{
    GfVec3f t;
    GfQuatf r;
    GfVec3h s;
    if (!UsdSkelDecomposeTransform(mx, &t, &r, &s)) {
        // XXX: Want this case to throw an exception.
        TF_CODING_ERROR("Failed decomposing transform. "
                        "The transform may be singular.");
    }
    return nb::make_tuple(t, r, s);
}


template <typename Matrix4>
nb::tuple
_DecomposeTransforms(TfSpan<const Matrix4> xforms)
{
    VtVec3fArray t(xforms.size());
    VtQuatfArray r(xforms.size());
    VtVec3hArray s(xforms.size());
    if (!UsdSkelDecomposeTransforms(xforms, t, r, s)) {
        TF_CODING_ERROR("Failed decomposing transforms. "
                        "Some transforms may be singular.");
    }
    return nb::make_tuple(t, r, s);
}


GfMatrix4d
_MakeTransform(const GfVec3f& translate,
               const GfQuatf& rotate,
               const GfVec3h& scale)
{
    GfMatrix4d xform;
    UsdSkelMakeTransform(translate, rotate, scale, &xform);
    return xform;
}


VtMatrix4dArray
_MakeTransforms(TfSpan<const GfVec3f> translations,
                TfSpan<const GfQuatf> rotations,
                TfSpan<const GfVec3h> scales)
{
    VtMatrix4dArray xforms(translations.size());
    UsdSkelMakeTransforms(translations, rotations, scales, xforms);
    return xforms;
}


template <typename Matrix4>
GfRange3f
_ComputeJointsExtent(TfSpan<const Matrix4> xforms,
                     float pad=0,
                     const Matrix4* rootXform=nullptr)
{   
    GfRange3f range;
    UsdSkelComputeJointsExtent(xforms, &range, pad, rootXform);
    return range;
}


template <typename T>
bool
_ExpandConstantInfluencesToVarying(VtArray<T>& array, size_t size)
{
    return UsdSkelExpandConstantInfluencesToVarying(&array, size);
}


template <typename T>
bool
_ResizeInfluences(VtArray<T>& array,
                  int srcNumInfluencesPerPoint,
                  int newNumInfluencesPerPoint)
{
    return UsdSkelResizeInfluences(
        &array, srcNumInfluencesPerPoint, newNumInfluencesPerPoint);
}


template <typename Matrix4>
Matrix4
_InterleavedSkinTransformLBS(const Matrix4& geomBindTransform,
                             TfSpan<const Matrix4> jointXforms,
                             TfSpan<const GfVec2f> influences)
{
    Matrix4 xform;
    if (!UsdSkelSkinTransformLBS(geomBindTransform, jointXforms,
                                 influences, &xform)) {
        xform = geomBindTransform;
    }
    return xform;
}


template <typename Matrix4>
Matrix4
_NonInterleavedSkinTransformLBS(const Matrix4& geomBindTransform,
                                TfSpan<const Matrix4> jointXforms,
                                TfSpan<const int> jointIndices,
                                TfSpan<const float> jointWeights)
{
    Matrix4 xform;
    if (!UsdSkelSkinTransformLBS(geomBindTransform, jointXforms,
                                 jointIndices, jointWeights, &xform)) {
        xform = geomBindTransform;
    }
    return xform;
}


template <typename Matrix4>
Matrix4
_InterleavedSkinTransform(const TfToken& skinningMethod,
                          const Matrix4& geomBindTransform,
                          TfSpan<const Matrix4> jointXforms,
                          TfSpan<const GfVec2f> influences)
{
    Matrix4 xform;
    if (!UsdSkelSkinTransform(skinningMethod, geomBindTransform, jointXforms,
                              influences, &xform)) {
        xform = geomBindTransform;
    }
    return xform;
}


template <typename Matrix4>
Matrix4
_NonInterleavedSkinTransform(const TfToken& skinningMethod,
                             const Matrix4& geomBindTransform,
                             TfSpan<const Matrix4> jointXforms,
                             TfSpan<const int> jointIndices,
                             TfSpan<const float> jointWeights)
{
    Matrix4 xform;
    if (!UsdSkelSkinTransform(skinningMethod, geomBindTransform, jointXforms,
                              jointIndices, jointWeights, &xform)) {
        xform = geomBindTransform;
    }
    return xform;
}


template <class Matrix3, class Matrix4>
void _WrapUtilsT(nb::module_ &m)
{
    m.def("ComputeJointLocalTransforms",
          static_cast<bool (*)(const UsdSkelTopology&, TfSpan<const Matrix4>,
                               TfSpan<const Matrix4>, TfSpan<Matrix4>,
                               const Matrix4*)>(
                                   &UsdSkelComputeJointLocalTransforms),
          nb::arg("topology"), nb::arg("xforms"), nb::arg("inverseXforms"),
          nb::arg("jointLocalXforms"), nb::arg("rootInverseXform")=nullptr);

    m.def("ComputeJointLocalTransforms",
          static_cast<bool (*)(const UsdSkelTopology&, TfSpan<const Matrix4>,
                               TfSpan<Matrix4>, const Matrix4*)>(
                                   &UsdSkelComputeJointLocalTransforms),
          nb::arg("topology"), nb::arg("xforms"),
          nb::arg("jointLocalXforms"), nb::arg("rootInverseXform")=nullptr);

    m.def("ConcatJointTransforms",
          static_cast<bool (*)(const UsdSkelTopology&, TfSpan<const Matrix4>,
                               TfSpan<Matrix4>, const Matrix4*)>(
                                   &UsdSkelConcatJointTransforms),
          nb::arg("topology"), nb::arg("jointLocalXforms"),
          nb::arg("xforms"), nb::arg("rootXform")=nullptr);

    m.def("DecomposeTransform", &_DecomposeTransform<Matrix4>,
          "Decompose a transform into a (translate,rotate,scale) tuple.");

    m.def("DecomposeTransforms", &_DecomposeTransforms<Matrix4>,
          "Decompose a transform array into a "
          "(translations,rotations,scales) tuple.");

    m.def("ComputeJointsExtent", _ComputeJointsExtent<Matrix4>,
          nb::arg("xforms"), nb::arg("pad")=0.0f, nb::arg("rootXform")=nullptr);

    m.def("SkinPointsLBS",
          static_cast<bool (*)(const Matrix4&, TfSpan<const Matrix4>,
                               TfSpan<const int>, TfSpan<const float>,
                               int, TfSpan<GfVec3f>, bool)>(
                                   &UsdSkelSkinPointsLBS),
          nb::arg("geomBindTransform"),
          nb::arg("jointXforms"),
          nb::arg("jointIndices"),
          nb::arg("jointWeights"),
          nb::arg("numInfluencesPerPoint"),
          nb::arg("points"),
          nb::arg("inSerial")=true);

    m.def("SkinPointsLBS",
          static_cast<bool (*)(const Matrix4&, TfSpan<const Matrix4>,
                               TfSpan<const GfVec2f>, int,
                               TfSpan<GfVec3f>, bool)>(
                                   &UsdSkelSkinPointsLBS),
          nb::arg("geomBindTransform"),
          nb::arg("jointXforms"),
          nb::arg("influences"),
          nb::arg("numInfluencesPerPoint"),
          nb::arg("points"),
          nb::arg("inSerial")=true);

    m.def("SkinPoints",
          static_cast<bool (*)(const TfToken&, const Matrix4&, TfSpan<const Matrix4>,
                               TfSpan<const int>, TfSpan<const float>,
                               int, TfSpan<GfVec3f>, bool)>(
                                   &UsdSkelSkinPoints),
          nb::arg("skinningMethod"),
          nb::arg("geomBindTransform"),
          nb::arg("jointXforms"),
          nb::arg("jointIndices"),
          nb::arg("jointWeights"),
          nb::arg("numInfluencesPerPoint"),
          nb::arg("points"),
          nb::arg("inSerial")=true);

    m.def("SkinPoints",
          static_cast<bool (*)(const TfToken&, const Matrix4&, TfSpan<const Matrix4>,
                               TfSpan<const GfVec2f>, int,
                               TfSpan<GfVec3f>, bool)>(
                                   &UsdSkelSkinPoints),
          nb::arg("skinningMethod"),
          nb::arg("geomBindTransform"),
          nb::arg("jointXforms"),
          nb::arg("influences"),
          nb::arg("numInfluencesPerPoint"),
          nb::arg("points"),
          nb::arg("inSerial")=true);



    m.def("SkinNormalsLBS",
          static_cast<bool (*)(const Matrix3&, TfSpan<const Matrix3>,
                               TfSpan<const int>, TfSpan<const float>,
                               int, TfSpan<GfVec3f>, bool)>(
                                   &UsdSkelSkinNormalsLBS),
          nb::arg("geomBindTransform"),
          nb::arg("jointXforms"),
          nb::arg("jointIndices"),
          nb::arg("jointWeights"),
          nb::arg("numInfluencesPerPoint"),
          nb::arg("normals"),
          nb::arg("inSerial")=true);

    m.def("SkinNormalsLBS",
          static_cast<bool (*)(const Matrix3&, TfSpan<const Matrix3>,
                               TfSpan<const GfVec2f>, int,
                               TfSpan<GfVec3f>, bool)>(
                                   &UsdSkelSkinNormalsLBS),
          nb::arg("geomBindTransform"),
          nb::arg("jointXforms"),
          nb::arg("influences"),
          nb::arg("numInfluencesPerPoint"),
          nb::arg("normals"),
          nb::arg("inSerial")=true);

    m.def("SkinNormals",
          static_cast<bool (*)(const TfToken&, const Matrix3&, TfSpan<const Matrix3>,
                               TfSpan<const int>, TfSpan<const float>,
                               int, TfSpan<GfVec3f>, bool)>(
                                   &UsdSkelSkinNormals),
          nb::arg("skinningMethod"),
          nb::arg("geomBindTransform"),
          nb::arg("jointXforms"),
          nb::arg("jointIndices"),
          nb::arg("jointWeights"),
          nb::arg("numInfluencesPerPoint"),
          nb::arg("normals"),
          nb::arg("inSerial")=true);

    m.def("SkinNormals",
          static_cast<bool (*)(const TfToken&, const Matrix3&, TfSpan<const Matrix3>,
                               TfSpan<const GfVec2f>, int,
                               TfSpan<GfVec3f>, bool)>(
                                   &UsdSkelSkinNormals),
          nb::arg("skinningMethod"),
          nb::arg("geomBindTransform"),
          nb::arg("jointXforms"),
          nb::arg("influences"),
          nb::arg("numInfluencesPerPoint"),
          nb::arg("normals"),
          nb::arg("inSerial")=true);


    m.def("SkinTransformLBS", &_InterleavedSkinTransformLBS<Matrix4>,
          nb::arg("geomBindTransform"),
          nb::arg("jointXforms"),
          nb::arg("influences"));

    m.def("SkinTransformLBS", &_NonInterleavedSkinTransformLBS<Matrix4>,
          nb::arg("geomBindTransform"),
          nb::arg("jointXforms"),
          nb::arg("jointIndices"),
          nb::arg("jointWeights"));

    m.def("SkinTransform", &_InterleavedSkinTransform<Matrix4>,
          nb::arg("skinningMethod"),
          nb::arg("geomBindTransform"),
          nb::arg("jointXforms"),
          nb::arg("influences"));

    m.def("SkinTransform", &_NonInterleavedSkinTransform<Matrix4>,
          nb::arg("skinningMethod"),
          nb::arg("geomBindTransform"),
          nb::arg("jointXforms"),
          nb::arg("jointIndices"),
          nb::arg("jointWeights"));
}


} // namespace


void wrapUsdSkelUtils(nb::module_ &m)
{
    // Wrap methods supporting different matrix precisions.
    _WrapUtilsT<GfMatrix3d, GfMatrix4d>(m);
    _WrapUtilsT<GfMatrix3f, GfMatrix4f>(m);

    m.def("IsSkelAnimationPrim", &UsdSkelIsSkelAnimationPrim, nb::arg("prim"));

    m.def("IsSkinnablePrim", &UsdSkelIsSkinnablePrim, nb::arg("prim"));

    // deprecated
    m.def("ComputeJointLocalTransforms", &_ComputeJointLocalTransforms,
          nb::arg("topology"), nb::arg("xforms"), nb::arg("inverseXforms"),
          nb::arg("rootInverseXform")=nullptr);

    // deprecated
    m.def("ComputeJointLocalTransforms",
          &_ComputeJointLocalTransforms_NoInvXforms,
          nb::arg("topology"), nb::arg("xforms"),
          nb::arg("rootInverseXform")=nullptr);

    // deprecated
    m.def("ConcatJointTransforms", &_ConcatJointTransforms,
          nb::arg("topology"), nb::arg("jointLocalXforms"),
          nb::arg("rootXform")=nullptr);

    m.def("MakeTransform", &_MakeTransform,
          nb::arg("translate"), nb::arg("rotate"), nb::arg("scale"));

    // deprecated
    m.def("MakeTransforms", &_MakeTransforms,
          nb::arg("translations"), nb::arg("rotations"), nb::arg("scales"));

    m.def("NormalizeWeights",
          static_cast<bool (*)(TfSpan<float>, int, float)>(
              &UsdSkelNormalizeWeights),
          nb::arg("weights"), nb::arg("numInfluencesPerComponent"),
          nb::arg("eps")=std::numeric_limits<float>::epsilon());

    m.def("SortInfluences",
          static_cast<bool (*)(TfSpan<int>, TfSpan<float>, int)>(
              &UsdSkelSortInfluences),
          nb::arg("indices"), nb::arg("weights"), nb::arg("numInfluencesPerComponent"));

    m.def("ExpandConstantInfluencesToVarying",
          &_ExpandConstantInfluencesToVarying<int>,
          nb::arg("array"), nb::arg("size"));

    m.def("ExpandConstantInfluencesToVarying",
          &_ExpandConstantInfluencesToVarying<float>,
          nb::arg("array"), nb::arg("size"));

    m.def("ResizeInfluences", &_ResizeInfluences<int>,
          nb::arg("array"),
          nb::arg("srcNumInfluencesPerComponent"),
          nb::arg("newNumInfluencesPerComponent"));

    m.def("ResizeInfluences", &_ResizeInfluences<float>,
          nb::arg("array"),
          nb::arg("srcNumInfluencesPerComponent"),
          nb::arg("newNumInfluencesPerComponent"));

    m.def("InterleaveInfluences", &UsdSkelInterleaveInfluences,
          nb::arg("indices"), nb::arg("weights"),
          nb::arg("interleavedInfluences"));

    m.def("ApplyBlendShape", &UsdSkelApplyBlendShape,
          nb::arg("weight"),
          nb::arg("offsets"),
          nb::arg("indices"),
          nb::arg("points"));
}
