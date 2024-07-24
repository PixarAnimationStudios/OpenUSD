//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

static size_t __hash__(const UsdSkelSkeletonQuery &self)
{
    return TfHash{}(self);
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
        .def("__hash__" , __hash__)

        .def("GetPrim", &This::GetPrim,
             return_value_policy<return_by_value>())

        .def("GetSkeleton", &This::GetSkeleton,
             return_value_policy<return_by_value>())

        .def("GetAnimQuery", &This::GetAnimQuery,
             return_value_policy<return_by_value>())

        .def("GetTopology", &This::GetTopology,
             return_value_policy<return_by_value>())

        .def("GetMapper", &This::GetMapper,
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

        .def("HasBindPose", &This::HasBindPose)

        .def("HasRestPose", &This::HasRestPose)
        
        ;
}            
