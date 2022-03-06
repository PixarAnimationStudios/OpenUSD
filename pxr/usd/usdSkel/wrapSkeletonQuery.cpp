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
#include "pxr/usd/usdSkel/skeletonQuery.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usdSkel/animQuery.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/topology.h"

#include <boost/python.hpp>
#include <boost/python/extract.hpp>



PXR_NAMESPACE_USING_DIRECTIVE


namespace pxrUsdUsdSkelWrapSkeletonQuery {


VtMatrix4dArray
_ComputeJointLocalTransforms(UsdSkelSkeletonQuery& self,
                             UsdTimeCode time, bool atRest)
{
    VtMatrix4dArray xforms;
    self.ComputeJointLocalTransforms(&xforms, time, atRest);
    return xforms;
}


VtMatrix4dArray
_ComputeJointSkelTransforms(UsdSkelSkeletonQuery& self,
                            UsdTimeCode time, bool atRest)
{
    VtMatrix4dArray xforms;
    self.ComputeJointSkelTransforms(&xforms, time, atRest);
    return xforms;
}


VtMatrix4dArray
_ComputeJointWorldTransforms(UsdSkelSkeletonQuery& self,
                             UsdGeomXformCache& xfCache,
                             bool atRest)
{
    VtMatrix4dArray xforms;
    self.ComputeJointWorldTransforms(&xforms, &xfCache, atRest);
    return xforms;
}


VtMatrix4dArray
_ComputeSkinningTransforms(UsdSkelSkeletonQuery& self, UsdTimeCode time)
{
    VtMatrix4dArray xforms;
    self.ComputeSkinningTransforms(&xforms, time);
    return xforms;
}


VtMatrix4dArray
_GetJointWorldBindTransforms(UsdSkelSkeletonQuery& self)
{
    VtMatrix4dArray xforms;
    self.GetJointWorldBindTransforms(&xforms);
    return xforms;
}


VtMatrix4dArray
_ComputeJointRestRelativeTransforms(UsdSkelSkeletonQuery& self,
                                    UsdTimeCode time)
{
    VtMatrix4dArray xforms;
    self.ComputeJointRestRelativeTransforms(&xforms, time);
    return xforms;
}


} // namespace


void wrapUsdSkelSkeletonQuery()
{
    using This = UsdSkelSkeletonQuery;

    boost::python::class_<This>("SkeletonQuery", boost::python::no_init)

        .def(!boost::python::self)
        .def(boost::python::self == boost::python::self)
        .def(boost::python::self != boost::python::self)
        
        .def("__str__", &This::GetDescription)

        .def("GetPrim", &This::GetPrim,
             boost::python::return_value_policy<boost::python::return_by_value>())

        .def("GetSkeleton", &This::GetSkeleton,
             boost::python::return_value_policy<boost::python::return_by_value>())

        .def("GetAnimQuery", &This::GetAnimQuery,
             boost::python::return_value_policy<boost::python::return_by_value>())

        .def("GetTopology", &This::GetTopology,
             boost::python::return_value_policy<boost::python::return_by_value>())

        .def("GetMapper", &This::GetMapper,
             boost::python::return_value_policy<boost::python::return_by_value>())

        .def("GetJointOrder", &This::GetJointOrder)
        
        .def("GetJointWorldBindTransforms", &pxrUsdUsdSkelWrapSkeletonQuery::_GetJointWorldBindTransforms)
        
        .def("ComputeJointLocalTransforms", &pxrUsdUsdSkelWrapSkeletonQuery::_ComputeJointLocalTransforms,
             (boost::python::arg("time")=UsdTimeCode::Default(), boost::python::arg("atRest")=false))
        
        .def("ComputeJointSkelTransforms", &pxrUsdUsdSkelWrapSkeletonQuery::_ComputeJointSkelTransforms,
             (boost::python::arg("time")=UsdTimeCode::Default(), boost::python::arg("atRest")=false))

        .def("ComputeJointWorldTransforms", &pxrUsdUsdSkelWrapSkeletonQuery::_ComputeJointWorldTransforms,
             (boost::python::arg("time")=UsdTimeCode::Default(), boost::python::arg("atRest")=false))

        .def("ComputeSkinningTransforms", &pxrUsdUsdSkelWrapSkeletonQuery::_ComputeSkinningTransforms,
             (boost::python::arg("time")=UsdTimeCode::Default()))

        .def("ComputeJointRestRelativeTransforms",
             &pxrUsdUsdSkelWrapSkeletonQuery::_ComputeJointRestRelativeTransforms,
             (boost::python::arg("time")=UsdTimeCode::Default()))

        .def("HasBindPose", &This::HasBindPose)

        .def("HasRestPose", &This::HasRestPose)
        
        ;
}            
