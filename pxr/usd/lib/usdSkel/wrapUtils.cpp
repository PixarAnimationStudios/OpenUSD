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

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/relationship.h"

#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/topology.h"
#include "pxr/usd/usdSkel/utils.h"

#include <boost/python.hpp>
#include <boost/python/extract.hpp>


using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE


namespace {


VtMatrix4dArray
_ComputeJointLocalTransforms(const UsdSkelTopology& topology,
                             const VtMatrix4dArray& xforms,
                             const VtMatrix4dArray& inverseXforms,
                             const object& rootInverseXformObj)
{
    VtMatrix4dArray jointLocalXforms;
    if(rootInverseXformObj) {
        GfMatrix4d rootInverseXform = extract<GfMatrix4d>(rootInverseXformObj);
        UsdSkelComputeJointLocalTransforms(topology, xforms, inverseXforms,
                                           &jointLocalXforms, &rootInverseXform);
    } else {
        UsdSkelComputeJointLocalTransforms(topology, xforms, inverseXforms,
                                           &jointLocalXforms);
    }
    return jointLocalXforms;
}


VtMatrix4dArray
_ComputeJointLocalTransforms_NoInvXforms(const UsdSkelTopology& topology,
                                         const VtMatrix4dArray& xforms,
                                         const object& rootInverseXformObj)
{
    VtMatrix4dArray jointLocalXforms;
    if (rootInverseXformObj) {
        GfMatrix4d rootInverseXform = extract<GfMatrix4d>(rootInverseXformObj);
        UsdSkelComputeJointLocalTransforms(topology, xforms, &jointLocalXforms,
                                           &rootInverseXform);
    } else {
        UsdSkelComputeJointLocalTransforms(topology, xforms, &jointLocalXforms);
    }
    return jointLocalXforms;
}


VtMatrix4dArray
_ConcatJointTransforms(const UsdSkelTopology& topology,
                       const VtMatrix4dArray& jointLocalXforms,
                       const object& rootXformObj)
{
    VtMatrix4dArray xforms;
    if(rootXformObj) {
        GfMatrix4d rootXform = extract<GfMatrix4d>(rootXformObj);
        UsdSkelConcatJointTransforms(topology, jointLocalXforms,
                                     &xforms, &rootXform);
    } else {
        UsdSkelConcatJointTransforms(topology, jointLocalXforms, &xforms);
    }
    return xforms;
}


tuple
_DecomposeTransform(const GfMatrix4d& mx)
{
    GfVec3f t;
    GfQuatf r;
    GfVec3h s;
    if(!UsdSkelDecomposeTransform(mx, &t, &r, &s)) {
        // XXX: Want this case to throw an exception.
        TF_CODING_ERROR("Failed decomposing transform. "
                        "The transform may be singular.");
    }
    return boost::python::make_tuple(t, r, s);
}


tuple
_DecomposeTransforms(const VtMatrix4dArray& xforms)
{
    VtVec3fArray t;
    VtQuatfArray r;
    VtVec3hArray s;
    if(!UsdSkelDecomposeTransforms(xforms, &t, &r, &s)) {
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
    return UsdSkelMakeTransform(translate, rotate, scale);
}


VtMatrix4dArray
_MakeTransforms(const VtVec3fArray& translations,
                const VtQuatfArray& rotations,
                const VtVec3hArray& scales)
{
    VtMatrix4dArray xforms;
    UsdSkelMakeTransforms(translations, rotations, scales, &xforms);
    return xforms;
}
                

VtVec3fArray
_ComputeJointsExtent(const VtMatrix4dArray& xforms, float pad,
                     const object& rootXformObj)
{
    VtVec3fArray extent;

    extract<GfMatrix4d> x(rootXformObj);
    if(x.check()) {
        const GfMatrix4d& rootXform = x;
        UsdSkelComputeJointsExtent(xforms, &extent, pad, &rootXform);
    } else {
        UsdSkelComputeJointsExtent(xforms, &extent, pad);
    }
    return extent;
}


bool
_NormalizeWeights(VtFloatArray& weights,
                  int numInfluencesPerComponent)
{
    return UsdSkelNormalizeWeights(&weights, numInfluencesPerComponent);
}


bool
_SortInfluences(VtIntArray& indices,
                VtFloatArray& weights,
                int numInfluencesPerComponent)
{
    return UsdSkelSortInfluences(&indices, &weights, numInfluencesPerComponent);
}


bool
_ExpandConstantInfluencesToVarying(object& arrayObj, size_t size)
{
    extract<VtIntArray&> x(arrayObj);
    if(x.check()) {
        VtIntArray& array = x;
        return UsdSkelExpandConstantInfluencesToVarying(&array, size);
    } else {
        VtFloatArray& array = extract<VtFloatArray&>(arrayObj);
        return UsdSkelExpandConstantInfluencesToVarying(&array, size);
    }
}


bool
_ResizeInfluences(object& arrayObj,
                  int srcNumInfluencesPerPoint,
                  int newNumInfluencesPerPoint)
{
    extract<VtIntArray&> x(arrayObj);
    if(x.check()) {
        VtIntArray& array = x;   
        return UsdSkelResizeInfluences(
            &array, srcNumInfluencesPerPoint, newNumInfluencesPerPoint);
    } else {
        VtFloatArray& array = extract<VtFloatArray&>(arrayObj);
        return UsdSkelResizeInfluences(
            &array, srcNumInfluencesPerPoint, newNumInfluencesPerPoint);
    }
}


bool
_SkinPointsLBS(const GfMatrix4d& geomBindTransform,
               const VtMatrix4dArray& jointXforms,
               const VtIntArray& jointIndices,
               const VtFloatArray& jointWeights,
               int numInfluencesPerPoint,
               VtVec3fArray& points)
{
    return UsdSkelSkinPointsLBS(geomBindTransform, jointXforms, jointIndices,
                                jointWeights, numInfluencesPerPoint, &points);
}


GfMatrix4d
_SkinTransformLBS(const GfMatrix4d& geomBindTransform,
                  const VtMatrix4dArray& jointXforms,
                  const VtIntArray& jointIndices,
                  const VtFloatArray& jointWeights)
{
    GfMatrix4d xform;
    if(!UsdSkelSkinTransformLBS(geomBindTransform, jointXforms,
                                jointIndices, jointWeights, &xform)) {
        xform = geomBindTransform;
    }
    return xform;
}


template <typename T>
std::vector<T>
_PyListToVector(const list& l)
{
    std::vector<T> vec(len(l));
    for (size_t i = 0; i < vec.size(); ++i) {
        vec[i] = extract<T>(l[i]);
    }
    return vec;
}


} // namespace


void wrapUsdSkelUtils()
{
    def("IsSkelAnimationPrim", &UsdSkelIsSkelAnimationPrim, (arg("prim")));

    def("IsSkinnablePrim", &UsdSkelIsSkinnablePrim, (arg("prim")));

    def("ComputeJointLocalTransforms", &_ComputeJointLocalTransforms,
        (arg("topology"), arg("xforms"), arg("inverseXforms"),
         arg("rootInverseXform")=object()));

    def("ComputeJointLocalTransforms",
        &_ComputeJointLocalTransforms_NoInvXforms,
        (arg("topology"), arg("xforms"),
         arg("rootInverseXform")=object()));

    def("ConcatJointTransforms", &_ConcatJointTransforms,
        (arg("topology"), arg("jointLocalXforms"),
         arg("rootXform")=object()));

    def("DecomposeTransform", &_DecomposeTransform,
        "Decompose a transform into a (translate,rotate,scale) tuple.");

    def("DecomposeTransforms", &_DecomposeTransforms,
        "Decompose a transform array into a "
        "(translations,rotations,scales) tuple.");

    def("MakeTransform", &_MakeTransform,
        (arg("translate"), arg("rotate"), arg("scale")));

    def("MakeTransforms", &_MakeTransforms,
        (arg("translations"), arg("rotations"), arg("scales")));
        
    def("ComputeJointsExtent", &_ComputeJointsExtent,
        (arg("xforms"), arg("pad")=0.0f, arg("rootXform")=object()));

    def("NormalizeWeights", &_NormalizeWeights,
        (arg("weights"), arg("numInfluencesPerComponent")));

    def("SortInfluences", &_SortInfluences,
        (arg("indices"), arg("weights"), arg("numInfluencesPerComponent")));

    def("ExpandConstantInfluencesToVarying",
        &_ExpandConstantInfluencesToVarying,
        (arg("array"), arg("size")));

    def("ResizeInfluences", &_ResizeInfluences,
        (arg("array"),
         arg("srcNumInfluencesPerComponent"),
         arg("newNumInfluencesPerComponent")));

    def("SkinPointsLBS", &_SkinPointsLBS,
        (arg("geomBindTransform"),
         arg("jointXforms"),
         arg("jointIndices"),
         arg("jointWeights"),
         arg("numInfluencesPerPoint"),
         arg("points")));

    def("SkinTransformLBS", &_SkinTransformLBS,
        (arg("geomBindTransform"),
         arg("jointXforms"),
         arg("jointIndices"),
         arg("jointWeights")));

    def("ApplyBlendShape", &UsdSkelApplyBlendShape,
        (arg("weight"),
         arg("offsets"),
         arg("indices"),
         arg("points")));

    def("BakeSkinning", ((bool (*)(const UsdSkelRoot&,
                                   const GfInterval&))&UsdSkelBakeSkinning),
        (arg("root"), arg("interval")=GfInterval::GetFullInterval()));

    def("BakeSkinning", ((bool (*)(const UsdPrimRange&,
                                   const GfInterval&))&UsdSkelBakeSkinning),
        (arg("range"), arg("interval")=GfInterval::GetFullInterval()));
}
