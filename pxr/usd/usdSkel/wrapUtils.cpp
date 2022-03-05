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

#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/range3f.h"
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

#include <boost/python.hpp>
#include <boost/python/extract.hpp>



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
boost::python::tuple
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
    return boost::python::make_tuple(t, r, s);
}


template <typename Matrix4>
boost::python::tuple
_DecomposeTransforms(const TfSpan<Matrix4>& xforms)
{
    VtVec3fArray t(xforms.size());
    VtQuatfArray r(xforms.size());
    VtVec3hArray s(xforms.size());
    if (!UsdSkelDecomposeTransforms(xforms, t, r, s)) {
        TF_CODING_ERROR("Failed decomposing transforms. "
                        "Some transforms may be singular.");
    }
    return boost::python::make_tuple(t, r, s);
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


template <class Matrix3, class Matrix4>
void _WrapUtilsT()
{
    boost::python::def("ComputeJointLocalTransforms",
        static_cast<bool (*)(const UsdSkelTopology&, TfSpan<const Matrix4>,
                             TfSpan<const Matrix4>, TfSpan<Matrix4>,
                             const Matrix4*)>(
                                 &UsdSkelComputeJointLocalTransforms),
        (boost::python::arg("topology"), boost::python::arg("xforms"), boost::python::arg("inverseXforms"),
         boost::python::arg("jointLocalXforms"), boost::python::arg("rootInverseXform")=boost::python::object()));

    boost::python::def("ComputeJointLocalTransforms",
        static_cast<bool (*)(const UsdSkelTopology&, TfSpan<const Matrix4>,
                             TfSpan<Matrix4>, const Matrix4*)>(
                                 &UsdSkelComputeJointLocalTransforms),
        (boost::python::arg("topology"), boost::python::arg("xforms"),
         boost::python::arg("jointLocalXforms"), boost::python::arg("rootInverseXform")=boost::python::object()));

    boost::python::def("ConcatJointTransforms",
        static_cast<bool (*)(const UsdSkelTopology&, TfSpan<const Matrix4>,
                             TfSpan<Matrix4>, const Matrix4*)>(
                                 &UsdSkelConcatJointTransforms),
        (boost::python::arg("topology"), boost::python::arg("jointLocalXforms"),
         boost::python::arg("rootXform")=boost::python::object()));

    boost::python::def("DecomposeTransform", &_DecomposeTransform<Matrix4>,
        "Decompose a transform into a (translate,rotate,scale) tuple.");

    boost::python::def("DecomposeTransforms", &_DecomposeTransforms<Matrix4>,
        "Decompose a transform array into a "
        "(translations,rotations,scales) tuple.");

    boost::python::def("ComputeJointsExtent", _ComputeJointsExtent<Matrix4>,
        (boost::python::arg("xforms"), boost::python::arg("pad")=0.0f, boost::python::arg("rootXform")=boost::python::object()));

    boost::python::def("SkinPointsLBS",
        static_cast<bool (*)(const Matrix4&, TfSpan<const Matrix4>,
                             TfSpan<const int>, TfSpan<const float>,
                             int, TfSpan<GfVec3f>, bool)>(
                                 &UsdSkelSkinPointsLBS),
        (boost::python::arg("geomBindTransform"),
         boost::python::arg("jointXforms"),
         boost::python::arg("jointIndices"),
         boost::python::arg("jointWeights"),
         boost::python::arg("numInfluencesPerPoint"),
         boost::python::arg("points"),
         boost::python::arg("inSerial")=true));

    boost::python::def("SkinPointsLBS",
        static_cast<bool (*)(const Matrix4&, TfSpan<const Matrix4>,
                             TfSpan<const GfVec2f>, int,
                             TfSpan<GfVec3f>, bool)>(
                                 &UsdSkelSkinPointsLBS),
        (boost::python::arg("geomBindTransform"),
         boost::python::arg("jointXforms"),
         boost::python::arg("influences"),
         boost::python::arg("numInfluencesPerPoint"),
         boost::python::arg("points"),
         boost::python::arg("inSerial")=true));



    boost::python::def("SkinNormalsLBS",
        static_cast<bool (*)(const Matrix3&, TfSpan<const Matrix3>,
                             TfSpan<const int>, TfSpan<const float>,
                             int, TfSpan<GfVec3f>, bool)>(
                                 &UsdSkelSkinNormalsLBS),
        (boost::python::arg("geomBindTransform"),
         boost::python::arg("jointXforms"),
         boost::python::arg("jointIndices"),
         boost::python::arg("jointWeights"),
         boost::python::arg("numInfluencesPerPoint"),
         boost::python::arg("normals"),
         boost::python::arg("inSerial")=true));

    boost::python::def("SkinNormalsLBS",
        static_cast<bool (*)(const Matrix3&, TfSpan<const Matrix3>,
                             TfSpan<const GfVec2f>, int,
                             TfSpan<GfVec3f>, bool)>(
                                 &UsdSkelSkinNormalsLBS),
        (boost::python::arg("geomBindTransform"),
         boost::python::arg("jointXforms"),
         boost::python::arg("influences"),
         boost::python::arg("numInfluencesPerPoint"),
         boost::python::arg("normals"),
         boost::python::arg("inSerial")=true));

    boost::python::def("SkinTransformLBS", &_InterleavedSkinTransformLBS<Matrix4>,
        (boost::python::arg("geomBindTransform"),
         boost::python::arg("jointXforms"),
         boost::python::arg("influences")));

    boost::python::def("SkinTransformLBS", &_NonInterleavedSkinTransformLBS<Matrix4>,
        (boost::python::arg("geomBindTransform"),
         boost::python::arg("jointXforms"),
         boost::python::arg("jointIndices"),
         boost::python::arg("jointWeights")));
}


} // namespace


