//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/skeletonQuery.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usdSkel/animQuery.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/topology.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;

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

void wrapUsdSkelSkeletonQuery(nb::module_ &m)
{
    using This = UsdSkelSkeletonQuery;

    nb::class_<This>(m, "SkeletonQuery")

        .def("__bool__", [](const This& self) { return bool(self); })
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        
        .def("__str__", &This::GetDescription)
        .def("__hash__", __hash__)

        .def("GetPrim", &This::GetPrim, nb::rv_policy::copy)

        .def("GetSkeleton", &This::GetSkeleton, nb::rv_policy::copy)

        .def("GetAnimQuery", &This::GetAnimQuery, nb::rv_policy::copy)

        .def("GetTopology", &This::GetTopology, nb::rv_policy::copy)

        .def("GetMapper", &This::GetMapper, nb::rv_policy::copy)

        .def("GetJointOrder", &This::GetJointOrder)
        
        .def("GetJointWorldBindTransforms", &_GetJointWorldBindTransforms)
        
        .def("ComputeJointLocalTransforms", &_ComputeJointLocalTransforms,
             nb::arg("time")=UsdTimeCode::Default(), nb::arg("atRest")=false)
        
        .def("ComputeJointSkelTransforms", &_ComputeJointSkelTransforms,
             nb::arg("time")=UsdTimeCode::Default(), nb::arg("atRest")=false)

        .def("ComputeJointWorldTransforms", &_ComputeJointWorldTransforms,
             nb::arg("time")=UsdTimeCode::Default(), nb::arg("atRest")=false)

        .def("ComputeSkinningTransforms", &_ComputeSkinningTransforms,
             nb::arg("time")=UsdTimeCode::Default())

        .def("ComputeJointRestRelativeTransforms",
             &_ComputeJointRestRelativeTransforms,
             nb::arg("time")=UsdTimeCode::Default())

        .def("HasBindPose", &This::HasBindPose)

        .def("HasRestPose", &This::HasRestPose)
        
        ;

}
