//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include <boost/python/class.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static
tuple
_GetLocalTransformation(
        UsdGeomXformCache& self,
        const UsdPrim& prim)
{
    bool resetsXformStack;
    GfMatrix4d localXform = self.GetLocalTransformation(prim, &resetsXformStack);

    return make_tuple(localXform, resetsXformStack);
}

static
tuple
_ComputeRelativeTransform(UsdGeomXformCache& self,
                          const UsdPrim& prim,
                          const UsdPrim& ancestor)
{
    bool resetXformStack;
    GfMatrix4d xform =
        self.ComputeRelativeTransform(prim, ancestor, &resetXformStack);
    
    return make_tuple(xform, resetXformStack);
}

} // anonymous namespace 

void wrapUsdGeomXformCache()
{
    typedef UsdGeomXformCache XformCache;

    class_<XformCache>("XformCache")
        .def(init<UsdTimeCode>(arg("time")))
        .def("GetLocalToWorldTransform",
             &XformCache::GetLocalToWorldTransform, arg("prim"))
        .def("GetParentToWorldTransform",
             &XformCache::GetParentToWorldTransform, arg("prim"))
        .def("GetLocalTransformation",
             &_GetLocalTransformation, arg("prim"))
        .def("ComputeRelativeTransform",
             &_ComputeRelativeTransform, (arg("prim"), arg("ancestor")))
        .def("Clear", &XformCache::Clear)
        .def("SetTime", &XformCache::SetTime, arg("time"))
        .def("GetTime", &XformCache::GetTime)

        .def("Swap", &XformCache::Swap, arg("other"))
        ;
}

