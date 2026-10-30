//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>

namespace nb = nanobind;
PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static
nb::tuple
_GetLocalTransformation(
        UsdGeomXformCache& self,
        const UsdPrim& prim)
{
    bool resetsXformStack;
    GfMatrix4d localXform = self.GetLocalTransformation(prim, &resetsXformStack);

    return nb::make_tuple(localXform, resetsXformStack);
}

static
nb::tuple
_ComputeRelativeTransform(UsdGeomXformCache& self,
                          const UsdPrim& prim,
                          const UsdPrim& ancestor)
{
    bool resetXformStack;
    GfMatrix4d xform =
        self.ComputeRelativeTransform(prim, ancestor, &resetXformStack);
    
    return nb::make_tuple(xform, resetXformStack);
}

} // anonymous namespace 

void wrapUsdGeomXformCache(nanobind::module_ &m)
{
    typedef UsdGeomXformCache XformCache;

    nb::class_<XformCache>(m, "XformCache")
        .def(nb::init<UsdTimeCode>(), nb::arg("time") = UsdTimeCode::Default())
        .def("GetLocalToWorldTransform",
             &XformCache::GetLocalToWorldTransform, nb::arg("prim"))
        .def("GetParentToWorldTransform",
             &XformCache::GetParentToWorldTransform, nb::arg("prim"))
        .def("GetLocalTransformation",
             &_GetLocalTransformation, nb::arg("prim"))
        .def("ComputeRelativeTransform",
             &_ComputeRelativeTransform, nb::arg("prim"), nb::arg("ancestor"))
        .def("Clear", &XformCache::Clear)
        .def("SetTime", &XformCache::SetTime, nb::arg("time"))
        .def("GetTime", &XformCache::GetTime)

        .def("Swap", &XformCache::Swap, nb::arg("other"))
        ;

}

