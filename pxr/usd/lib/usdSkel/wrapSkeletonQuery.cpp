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


using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE


namespace {


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

    class_<This>("SkeletonQuery", no_init)

        .def(!self)
        .def(self == self)
        .def(self != self)
        
        .def("__str__", &This::GetDescription)

        .def("GetPrim", &This::GetPrim,
             return_value_policy<return_by_value>())

        .def("GetSkeleton", &This::GetSkeleton,
             return_value_policy<return_by_value>())

        .def("GetAnimQuery", &This::GetAnimQuery,
             return_value_policy<return_by_value>())

        .def("GetTopology", &This::GetTopology,
             return_value_policy<return_by_value>())

        .def("GetJointOrder", &This::GetJointOrder)
        
        .def("GetJointWorldBindTransforms", &_GetJointWorldBindTransforms)
        
        .def("ComputeJointLocalTransforms", &_ComputeJointLocalTransforms,
             (arg("time")=UsdTimeCode::Default(), arg("atRest")=false))
        
        .def("ComputeJointSkelTransforms", &_ComputeJointSkelTransforms,
             (arg("time")=UsdTimeCode::Default(), arg("atRest")=false))

        .def("ComputeJointWorldTransforms", &_ComputeJointWorldTransforms,
             (arg("time")=UsdTimeCode::Default(), arg("atRest")=false))

        .def("ComputeSkinningTransforms", &_ComputeSkinningTransforms,
             (arg("time")=UsdTimeCode::Default()))

        .def("ComputeJointRestRelativeTransforms",
             &_ComputeJointRestRelativeTransforms,
             (arg("time")=UsdTimeCode::Default()))
        
        ;
}            
