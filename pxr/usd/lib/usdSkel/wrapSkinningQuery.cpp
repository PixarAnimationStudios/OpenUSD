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
#include "pxr/usd/usdSkel/skinningQuery.h"

#include "pxr/base/gf/interval.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/usd/usdGeom/boundable.h"

#include <boost/python.hpp>


using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE


namespace {


object
_ComputeJointInfluences(const UsdSkelSkinningQuery& self, UsdTimeCode time)
{
    VtIntArray indices;
    VtFloatArray weights;
    if (self.ComputeJointInfluences(&indices, &weights, time)) {
        return boost::python::make_tuple(indices, weights);
    }
    return object();
}


object
_ComputeVaryingJointInfluences(const UsdSkelSkinningQuery& self,
                               size_t numPoints,
                               UsdTimeCode time)
{
    VtIntArray indices;
    VtFloatArray weights;
    if (self.ComputeVaryingJointInfluences(numPoints, &indices,
                                          &weights, time)) {
        return boost::python::make_tuple(indices, weights);
    }
    return object();
}


std::vector<double>
_GetTimeSamples(const UsdSkelSkinningQuery& self)
{
    std::vector<double> times;
    self.GetTimeSamples(&times);
    return times;
}


std::vector<double>
_GetTimeSamplesInInterval(const UsdSkelSkinningQuery& self,
                          const GfInterval& interval)
{
    std::vector<double> times;
    self.GetTimeSamplesInInterval(interval, &times);
    return times;
}


object
_GetJointOrder(const UsdSkelSkinningQuery& self)
{
    VtTokenArray jointOrder;
    if (self.GetJointOrder(&jointOrder))
        return object(jointOrder);
    return object();
}


template <typename Matrix4>
bool
_ComputeSkinnedPoints(const UsdSkelSkinningQuery& self,
                      const VtArray<Matrix4>& xforms,
                      VtVec3fArray& points,
                      UsdTimeCode time)
{
    return self.ComputeSkinnedPoints(xforms, &points, time);
}


template <typename Matrix4>
Matrix4
_ComputeSkinnedTransform(const UsdSkelSkinningQuery& self,
                         const VtArray<Matrix4>& xforms,
                         UsdTimeCode time)
{
    Matrix4 xform;
    self.ComputeSkinnedTransform(xforms, &xform, time);
    return xform;
}


} // namespace


void wrapUsdSkelSkinningQuery()
{
    using This = UsdSkelSkinningQuery;

    class_<This>("UsdSkelSkinningQuery")

        .def(!self)

        .def("__str__", &This::GetDescription)

        .def("GetPrim", &This::GetPrim,
             return_value_policy<return_by_value>())

        .def("HasJointInfluences", &This::HasJointInfluences)

        .def("HasBlendShapes", &This::HasBlendShapes)
        
        .def("GetNumInfluencesPerComponent",
             &This::GetNumInfluencesPerComponent)

        .def("GetInterpolation", &This::GetInterpolation,
             return_value_policy<return_by_value>())

        .def("IsRigidlyDeformed", &This::IsRigidlyDeformed)

        .def("GetGeomBindTransformAttr", &This::GetGeomBindTransformAttr,
             return_value_policy<return_by_value>())

        .def("GetJointIndicesPrimvar", &This::GetJointIndicesPrimvar,
             return_value_policy<return_by_value>())

        .def("GetJointWeightsPrimvar", &This::GetJointWeightsPrimvar,
             return_value_policy<return_by_value>())

        .def("GetMapper", &This::GetMapper,
             return_value_policy<return_by_value>())

        .def("GetJointOrder", &_GetJointOrder)

        .def("GetTimeSamples", &_GetTimeSamples)

        .def("GetTimeSamplesInInterval", &_GetTimeSamplesInInterval)

        .def("ComputeJointInfluences", &_ComputeJointInfluences,
             (arg("time")=UsdTimeCode::Default()))

        .def("ComputeVaryingJointInfluences", &_ComputeVaryingJointInfluences,
             (arg("numPoints"), arg("time")=UsdTimeCode::Default()))

        .def("ComputeSkinnedPoints", &_ComputeSkinnedPoints<GfMatrix4d>,
             (arg("xforms"), arg("points"),
              arg("time")=UsdTimeCode::Default()))

        .def("ComputeSkinnedPoints", &_ComputeSkinnedPoints<GfMatrix4f>,
             (arg("xforms"), arg("points"),
              arg("time")=UsdTimeCode::Default()))

        .def("ComputeSkinnedTransform", &_ComputeSkinnedTransform<GfMatrix4d>,
             (arg("xforms"),
              arg("time")=UsdTimeCode::Default()))

        .def("ComputeSkinnedTransform", &_ComputeSkinnedTransform<GfMatrix4f>,
             (arg("xforms"),
              arg("time")=UsdTimeCode::Default()))

        .def("ComputeExentsPadding",
             static_cast<float (UsdSkelSkinningQuery::*)(
                 const VtMatrix4dArray&,
                 const UsdGeomBoundable&) const>(
                     &This::ComputeExtentsPadding),
             (arg("skelRestXforms"),
              arg("boundable"),
              arg("time")=UsdTimeCode::Default()))

        .def("ComputeExentsPadding",
             static_cast<float (UsdSkelSkinningQuery::*)(
                 const VtMatrix4fArray&,
                 const UsdGeomBoundable&) const>(
                     &This::ComputeExtentsPadding),
             (arg("skelRestXforms"),
              arg("boundable"),
              arg("time")=UsdTimeCode::Default()))

        .def("GetGeomBindTransform", &This::GetGeomBindTransform,
             (arg("time")=UsdTimeCode::Default()))
        ;
}