void wrapUsdSkelUtils()
{
    // Wrap methods supporting different matrix precisions.
    _WrapUtilsT<GfMatrix3d, GfMatrix4d>();
    _WrapUtilsT<GfMatrix3f, GfMatrix4f>();

    boost::python::def("IsSkelAnimationPrim", &UsdSkelIsSkelAnimationPrim, (boost::python::arg("prim")));

    boost::python::def("IsSkinnablePrim", &UsdSkelIsSkinnablePrim, (boost::python::arg("prim")));

    // deprecated
    boost::python::def("ComputeJointLocalTransforms", &_ComputeJointLocalTransforms,
        (boost::python::arg("topology"), boost::python::arg("xforms"), boost::python::arg("inverseXforms"),
         boost::python::arg("rootInverseXform")=boost::python::object()));

    // deprecated
    boost::python::def("ComputeJointLocalTransforms",
        &_ComputeJointLocalTransforms_NoInvXforms,
        (boost::python::arg("topology"), boost::python::arg("xforms"),
         boost::python::arg("rootInverseXform")=boost::python::object()));

    // deprecated
    boost::python::def("ConcatJointTransforms", &_ConcatJointTransforms,
        (boost::python::arg("topology"), boost::python::arg("jointLocalXforms"),
         boost::python::arg("rootXform")=boost::python::object()));

    boost::python::def("MakeTransform", &_MakeTransform,
        (boost::python::arg("translate"), boost::python::arg("rotate"), boost::python::arg("scale")));

    // deprecated
    boost::python::def("MakeTransforms", &_MakeTransforms,
        (boost::python::arg("translations"), boost::python::arg("rotations"), boost::python::arg("scales")));

    boost::python::def("NormalizeWeights",
        static_cast<bool (*)(TfSpan<float>,int)>(
            &UsdSkelNormalizeWeights),
        (boost::python::arg("weights"), boost::python::arg("numInfluencesPerComponent")));

    boost::python::def("SortInfluences",
        static_cast<bool (*)(TfSpan<int>, TfSpan<float>,int)>(
            &UsdSkelSortInfluences),
        (boost::python::arg("indices"), boost::python::arg("weights"), boost::python::arg("numInfluencesPerComponent")));

    boost::python::def("ExpandConstantInfluencesToVarying",
        &_ExpandConstantInfluencesToVarying<int>,
        (boost::python::arg("array"), boost::python::arg("size")));

    boost::python::def("ExpandConstantInfluencesToVarying",
        &_ExpandConstantInfluencesToVarying<float>,
        (boost::python::arg("array"), boost::python::arg("size")));

    boost::python::def("ResizeInfluences", &_ResizeInfluences<int>,
        (boost::python::arg("array"),
         boost::python::arg("srcNumInfluencesPerComponent"),
         boost::python::arg("newNumInfluencesPerComponent")));

    boost::python::def("ResizeInfluences", &_ResizeInfluences<float>,
        (boost::python::arg("array"),
         boost::python::arg("srcNumInfluencesPerComponent"),
         boost::python::arg("newNumInfluencesPerComponent")));

    boost::python::def("InterleaveInfluences", &UsdSkelInterleaveInfluences,
        (boost::python::arg("indices"), boost::python::arg("weights"),
         boost::python::arg("interleavedInfluences")));

    boost::python::def("ApplyBlendShape", &UsdSkelApplyBlendShape,
        (boost::python::arg("weight"),
         boost::python::arg("offsets"),
         boost::python::arg("indices"),
         boost::python::arg("points")));
}
