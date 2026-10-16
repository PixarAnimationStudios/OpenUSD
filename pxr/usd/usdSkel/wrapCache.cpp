//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/cache.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/usd/usd/prim.h"

#include "pxr/usd/usdSkel/animation.h"
#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/skeletonQuery.h"
#include "pxr/usd/usdSkel/skinningQuery.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

std::vector<UsdSkelBinding>
_ComputeSkelBindings(const UsdSkelCache& self,  
                     const UsdSkelRoot& skelRoot,
                     const Usd_PrimFlagsPredicate predicate)
{   
    std::vector<UsdSkelBinding> bindings;
    self.ComputeSkelBindings(skelRoot, &bindings, predicate);
    return bindings;
}

UsdSkelBinding
_ComputeSkelBinding(const UsdSkelCache& self,
                    const UsdSkelRoot& skelRoot,
                    const UsdSkelSkeleton& skel,
                    const Usd_PrimFlagsPredicate predicate)
{
    UsdSkelBinding binding;
    self.ComputeSkelBinding(skelRoot, skel, &binding, predicate);
    return binding;
}

} // namespace

void wrapUsdSkelCache(nb::module_ &m)
{
    using This = UsdSkelCache;

    nb::class_<This>(m, "Cache")
        .def(nb::init<>())

        .def("Clear", &This::Clear)

        .def("Populate", &This::Populate,
             nb::arg("skelRoot"), nb::arg("predicate"))

        .def("GetSkelQuery", &This::GetSkelQuery)
        
        .def("GetSkinningQuery", &This::GetSkinningQuery)

        .def("GetAnimQuery",
             (UsdSkelAnimQuery (UsdSkelCache::*)(const UsdPrim&) const)
             &This::GetAnimQuery,
             nb::arg("prim"))

        .def("GetAnimQuery",
             (UsdSkelAnimQuery (UsdSkelCache::*)(const UsdSkelAnimation&) const)
             &This::GetAnimQuery,
             nb::arg("anim"))

        .def("ComputeSkelBindings", &_ComputeSkelBindings,
             nb::arg("skelRoot"), nb::arg("predicate"))

        .def("ComputeSkelBinding", &_ComputeSkelBinding,
             nb::arg("skelRoot"), nb::arg("skel"), nb::arg("predicate"))
        ;

}
