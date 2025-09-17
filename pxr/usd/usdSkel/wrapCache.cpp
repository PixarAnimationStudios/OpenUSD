//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/cache.h"

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

#include "pxr/external/boost/python.hpp"
#include "pxr/external/boost/python/extract.hpp"


PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;


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


void wrapUsdSkelCache()
{
    using This = UsdSkelCache;

    class_<This>("Cache", init<>())

        .def("Clear", &This::Clear)

        .def("Populate", &This::Populate,
             (arg("skelRoot"), arg("predicate")))

        .def("GetSkelQuery", &This::GetSkelQuery)
        
        .def("GetSkinningQuery", &This::GetSkinningQuery)

        .def("GetAnimQuery",
             (UsdSkelAnimQuery (UsdSkelCache::*)(const UsdPrim&) const)
             &This::GetAnimQuery,
             (arg("prim")))

        .def("GetAnimQuery",
             (UsdSkelAnimQuery (UsdSkelCache::*)(const UsdSkelAnimation&) const)
             &This::GetAnimQuery,
             (arg("anim")))

        .def("ComputeSkelBindings", &_ComputeSkelBindings,
             return_value_policy<TfPySequenceToList>(),
             (arg("skelRoot"), arg("predicate")))

        .def("ComputeSkelBinding", &_ComputeSkelBinding,
             (arg("skelRoot"), arg("skel"), arg("predicate")))
        ;
}            
